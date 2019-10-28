/*Non-Canonical Input Processing*/

#include "application.h"

FILE *file;
int used_bytes;

int main(int argc, char **argv)
{
  struct termios oldtio, newtio;
  //char buf[BUFFER_SIZE];
  int fd, flag;

  if ((argc < 3) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0)))
  {
    printf("[TRANSMITTER] Usage:\tprogram serial_port role file_name\n\tex: nserial /dev/ttyS1 transmitter image.png\n");
    printf("[RECEIVER] Usage:\tprogram serial_port role\n\tex: nserial /dev/ttyS1 receiver\n");
    exit(1);
  }

  // 0 - RECEIVER, 1 - TRANSMITTER
  if (strcmp("receiver", argv[2]) == 0)
  {
    flag = RECEIVER;
  }
  else if (strcmp("transmitter", argv[2]) == 0)
  {
    if (argc < 4)
    {
      printf("[TRANSMITTER] Usage:\tprogram serial_port role file_name\n\tex: nserial /dev/ttyS1 transmitter image.png\n");
      return 1;
    }
    else
    {
      flag = TRANSMITTER;
    }
  }
  else
  {
    printf("** Not a valid role! **\n");
    return 1;
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

  if (llopen(fd, flag) == 0)
    return 1;

  struct stat file_info;
  char *file_name;

  if (flag == TRANSMITTER)
  {
    file_name = (char *)malloc(strlen(argv[3]));
    memcpy(file_name, argv[3], strlen(argv[3]));
    printf("%s \t %s\n", argv[3], file_name);
    file = fopen(file_name, "rb");
    stat(file_name, &file_info);
    start_writting(fd, &file_info, file_name);
    fclose(file);
  }

  if (flag == RECEIVER)
  {
    start_reading(fd);
  }

  if (llclose(fd) < 0)
    return 1;

  printf("llclosin\n\n");

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);

  return 0;
}

int start_writting(int fd, struct stat *file_info, char *file_name)
{
  off_t size = file_info->st_size;
  unsigned char control_packet[BUFFER_SIZE];
  int packet_size;

  if ((packet_size = send_start_packet(fd, control_packet, size, file_name)) < 0)
    return 1;

  // Sends data packets
  write_data(fd, used_bytes);

  //Sends end control packet
  send_end_packet(fd, control_packet, packet_size);

  return 0;
}

int start_reading(int fd)
{
  unsigned char buf[BUFFER_SIZE];
  off_t file_size = 0;
  char *file_name;

  // Gets start control packet
  llread(fd, buf);

  if (buf[PACKET_TYPE_INDEX] != START_TYPE)
    return 1; //TODO define what error means i.e pos vs neg

  // Gets file size information
  if (buf[SIZE_TYPE_INDEX] != SIZE_TYPE)
    return 1;

  for (int i = 0; i < buf[SIZE_LENGTH_INDEX]; i++)
  {
    file_size |= buf[SIZE_VALUE_INDEX + i] << (i * BITS_IN_BYTE); // from the LSB to MSB
  }

  printf("File size : %ld\n", file_size);
  used_bytes = file_size / MAX_DATA; //int used_bytes
  if (file_size % MAX_DATA != 0)
    used_bytes++;

  // Gets file name information
  if (buf[NAME_TYPE_INDEX + buf[SIZE_LENGTH_INDEX]] != NAME_TYPE)
    return 1;

  file_name = (char *)malloc(buf[buf[SIZE_LENGTH_INDEX] + NAME_LENGTH_INDEX]);

  for (int i = 0; i < buf[NAME_LENGTH_INDEX + buf[SIZE_LENGTH_INDEX]]; i++)
  {
    //printf("2\n");
    file_name[i] = buf[NAME_VALUE_INDEX + buf[SIZE_LENGTH_INDEX] + i];
  }

  printf("File name: %s %ld\n", file_name, strlen(file_name));

  read_data(fd, used_bytes, file_name);

  llread(fd, buf);

  if (buf[0] != END_TYPE)
  {
    printf("** End control packet not received! **\n");
    return 1;
  }
  else
  {
    printf("Closing connection\n");
  }

  free(file_name);
  return 0;
}

int read_data(int fd, int cycles, char *file_name)
{
  unsigned char buf[BUFFER_SIZE], data_buffer[BUFFER_SIZE];
  unsigned int number_bytes; // bytes read
  char copy_name[BUFFER_SIZE];

  //TODO
  if (strcmp(file_name, "robot.png") != 0) // pinguim.gif
  {
    sprintf(copy_name, "wrong_name.png");
  }
  else
  {
    sprintf(copy_name, "pinguim_copy.png");
  }

  FILE *file = fopen(copy_name, "a");
  int counter = 0;

  for (int i = 0; i < 2 * cycles; i++)
  {
    //printf("reading\n");
    llread(fd, buf);

    number_bytes = 255 * buf[2] + buf[3];

    for (unsigned int j = 0; j < number_bytes; j++)
    {
      data_buffer[j] = buf[4 + j];
    }

    fwrite(data_buffer, sizeof(char), number_bytes, file);

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
    data_packet[0] = 1; // data packet
    data_packet[1] = i % MAX_NUM_BYTE;
    data_packet[2] = (MAX_DATA / 2) / MAX_NUM_BYTE;
    data_packet[3] = (MAX_DATA / 2) % MAX_NUM_BYTE;

    fread(buf, sizeof(char), MAX_DATA / 2, file);
    show_transf_progress(i, 2 * used_bytes - 1);

    for (int j = 0; j < MAX_DATA / 2; j++)
    {
      data_packet[4 + j] = buf[j];
    }

    llwrite(fd, data_packet, MAX_DATA);
  }

  printf("\n");

  return 0;
}

int send_start_packet(int fd, unsigned char *control_packet, off_t size, char *file_name)
{
  // Sets file size information
  control_packet[PACKET_TYPE_INDEX] = START_TYPE;   // start
  control_packet[SIZE_TYPE_INDEX] = SIZE_TYPE;      // type => size of file
  control_packet[SIZE_LENGTH_INDEX] = sizeof(size); // length
  int i = 0;

  for (; i < control_packet[SIZE_LENGTH_INDEX]; i++)
  {
    control_packet[SIZE_VALUE_INDEX + i] = (size >> (i * BITS_IN_BYTE)) & 0xFF; // from the LSB to MSB
  }

  used_bytes = size / MAX_DATA;
  if (size % MAX_DATA != 0)
    used_bytes++;

  // Sets file name information
  control_packet[NAME_TYPE_INDEX + control_packet[SIZE_LENGTH_INDEX]] = NAME_TYPE;
  control_packet[NAME_LENGTH_INDEX + control_packet[SIZE_LENGTH_INDEX]] = strlen(file_name);
  int j = 0;

  for (; j < control_packet[NAME_LENGTH_INDEX + control_packet[SIZE_LENGTH_INDEX]]; j++)
  {
    control_packet[NAME_VALUE_INDEX + control_packet[SIZE_LENGTH_INDEX] + j] = file_name[j];
  }

  if (llwrite(fd, control_packet, NAME_VALUE_INDEX + control_packet[SIZE_LENGTH_INDEX] + j) < 0)
    return -1;
  else
    return NAME_VALUE_INDEX + control_packet[SIZE_LENGTH_INDEX] + j;
}

int send_end_packet(int fd, unsigned char *control_packet, int packet_size)
{
  control_packet[PACKET_TYPE_INDEX] = END_TYPE; // end control packet

  if (llwrite(fd, control_packet, packet_size) < 0)
    return 1;
  else
    return 0;
}

void show_transf_progress(int packet_number, int total_packets)
{
  float percentage = (100 * packet_number) / (float)total_packets;

  printf("[Packet %d of %d] %3.2f%% \r", packet_number, total_packets, percentage);
  fflush(stdout);
}