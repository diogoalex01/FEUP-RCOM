/*Non-Canonical Input Processing*/

#include "application.h"

FILE *file;

int main(int argc, char **argv)
{
  struct termios oldtio, newtio;
  //char buf[BUFFER_SIZE];
  int fd, flag;

  if ((argc < 3) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS2", argv[1]) != 0)))
  {
    printf("Usage:\tprogram serial_port role\n\tex: nserial /dev/ttyS1 transmitter OR receiver\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("--------------------------\n");
  printf("New termios structure set.\n\n");

  // 0 - RECEIVER, 1 - TRANSMITTER
  if (strcmp("receiver", argv[2]) == 0)
  {
    flag = RECEIVER;
  }
  else if (strcmp("transmitter", argv[2]) == 0)
  {
    flag = TRANSMITTER;
  }
  else
  {
    printf("**Not a valid role!**\n");
    return 1;
  }

  if (llopen(fd, flag) == 0)
    return 1;

  struct stat file_info;
  char *filename;

  if (flag == TRANSMITTER)
  {
    filename = "pinguim.gif";
    file = fopen(filename, "rb");
    stat(filename, &file_info);
    start_writting(fd, &file_info);
    fclose(file);
  }

  if (flag == RECEIVER)
  {
    start_reading(fd);
  }

  if (llclose(fd) < 0)
  {
    return 1;
  }
  printf("llclosin\n\n");

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);

  return 0;
}

int start_writting(int fd, struct stat *file_info)
{
  off_t size = file_info->st_size;
  unsigned char control_packet[BUFFER_SIZE];
  //int n_packets = ceil(size / sizeof(char));

  control_packet[0] = START; // start

  // Sets file size information
  control_packet[1] = 0;            // type => size of file
  control_packet[2] = sizeof(size); // length
  int i = 0;
  for (; i < control_packet[2]; i++)
  {
    control_packet[3 + i] = (size >> (i * 8)) & 0xFF; // from the LSB to MSB
    printf("size e: %x\n", control_packet[3 + i]);
  }

  int used_bytes = size / MAX_DATA;
  if (size % MAX_DATA != 0)
    used_bytes++;

  printf("UB: %d\n", used_bytes);

  llwrite(fd, control_packet, 3 + i);

  write_data(fd, used_bytes);
  return 0;
}

int start_reading(int fd)
{
  unsigned char buf[BUFFER_SIZE];
  off_t file_size = 0;
  //char *file_name;

  llread(fd, buf);

  if (buf[0] == START)
  {
    if (buf[1] == 0) // types => size of file
    {

      printf("buf 2 : %d\n", buf[2]);

      for (int i = 0; i < buf[2]; i++)
      {
        printf("buf[3+%d]: %x\n", i, buf[3 + i]);
        file_size |= buf[3 + i] << (i * 8); // from the LSB to MSB
        printf("file_size : %ld\n", file_size);
      }
      //file_size = 0xd8;       // from the LSB to MSB
      //file_size |= 0x2a << 8; // from the LSB to MSB

      printf("file_size : %ld\n", file_size);
      int used_bytes = file_size / MAX_DATA;
      if (file_size % MAX_DATA != 0)
        used_bytes++;

      printf("UB: %d\n", used_bytes);

      read_data(fd, used_bytes);
    }
  }
  else
  {
    return 1;
  }

  return 0;
}

int read_data(int fd, int cycles)
{
  unsigned char buf[BUFFER_SIZE], data_buffer[BUFFER_SIZE];
  unsigned int number_bytes; // bytes read
  FILE *file = fopen("pinguimCopia.gif", "a");
  int counter = 0;

  for (int i = 0; i < 2 * cycles; i++)
  {
    printf("reading\n");
    llread(fd, buf);
    printf("cycles = %d buf[1]= %d counter = %d\n", cycles, buf[1], counter % 255);
    number_bytes = 255 * buf[2] + buf[3];

    printf("L2 %d \t L1 %d\t number of bytes %d\n", buf[2], buf[3], number_bytes);

    printf("1\n");
    for (unsigned int j = 0; j < number_bytes; j++)
    {
      data_buffer[j] = buf[4 + j];
    }
    printf("2\n");
    fwrite(data_buffer, sizeof(char), number_bytes, file);
    printf("3\n");
    counter++;
  }

  fclose(file);

  return 0;
}

int write_data(int fd, int used_bytes)
{
  unsigned char data_packet[BUFFER_SIZE];
  unsigned char buf[MAX_DATA / 2];

  for (int i = 0; i < 2 * used_bytes; i++)
  {
    printf("1-r\n");
    data_packet[0] = 1; // data packet
    data_packet[1] = i % MAX_NUM_BYTE;
    data_packet[2] = 0;
    data_packet[3] = (MAX_DATA / 2) % 255;

    fread(buf, sizeof(char), MAX_DATA / 2, file);
    printf("Packet Number %d:\n", i);

    printf("2-r\n");
    for (int j = 0; j < MAX_DATA / 2; j++)
    {
      data_packet[4 + j] = buf[j];
    }

    printf("3-r\n");
    llwrite(fd, data_packet, MAX_DATA);
    printf("4-r\n");
  }
  printf("bleh\n\n");

  return 0;
}
