#pragma once

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <errno.h>
#include <string.h>

#define SERVER_PORT 21

// status codes
#define RETR 150 // Files are in good condition and ready to open data connection.
#define CONNECTED 220 // The service is ready to execute the new user's request.
#define DOWNLOAD_COMPLETED 226 // Download was sucessful.
#define PASSIVE_MODE 227 // Entering passive mode.
#define CORRECT_PASSWORD 230 // Users logged in to continue.
#define REQ_PASSWORD 331 // The user name is correct and a password is required.
#define WRONG_PASSWORD 530 // Is not logged in.

#define PASSIVE_CMD "PASV\r\n"

typedef struct
{
    char user[255];
    char password[255];
    char host[255];
    char url[255];
    char server[255];
    char ipaddr[32];
    int port;
} url_syntax;

/**
 * @brief 
 * 
 * @param name 
 */
char *get_IP(const char *name);

/**
 * @brief 
 * 
 * @param arguments 
 * @return int 
 */
int parse_arguments(char *arguments);

/**
 * @brief 
 * 
 */
void print_parsed_url();

static int get_reply(char *resp, int len, int socket_fd);

int login(char* user,char* password, int socket_fd);

int set_passive_mode(socket_fd);

int send_retr(int socket_fd);

int download(int socket_fd);
