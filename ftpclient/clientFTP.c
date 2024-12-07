/** (C)2000-2021 FEUP
 *  tidy up some includes and parameters

*/
#include "clientFTP.h"

// Logging function
void log_message(const char* level, const char* format, ...) {
    time_t now;
    char timestamp[26];
    va_list args;
    
    time(&now);
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';
    
    printf("%s %s ", timestamp, level);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}

// URL parsing and validation
int parse_url(char *input, struct URL *url) {
    regex_t regex;
    regmatch_t matches[4];
    char pattern[] = "ftp://([^:/@]+:[^:/@]+@)?([^/]+)/(.*)";
    
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        log_message(LOG_ERROR, "Failed to compile regex");
        return -1;
    }
    
    if (regexec(&regex, input, 4, matches, 0) != 0) {
        log_message(LOG_ERROR, "Invalid FTP URL format");
        regfree(&regex);
        return -1;
    }
    
    if (matches[1].rm_so != -1) {
        char credentials[LEN];
        int len = matches[1].rm_eo - matches[1].rm_so - 1; 
        strncpy(credentials, input + matches[1].rm_so, len);
        credentials[len] = '\0';
        sscanf(credentials, "%[^:]:%s", url->user, url->password);
    } else {
        strcpy(url->user, DF_USR);
        strcpy(url->password, DF_PWD);
    }
    
    int host_len = matches[2].rm_eo - matches[2].rm_so;
    strncpy(url->host, input + matches[2].rm_so, host_len);
    url->host[host_len] = '\0';

    int path_len = matches[3].rm_eo - matches[3].rm_so;
    strncpy(url->resource, input + matches[3].rm_so, path_len);
    url->resource[path_len] = '\0';
    

    char *filename = strrchr(url->resource, '/');
    if (filename != NULL) {
        strcpy(url->file, filename + 1);
    } else {
        strcpy(url->file, url->resource);
    }
    

    struct hostent *h = gethostbyname(url->host);
    if (h == NULL) {
        log_message(LOG_ERROR, "Failed to resolve hostname: %s", url->host);
        regfree(&regex);
        return -1;
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    
    regfree(&regex);
    return 0;
}

// Create and connect socket
int create_control_socket(const char *ip, int port) {
    int socket_desc;
    struct sockaddr_in server;
    struct timeval timeout;
    
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        log_message(LOG_ERROR, "Could not create socket: %s", strerror(errno));
        return -1;
    }
    
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        log_message(LOG_ERROR, "Connection failed: %s", strerror(errno));
        close(socket_desc);
        return -1;
    }
    
    return socket_desc;
}

// Authenticate with FTP server
int authenticate(int socket, const char* user, const char* pass) {
    char command[LEN];
    char response[LEN];
    int response_code;

    snprintf(command, sizeof(command), "USER %s\r\n", user);
    if (write(socket, command, strlen(command)) < 0) {
        log_message(LOG_ERROR, "Failed to send username: %s", strerror(errno));
        return -1;
    }
    
    response_code = read_ftp_response(socket, response);
    
    switch (response_code) {
        case ST_READY4PASS:
            break;
        case ST_LOGINSUCCESS:
            log_message(LOG_INFO, "Logged in without password");
            return response_code;
        default:
            log_message(LOG_ERROR, "Unexpected response to USER command: %d", response_code);
            return -1;
    }
    

    snprintf(command, sizeof(command), "PASS %s\r\n", pass);
    if (write(socket, command, strlen(command)) < 0) {
        log_message(LOG_ERROR, "Failed to send password: %s", strerror(errno));
        return -1;
    }
    
    response_code = read_ftp_response(socket, response);
    
    if (response_code == ST_LOGINSUCCESS) {
        return response_code;
    } else {
        log_message(LOG_ERROR, "Authentication failed. Response: %d", response_code);
        return -1;
    }
}

// Enter passive mode
int enter_passive_mode(int socket, char *ip, int *port) {
    char command[] = "PASV\r\n";
    char response[LEN];
    int ip1, ip2, ip3, ip4, port1, port2;
    
    write(socket, command, strlen(command));
    int response_code = read_ftp_response(socket, response);
    
    if (response_code != ST_PASVMODE) {
        log_message(LOG_ERROR, "Passive mode request failed. Response: %d", response_code);
        return -1;
    }
    
    char *start = strchr(response, '(');
    if (!start) {
        log_message(LOG_ERROR, "Invalid PASV response format");
        return -1;
    }
    
    if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", 
               &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6) {
        log_message(LOG_ERROR, "Failed to parse IP and port from PASV response");
        return -1;
    }
    
    snprintf(ip, LEN, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    *port = port1 * 256 + port2;
    
    return 0;
}

// Read FTP server response
int read_ftp_response(int socket, char* buffer) {
    ssize_t n;
    size_t total_read = 0;
    int code = 0;
    char byte;
    char line[LEN];
    size_t line_length = 0;
    int multiline = 0;  

    memset(buffer, 0, LEN); 

    while (1) {
        
        n = read(socket, &byte, 1);
        if (n <= 0) {
            log_message(LOG_ERROR, "Failed to read server response: %s", strerror(errno));
            return -1;
        }

        if (line_length < LEN - 1) {
            line[line_length++] = byte;
        }
        line[line_length] = '\0';

        if (byte == '\n') {
            if (sscanf(line, "%d", &code) == 1) {
                if (line[3] == '-') {
                    multiline = 1; 
                } else if (line[3] == ' ') {
                    if (!multiline || (multiline && strncmp(line, buffer, 3) == 0)) {
                        strncat(buffer, line, LEN - total_read - 1);
                        break; 
                    }
                }
            }

            strncat(buffer, line, LEN - total_read - 1);
            total_read += line_length;
            line_length = 0; 
        }
    }
    
    return code;
}

// Request file transfer
int request_file(int socket, const char *resource) {
    char command[LEN];
    char response[LEN];

    snprintf(command, sizeof(command), "RETR %s\r\n", resource);
    if (write(socket, command, strlen(command)) <= 0) {
        log_message(LOG_ERROR, "Failed to send RETR command: %s", strerror(errno));
        return -1;
    }

    int code = read_ftp_response(socket, response);
    return code;
}

int receive_file(int control_socket, int data_socket, const char *filename) {
    char buffer[LEN];
    ssize_t bytes_received;
    FILE *file = fopen(filename, "wb");
    
    if (!file) {
        log_message(LOG_ERROR, "Failed to create output file");
        return -1;
    }
    
    while ((bytes_received = read(data_socket, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes_received, file) != bytes_received) {
            log_message(LOG_ERROR, "Failed to write to file");
            fclose(file);
            return -1;
        }
    }
    
    fclose(file);
    char response[LEN];
    return read_ftp_response(control_socket, response);
}

// Close all connections
int close_connections(int control_socket, int data_socket) {
    char command[] = "QUIT\r\n";
    char response[LEN];
    
    write(control_socket, command, strlen(command));
    read_ftp_response(control_socket, response);
    
    close(data_socket);
    close(control_socket);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        return 1;
    }
    
    struct URL url;
    memset(&url, 0, sizeof(url));
    
    log_message(LOG_INFO, "Starting FTP client...");
    
    if (parse_url(argv[1], &url) != 0) {
        log_message(LOG_ERROR, "Failed to parse URL");
        return 1;
    }
    
    log_message(LOG_INFO, "Connecting to %s (%s) as user %s", url.host, url.ip, url.user);
    
    int control_socket = create_control_socket(url.ip, PORT);
    if (control_socket < 0) {
        return 1;
    }
    
    char response[LEN];
    if (read_ftp_response(control_socket, response) != ST_READY4AUTH) {
        log_message(LOG_ERROR, "Server not ready");
        close(control_socket);
        return 1;
    }
    
    if (authenticate(control_socket, url.user, url.password) != ST_LOGINSUCCESS) {
        log_message(LOG_ERROR, "Authentication failed");
        close(control_socket);
        return 1;
    }
    
    char data_ip[LEN];
    int data_port;
    if (enter_passive_mode(control_socket, data_ip, &data_port) != 0) {
        log_message(LOG_ERROR, "Failed to enter passive mode");
        close(control_socket);
        return 1;
    }
    
    int data_socket = create_control_socket(data_ip, data_port);
    if (data_socket < 0) {
        close(control_socket);
        return 1;
    }

    int transfer_status = request_file(control_socket, url.resource);
    if (transfer_status != ST_READY4TRANSF && transfer_status != ST_DATACONNECTION_OPEN) {
        log_message(LOG_ERROR, "Failed to request file");
        close_connections(control_socket, data_socket);
        return 1;
    }
    
    log_message(LOG_INFO, "Downloading %s...", url.file);
    int received_status = receive_file(control_socket, data_socket, url.file);

    if (received_status != ST_TRANSFER_COMPLETE) {
        log_message(LOG_ERROR, "Might have failed to receive file. Check your files");
        close_connections(control_socket, data_socket);
        return 1;
    }
    
    close_connections(control_socket, data_socket);
    log_message(LOG_INFO, "Download completed successfully");
    
    return 0;
}