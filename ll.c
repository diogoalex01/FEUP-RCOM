#include "ll.h"

unsigned char BCC, C;
int alarm_flag = 1, conn_attempts = 1, fd;

int llopen(int port, int flag)
{
    states_t state = START;
    int bytes_read;
    int i = 1;
    char buf[BUFFER_SIZE];
    unsigned char received_A_value, expected_C_value;
    fd = port; //TODO: perguntar ao prof o que acha de extern

    (void)signal(SIGALRM, llopen_alarm_handler);

    /* 
        First connection attempt
    */

    if (flag == TRANSMITTER)
    {
        C = 0x03;
        expected_C_value = 0x07;
        printf("Sending set-up frame... \n");
        send();
        alarm(3); // 3s alarm
        printf("Awaiting acknowledgment...\n\n");
    }
    else
    {
        C = 0x07;
        expected_C_value = 0x03;
    }

    BCC = A ^ C;

    while (state != STOP_SM)
    {
        bytes_read = read(port, buf, 1);
        printf("%d: read '%x', %d byte.\n", i++, *buf, bytes_read);
        switch (state)
        {
        case START:
            if (*buf == FLAG)
            {
                state++;
            }
            break;

        case FLAG_RCV:
            if (*buf == FLAG)
            {
                break;
            }
            else if (*buf == A)
            {
                received_A_value = *buf;
                state++;
            }
            else
            {
                state--;
            }
            break;

        case A_RCV:
            if (*buf == FLAG)
            {
                state--;
            }
            else if (*buf == expected_C_value)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            printf("buf: %x -- A_r: %x -- C_e: %x\n", *buf, received_A_value, expected_C_value);
            if (*buf == FLAG)
            {
                state = FLAG_RCV;
            }
            else if ((received_A_value ^ expected_C_value) == *buf)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case BCC_OK:
            if (*buf == FLAG)
            {
                state = STOP_SM;
                if (flag == TRANSMITTER)
                {
                    alarm_flag = 0;
                    printf("\nAcknowledgment received.\n");
                }
                else
                {
                    printf("\nSet-up frame received.\n");
                }
            }
            else
            {
                state = START;
            }
            break;

        default:
            printf("default \n");
            break;
        }
    }

    if (flag == RECEIVER)
    {
        printf("Sending acknowledgment... ");
        send();
        sleep(1);
    }

    return 0;
}

int llwrite(int fd, char *buffer, int length)
{
    int bytes_written;
    unsigned char iFrame[FRAME_SIZE];

    // fill the frame
    iFrame[0] = FLAG;
    iFrame[1] = A;
    iFrame[2] = C;
    iFrame[3] = BCC; /////
    iFrame[4] = FLAG;
    printf("%x", iFrame[0]);

    bytes_written = write(fd, buffer, length);
    printf("%d bytes sent.\n\n", bytes_written);

    return 0;
}

void llopen_alarm_handler()
{
    if (!alarm_flag)
    {
        return;
    }

    if (conn_attempts < 3)
    {
        printf("Unsuccessful! New attempt... ");
        send();
        alarm(3);
        conn_attempts++;
        printf("Awaiting acknowledgment...\n\n");
    }
    else
    {
        printf("Connection Timed Out!\n");
        exit(1);
    }
}

void send()
{
    int bytes_written;
    unsigned char frame[FRAME_SIZE];

    // fill the frame
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = BCC;
    frame[4] = FLAG;

    bytes_written = write(fd, frame, FRAME_SIZE);
    printf("%d bytes sent.\n\n", bytes_written);
}
