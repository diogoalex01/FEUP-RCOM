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

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

typedef struct
{
    char user[255];
    char password[255];
    char host[255];
    char url[255];
    char server[255];
} url_syntax;

/**
 * @brief 
 * 
 * @param name 
 */
void get_IP(const char *name);

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