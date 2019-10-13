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
  BCC2_OK
} states_t;

/* prototypes */
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

/**
 * @brief Handler function for alarm signal
 * 
 */
void llopen_alarm_handler();

/**
 * @brief 
 * 
 */
void send();