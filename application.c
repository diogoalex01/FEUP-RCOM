/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include "ll.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

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

  if (llopen(fd, flag) != 0)
    return 1;

  if (flag == TRANSMITTER)
    llwrite(fd,"jonas",20); // 0x7e 0x63 0x63 0x7d => 0x7d 0x5e 0x63 0x63 0x7d 0x5d \0

  if (flag == RECEIVER)
  {
    char buf[255];
    int r = llread(fd, buf);
    printf("%d %s", r, buf);
  }

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}
