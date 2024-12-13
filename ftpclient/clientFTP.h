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

#define PORT 21
#define LEN 10000

//status
#define ST_DATACONNECTION_OPEN 125
#define ST_READY4TRANSF 150
#define ST_READY4AUTH 220
#define ST_GOODBYE 221
#define ST_TRANSFER_COMPLETE 226
#define ST_PASVMODE 227
#define ST_LOGINSUCCESS 230
#define ST_READY4PASS 331
#define ST_SWITCHTOBIN 200

#define DF_USR "anonymous"
#define DF_PWD "anonymous"

#define LOG_INFO    "[INFO] "
#define LOG_ERROR   "[ERROR] "
#define LOG_DEBUG   "[DEBUG] "

// URL structure
struct URL { char user[LEN],password[LEN],host[LEN],ip[LEN],resource[LEN],file[LEN]; };

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
