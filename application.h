#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <math.h>

#include "ll.h"

#define DATA 1
#define START 2
#define END 3

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
 * @param size
 * @return int
 */
int read_data(int fd, int cycles);

/**
 * @brief
 *
 * @param fd
 * @return int
 */
int write_data(int fd);
