/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters

INSPIRED BY: https://gist.github.com/XBachirX/865b00ba7a7c86b4fc2d7443b2c4f238
*/

#ifndef CLIENTFTP_H
#define CLIENTFTP_H

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#define SV_DATACONNECTION_OPEN 125
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

// Function declarations
void log_message(const char* level, const char* format, ...);
int parse_url(char *input, struct URL *url);
int create_control_socket(const char *ip, int port);
int authenticate(int socket, const char* user, const char* pass);
int enter_passive_mode(int socket, char *ip, int *port);
int read_ftp_response(int socket, char* buffer);
int request_file(int socket, const char *resource);
int receive_file(int control_socket, int data_socket, const char *filename);
int close_connections(int control_socket, int data_socket);

#endif // CLIENTFTP_H