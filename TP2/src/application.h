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
#define RETR 150               // Files are in good condition and ready to open data connection.
#define CONNECTED 220          // The service is ready to execute the new user's request.
#define DOWNLOAD_COMPLETED 226 // Download was sucessful.
#define PASSIVE_MODE 227       // Entering passive mode.
#define CORRECT_PASSWORD 230   // Authentication completed.
#define REQ_PASSWORD 331       // The user name is correct and a password is required.
#define WRONG_PASSWORD 530     // Authentication failed.

#define PASSIVE_CMD "PASV\r\n"
#define MAX_BUFFER_SIZE 1024
#define IP_ADDRESS_SIZE 32

typedef struct
{
    char user[MAX_BUFFER_SIZE / 4];
    char password[MAX_BUFFER_SIZE / 4];
    char host[MAX_BUFFER_SIZE / 4];
    char url[MAX_BUFFER_SIZE / 4];
    char filename[MAX_BUFFER_SIZE / 4];
    char server[MAX_BUFFER_SIZE / 4];
    char ipaddr[IP_ADDRESS_SIZE];
    int port;
} url_syntax;

/**
 * @brief Gets the ip address by the host name
 * 
 * @param name Name of the host
 */
char *get_IP(const char *name);

/**
 * @brief Parses aguments
 * 
 * @param arguments Arguments given
 * @return int Returns 0 on success, 1 otherwise
 */
int parse_arguments(char *arguments);

/**
 * @brief Prints to screen the information parsed 
 */
void print_parsed_url();

/**
 * @brief Gets the reply object from the server
 * 
 * @param resp 
 * @param socket_fd File descriptor for server TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
static int get_reply(char *resp, int socket_fd);

/**
 * @brief Sends login information to server
 * 
 * @param user User name credential
 * @param password Password credential
 * @param socket_fd File descriptor for server TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
int login(char *user, char *password, int socket_fd);

/**
 * @brief Sets the passive mode object
 * 
 * @param socket_fd File descriptor for server TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
int set_passive_mode(int socket_fd);

/**
 * @brief Sends RETR command to server
 * 
 * @param socket_fd File descriptor for server TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
int send_retr(int socket_fd);

/**
 * @brief Downloads the requested file
 * 
 * @param socket_fd File descriptor for server TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
int download(int socket_fd, int *socket_client);

/**
 * @brief Disconnects the used TCP sockets
 * 
 * @param socket_fd File descriptor for server TCP socket
 * @param socket_client File descriptor for client TCP socket
 * @return int Returns 0 on success, 1 otherwise
 */
int disconnect_sockets(int socket_fd, int socket_client);
