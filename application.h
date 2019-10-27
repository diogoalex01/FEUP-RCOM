#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <math.h>

#include "ll.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define DATA 1
#define START 2
#define END 3
#define MAX_NUM_BYTE 255
#define MAX_DATA 256

/**
 * @brief
 *
 * @param fd
 * @param file_info
 * @return int
 */
int start_writting(int fd, struct stat *file_info);

/**
 * @brief
 *
 *
 * @param fd
 * @return int
 */
int start_reading(int fd);

/**
 * @brief
 *
 * @param fd
 * @param cycles
 * @return int
 */
int read_data(int fd, int cycles);

/**
 * @brief
 *
 * @param fd
 * @param used_bytes
 * @return int
 */
int write_data(int fd, int used_bytes);
