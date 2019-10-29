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
#define MAX_NUM_BYTE 255
#define MAX_DATA 256
#define BITS_IN_BYTE 8

#define SEQUENCE_NUMBER_INDEX 1
#define L2_INDEX 2
#define L1_INDEX 3
#define DATA_INDEX 4

#define PACKET_TYPE_INDEX 0
#define DATA_TYPE 1
#define START_TYPE 2
#define END_TYPE 3

#define SIZE_TYPE 0
#define SIZE_TYPE_INDEX 1
#define SIZE_LENGTH_INDEX 2
#define SIZE_VALUE_INDEX 3

#define NAME_TYPE 1
#define NAME_TYPE_INDEX 4
#define NAME_LENGTH_INDEX 5
#define NAME_VALUE_INDEX 6

/**
 * @brief 
 * 
 * @param fd 
 * @param file_info 
 * @param filename 
 * @return int 
 */
int start_writting(int fd, struct stat *file_info, char *filename);

/**
 * @brief 
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
 * @param file_name 
 * @return int 
 */
int read_data(int fd, int cycles, char *file_name);

/**
 * @brief 
 * 
 * @param fd 
 * @param used_bytes 
 * @return int 
 */
int write_data(int fd, int used_bytes);

/**
 * @brief 
 * 
 * @param fd 
 * @param control_packet 
 * @param size 
 * @param file_name 
 * @return int 
 */
int send_start_packet(int fd, unsigned char *control_packet, off_t size, char *file_name);

/**
 * @brief 
 * 
 * @param fd 
 * @param control_packet 
 * @param packet_size 
 * @return int 
 */
int send_end_packet(int fd, unsigned char *control_packet, int packet_size);

/**
 * @brief 
 * 
 * @param packet_number 
 * @param total_packets 
 */
void show_transf_progress(int packet_number, int total_packets);