/*Non-Canonical Input Processing*/

#include "application.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int used_bytes = 0;
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
    flag = RECEIVER;

  if (strcmp("transmitter", argv[2]) == 0)
    flag = TRANSMITTER;

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
    //write_data(fd);
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
  char control_packet[BUFFER_SIZE];
  //int n_packets = ceil(size / sizeof(char));

  control_packet[0] = START; // start
  control_packet[1] = 0;     // type => size of file

  used_bytes = size / MAX_NUM_BYTE; //
  printf("UB: %d\n", used_bytes);

  for (int i = 0; i < used_bytes; i++)
  {
    control_packet[3 + i] = MAX_NUM_BYTE; // value
  }

  if (size % MAX_NUM_BYTE != 0)
  {
    control_packet[3 + used_bytes] = size % MAX_NUM_BYTE; // value
    used_bytes++;
  }

  printf("UB: %d\n", used_bytes);

  control_packet[2] = used_bytes; // length

  llwrite(fd, control_packet, used_bytes + 3);

  write_data(fd);
  return 0;
}

int start_reading(int fd)
{
  char buf[BUFFER_SIZE];
  llread(fd, buf); // 0 0 44 ...

  if (buf[0] == START)
  {
    if (buf[1] == 0) // types => size of file
    {
      read_data(fd, buf[2]);
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
  char buf[BUFFER_SIZE];
  unsigned int read; // bytes read
  FILE *file = fopen("pinguimCopia.gif", "a");

  for (int i = 0; i < 2 * cycles; i++)
  {
    read = llread(fd, buf);
    fwrite(buf, sizeof(char), read, file);
  }

  fclose(file);

  return 0;
}

int write_data(int fd)
{
  //char data_packet[BUFFER_SIZE];
  char buf[128];
  //data_packet[0] = 1; // data packet
  printf("used bytes : %d\n", used_bytes);
  for (int i = 0; i < 2 * used_bytes; i++)
  {
    fread(buf, sizeof(char), 128, file);
    printf("%d  %d:\n", fd, i);
    //memcpy(buf,"ola",4);
    llwrite(fd, buf, 128);
  }
  printf("bleh\n\n");

  return 0;
}
