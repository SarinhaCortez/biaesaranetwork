/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * 
 TO RUN:

gcc -o clientFTP clientFTP.c

./clientFTP URL
INSPIRED BY: https://gist.github.com/XBachirX/865b00ba7a7c86b4fc2d7443b2c4f238
*/
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <regex.h>
#include <stdarg.h>

#define FTP_PORT 21
#define MAX_LENGTH 1024
#define MAX_FILENAME_LEN 256
#define RECV_TIMEOUT_SECS 30

#define SV_READY4AUTH 220
#define SV_READY4PASS 331
#define SV_LOGINSUCCESS 230
#define SV_PASSIVE 227
#define SV_READY4TRANSFER 150
#define SV_TRANSFER_COMPLETE 226
#define SV_GOODBYE 221

#define URL_PATTERN "ftp://([^:/@]+:[^:/@]+@)?([^/]+)/(.*)"
#define DEFAULT_USER "anonymous"
#define DEFAULT_PASSWORD "anonymous"

#define LOG_INFO    "[INFO] "
#define LOG_ERROR   "[ERROR] "
#define LOG_DEBUG   "[DEBUG] "

// URL structure
struct URL {
    char user[MAX_LENGTH];
    char password[MAX_LENGTH];
    char host[MAX_LENGTH];
    char ip[MAX_LENGTH];
    char resource[MAX_LENGTH];
    char file[MAX_LENGTH];
};

void log_message(const char* level, const char* format, ...);
int parse_url(char *input, struct URL *url);
int create_control_socket(const char *ip, int port);
int authenticate(int socket, const char* user, const char* pass);
int enter_passive_mode(int socket, char *ip, int *port);
int read_ftp_response(int socket, char* buffer);
int request_file(int socket, const char *resource);
int receive_file(int control_socket, int data_socket, const char *filename);
int close_connections(int control_socket, int data_socket);

// Logging function implementation)
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

// Parse FTP URL
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
    
    // Extract credentials if present
    if (matches[1].rm_so != -1) {
        char credentials[MAX_LENGTH];
        int len = matches[1].rm_eo - matches[1].rm_so - 1; // -1 to remove '@'
        strncpy(credentials, input + matches[1].rm_so, len);
        credentials[len] = '\0';
        sscanf(credentials, "%[^:]:%s", url->user, url->password);
    } else {
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    }
    
    // Extract host
    int host_len = matches[2].rm_eo - matches[2].rm_so;
    strncpy(url->host, input + matches[2].rm_so, host_len);
    url->host[host_len] = '\0';
    
    // Extract resource path
    int path_len = matches[3].rm_eo - matches[3].rm_so;
    strncpy(url->resource, input + matches[3].rm_so, path_len);
    url->resource[path_len] = '\0';
    
    // Extract filename from resource path
    char *filename = strrchr(url->resource, '/');
    if (filename != NULL) {
        strcpy(url->file, filename + 1);
    } else {
        strcpy(url->file, url->resource);
    }
    
    // Resolve hostname to IP
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
    
    timeout.tv_sec = RECV_TIMEOUT_SECS;
    timeout.tv_usec = 0;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_message(LOG_ERROR, "Failed to set socket timeout");
        close(socket_desc);
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
    char command[MAX_LENGTH];
    char response[MAX_LENGTH];
    int response_code;
    
    // Send username
    snprintf(command, sizeof(command), "USER %s\r\n", user);
    if (write(socket, command, strlen(command)) < 0) {
        log_message(LOG_ERROR, "Failed to send username: %s", strerror(errno));
        return -1;
    }
    
    response_code = read_ftp_response(socket, response);
    
    switch (response_code) {
        case SV_READY4PASS:
            break;
        case SV_LOGINSUCCESS:
            log_message(LOG_INFO, "Logged in without password");
            return response_code;
        default:
            log_message(LOG_ERROR, "Unexpected response to USER command: %d", response_code);
            return -1;
    }
    
    // Send password
    snprintf(command, sizeof(command), "PASS %s\r\n", pass);
    if (write(socket, command, strlen(command)) < 0) {
        log_message(LOG_ERROR, "Failed to send password: %s", strerror(errno));
        return -1;
    }
    
    response_code = read_ftp_response(socket, response);
    
    if (response_code == SV_LOGINSUCCESS) {
        return response_code;
    } else {
        log_message(LOG_ERROR, "Authentication failed. Response: %d", response_code);
        return -1;
    }
}

// Enter passive mode
int enter_passive_mode(int socket, char *ip, int *port) {
    char command[] = "PASV\r\n";
    char response[MAX_LENGTH];
    int ip1, ip2, ip3, ip4, port1, port2;
    
    write(socket, command, strlen(command));
    int response_code = read_ftp_response(socket, response);
    
    if (response_code != SV_PASSIVE) {
        log_message(LOG_ERROR, "Passive mode request failed. Response: %d", response_code);
        return -1;
    }
    
    // More robust parsing of PASV response
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
    
    snprintf(ip, MAX_LENGTH, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    *port = port1 * 256 + port2;
    
    return 0;
}

// Read FTP server response
int read_ftp_response(int socket, char* buffer) {
    ssize_t n;
    size_t total_read = 0;
    
    memset(buffer, 0, MAX_LENGTH);
    
    while (total_read < MAX_LENGTH - 1) {
        n = read(socket, buffer + total_read, MAX_LENGTH - total_read - 1);
        if (n <= 0) {
            log_message(LOG_ERROR, "Failed to read server response: %s", strerror(errno));
            return -1;
        }
        
        total_read += n;
        buffer[total_read] = '\0';
        
        // Check if response is complete (ends with \r\n)
        if (total_read >= 2 && 
            buffer[total_read-2] == '\r' && 
            buffer[total_read-1] == '\n') {
            break;
        }
    }
    
    // Log full server response for debugging
    log_message(LOG_DEBUG, "Full server response: %s", buffer);
    
    int code;
    if (sscanf(buffer, "%d", &code) != 1) {
        log_message(LOG_ERROR, "Could not parse response code");
        return -1;
    }
    
    return code;
}

// Request file transfer
int request_file(int socket, const char *resource) {
    char command[MAX_LENGTH];
    char response[MAX_LENGTH];
    
    snprintf(command, sizeof(command), "RETR %s\r\n", resource);
    write(socket, command, strlen(command));
    return read_ftp_response(socket, response);
}

// Receive file data
int receive_file(int control_socket, int data_socket, const char *filename) {
    char buffer[MAX_LENGTH];
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
    char response[MAX_LENGTH];
    return read_ftp_response(control_socket, response);
}

// Close all connections
int close_connections(int control_socket, int data_socket) {
    char command[] = "QUIT\r\n";
    char response[MAX_LENGTH];
    
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
    
    // Parse URL
    if (parse_url(argv[1], &url) != 0) {
        log_message(LOG_ERROR, "Failed to parse URL");
        return 1;
    }
    
    log_message(LOG_INFO, "Connecting to %s (%s) as user %s", url.host, url.ip, url.user);
    
    // Create control connection
    int control_socket = create_control_socket(url.ip, FTP_PORT);
    if (control_socket < 0) {
        return 1;
    }
    
    char response[MAX_LENGTH];
    if (read_ftp_response(control_socket, response) != SV_READY4AUTH) {
        log_message(LOG_ERROR, "Server not ready");
        close(control_socket);
        return 1;
    }
    
    // Authenticate
    if (authenticate(control_socket, url.user, url.password) != SV_LOGINSUCCESS) {
        log_message(LOG_ERROR, "Authentication failed");
        close(control_socket);
        return 1;
    }
    
    // Enter passive mode
    char data_ip[MAX_LENGTH];
    int data_port;
    if (enter_passive_mode(control_socket, data_ip, &data_port) != 0) {
        log_message(LOG_ERROR, "Failed to enter passive mode");
        close(control_socket);
        return 1;
    }
    
    // Create data connection
    int data_socket = create_control_socket(data_ip, data_port);
    if (data_socket < 0) {
        close(control_socket);
        return 1;
    }
    
    // Request file
    if (request_file(control_socket, url.resource) != SV_READY4TRANSFER) {
        log_message(LOG_ERROR, "Failed to request file");
        close_connections(control_socket, data_socket);
        return 1;
    }
    
    // Receive file
    log_message(LOG_INFO, "Downloading %s...", url.file);
    if (receive_file(control_socket, data_socket, url.file) != SV_TRANSFER_COMPLETE) {
        log_message(LOG_ERROR, "Failed to receive file");
        close_connections(control_socket, data_socket);
        return 1;
    }
    
    // Clean up
    close_connections(control_socket, data_socket);
    log_message(LOG_INFO, "Download completed successfully");
    
    return 0;
}
