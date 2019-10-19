#include "ll.h"

/* global variables */
// ========================================================= //

unsigned char BCC, C = 0x00, expected_C_value = 0x85;
int alarm_flag = 1, conn_attempts = 1, fd, bytes_to_write;
char *write_buffer;

/* protocol */
// ========================================================= //

int llopen(int port, int flag)
{
    states_t state = START;
    int bytes_read;
    int i = 1;
    char buf[BUFFER_SIZE];
    unsigned char received_A_value, expected_C_value;
    fd = port;
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

    C = 0x00;

    return 0;
}

int llwrite(int fdesc, char *buffer, int length)
{
    bytes_to_write = length;
    printf("%d\n", bytes_to_write);
    write_buffer = buffer;
    fd = fdesc;

    states_t state = START;
    unsigned char received_A_value, rr_buf[BUFFER_SIZE];

    if (C == 0x00)
    {
        expected_C_value = 0x85;
    }
    else if (C == 0x40)
    {
        expected_C_value = 0x05;
    }

    (void)signal(SIGALRM, llwrite_alarm_handler);
    alarm_flag = 1;
    conn_attempts = 0;

    send_w();
    alarm(3);

    while (state != STOP_SM && conn_attempts < 3)
    {
        printf("Connection atmps %d \n", conn_attempts);
        read(fd, rr_buf, 1);
        printf("Buffer %x\n", *rr_buf);
        switch (state)
        {
        case START:
            printf("start\n");
            if (*rr_buf == FLAG)
            {
                state++;
            }
            break;

        case FLAG_RCV:
            printf("fl  \n");
            if (*rr_buf == FLAG)
            {
                break;
            }
            else if (*rr_buf == A)
            {
                received_A_value = *rr_buf;
                state++;
            }
            else
            {
                state--;
            }
            break;

        case A_RCV:
            printf("A\n");
            printf("expected C%x\n", expected_C_value);
            if (*rr_buf == FLAG)
            {
                state--;
            }
            else if (*rr_buf == expected_C_value)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            printf("c\n");

            if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
            }
            else if ((received_A_value ^ expected_C_value) == *rr_buf)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case BCC_OK:
            printf("BCC\n");

            if (*rr_buf == FLAG)
            {
                state = STOP_SM;
                alarm_flag = 0;
                C ^= (1 << 6);
                printf("\nReceiver ready. C = %c\n", C);
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

    sleep(2);

    return 0;
}

int llread(int fd, char *buffer)
{
    states_t state = START;
    unsigned char received_A_value = 0x03, BCC2_calc;
    unsigned char rr_buf[BUFFER_SIZE];
    int dn = 0;

    while (state != STOP_SM)
    {
        read(fd, rr_buf, 1);
        switch (state)
        {
        case START:
            printf("start\n");
            if (*rr_buf == FLAG)
            {
                state++;
            }
            break;

        case FLAG_RCV:
            printf("flag\n");
            if (*rr_buf == FLAG)
            {
                break;
            }
            else if (*rr_buf == A)
            {
                received_A_value = *rr_buf;
                state++;
            }
            else
            {
                state--;
            }
            break;

        case A_RCV:
            printf("A_rcv\n");
            if (*rr_buf == FLAG)
            {
                state--;
            }
            else if (*rr_buf == C)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            printf("C rcv\n");
            if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
            }
            else if ((received_A_value ^ C) == *rr_buf)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case BCC_OK:
            printf("bcc1_ok\n");
            if (*rr_buf == ESCAPE)
            {
                state = STUFF;
                break;
            }
            buffer[dn++] = *rr_buf;
            BCC2_calc = *rr_buf;
            state = DATA;
            break;

        case DATA:
            printf("data\t bcc2: %x\n", BCC2_calc);
            if (*rr_buf == BCC2_calc)
            {
                state = BCC2_OK;
            }
            else if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
                dn = 0;
            }
            else if (*rr_buf == ESCAPE)
            {
                state = STUFF;
            }
            else
            {
                BCC2_calc ^= *rr_buf;
                buffer[dn++] = *rr_buf;
            }
            break;

        case BCC2_OK:
            printf("bcc2\n");
            if (*rr_buf == FLAG)
            {
                if (C == 0x00)
                {
                    C = 0x85;
                }
                else if (C == 0x40)
                {
                    C = 0x05;
                }
                state = STOP_SM;
            }
            else
            {
                buffer[dn++] = BCC2_calc;
                BCC2_calc = 0;

                if (*rr_buf == ESCAPE)
                {
                    state = STUFF;
                    break;
                }

                if (*rr_buf != BCC2_calc)
                {
                    buffer[dn++] = *rr_buf;
                    BCC2_calc ^= *rr_buf;
                    state = DATA;
                }
            }
            break;

        case STUFF:
            printf("stuff\n");
            if (*rr_buf == 0x5d) // 0x5D = ESCAPE ^ OCT
            {
                if (ESCAPE == BCC2_calc)
                {
                    state = BCC2_OK;
                }
                else
                {
                    buffer[dn++] = ESCAPE;
                    BCC2_calc ^= ESCAPE;
                    state = DATA;
                }
            }
            else if (*rr_buf == 0x5e) // 0x5E = FLAG ^ OCT
            {
                if (FLAG == BCC2_calc)
                {
                    state = BCC2_OK;
                }
                else
                {
                    buffer[dn++] = FLAG;
                    BCC2_calc ^= FLAG;
                    state = DATA;
                }
            }
            else
            {
                state = FLAG_RCV;
            }
            break;

        default:
            printf("default \n");
            break;
        }
        
        printf(" # read '%x', bbc2 = %x.\n", *rr_buf, BCC2_calc);
    }

    send();

    printf("%s\n", buffer);

    return dn;
}

/* utility functions */
// ========================================================= //

void send()
{
    int bytes_written;
    unsigned char frame[FRAME_SIZE];
    BCC = A ^ C;

    // fill the frame
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = BCC;
    frame[4] = FLAG;

    bytes_written = write(fd, frame, FRAME_SIZE);
    printf("%d bytes sent.\n\n", bytes_written);
}

int send_w()
{
    int bytes_written, write_index = 0;
    unsigned char iFrame[BUFFER_SIZE], BCC2, stuff;

    // fill the frame
    printf("IFRAME:\n");
    iFrame[0] = FLAG;
    printf("%x\n", iFrame[0]);
    iFrame[1] = A;
    printf("%x\n", iFrame[1]);
    iFrame[2] = C;
    printf("%x\n", iFrame[2]);
    iFrame[3] = A ^ C;
    printf("%x\n", iFrame[3]);

    BCC2 = write_buffer[0];

    printf("Bytes to write %d\n", bytes_to_write);
    for (int i = 4; i < bytes_to_write + 4; i++, write_index++)
    {
        stuff = write_buffer[write_index];
        //byte stuffing
        if (write_buffer[write_index] == FLAG || write_buffer[write_index] == ESCAPE)
        {
            iFrame[i] = ESCAPE;
            printf("%d: %x\t%d\n", i, iFrame[i], bytes_to_write);
            i++;
            iFrame[i] = stuff ^ OCT;
            printf("%d: %x\t%d\n", i, iFrame[i], bytes_to_write);
            bytes_to_write++;
        }
        else
        {
            iFrame[i] = write_buffer[write_index];
            printf("%d: %x\t%d\n", i, iFrame[i], bytes_to_write);
        }

        if (i > 4)
        {
            BCC2 = BCC2 ^ stuff;
        }
    }

    if (BCC2 == FLAG)
    {
        iFrame[bytes_to_write + 4] = ESCAPE;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 4]);
        iFrame[bytes_to_write + 5] = FLAG ^ OCT;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 5]);
        iFrame[bytes_to_write + 6] = FLAG;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 6]);
    }
    else
    {
        iFrame[bytes_to_write + 4] = BCC2;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 4]);
        iFrame[bytes_to_write + 5] = FLAG;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 5]);
    }

    bytes_written = write(fd, iFrame, bytes_to_write + 6);
    printf("%d bytes sent.\n\n", bytes_written);
    return bytes_written;
}

void llwrite_alarm_handler()
{
    if (!alarm_flag)
    {
        return;
    }

    if (conn_attempts < 3)
    {
        printf("Unsuccessful WRITE! New attempt... \n");
        send_w();
        alarm(3);
        conn_attempts++;
        printf("Awaiting acknowledgment...\n\n");
    }
    else
    {
        printf("Connection Timed Out!\n");
        return;
    }
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
