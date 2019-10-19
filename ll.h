#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* MACROS */
// ========================================================= //

#define RECEIVER 0
#define TRANSMITTER 1

#define FRAME_SIZE 5
#define BUFFER_SIZE 255
#define READ_BUFFER_SIZE 20
#define FLAG 0x7E
#define ESCAPE 0x7D
#define OCT 0x20
#define A 0x03

/* enums */
// ========================================================= //

typedef enum
{
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC_OK,
  STOP_SM,
  DATA,
  BCC2_OK,
  STUFF
} states_t;

/* prototypes */
// ========================================================= //

/* protocol */
// ========================================================= //

/**
 * @brief 
 * 
 * @param port 
 * @param flag 
 * @return int 
 */
int llopen(int port, int flag);

/**
 * @brief 
 * 
 * @param fd 
 * @param buffer 
 * @param length 
 * @return int 
 */
int llwrite(int fd, char *buffer, int length);

/**
 * @brief
 * @param fd 
 * @param buffer
 * @return int
 */
int llread(int fd, char *buffer);

/* utility functions */
// ========================================================= //

/**
 * @brief 
 * 
 */
void send();

/**
 * @brief 
 * 
 * @return int 
 */
int send_w();

/**
 * @brief 
 * 
 */
void llwrite_alarm_handler();

/**
 * @brief 
 * 
 */
void llopen_alarm_handler();
