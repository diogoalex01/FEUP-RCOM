#include "ll.h"

/* global variables */
// ========================================================= //

unsigned char BCC, C = 0x00, expected_C_value = 0x85;
int alarm_flag = 1, conn_attempts = 1, fd, bytes_to_write, return_value = 0, written = 0, last_C_received = 0x40, is_transmitter;
unsigned char *write_buffer;

/* protocol */
// ========================================================= //

int llopen(int port, int flag)
{
    states_t state = START;
    int bytes_read;
    //int i = 1;
    unsigned char buf[BUFFER_SIZE];
    unsigned char received_A_value, expected_C_value;
    fd = port;
    return_value = fd;
    (void)signal(SIGALRM, llopen_alarm_handler);

    /*
        First connection attempt
    */

    if (flag == TRANSMITTER)
    {
        C = 0x03;
        expected_C_value = 0x07;
        //printf("Sending set-up frame... \n");
        send();
        alarm(3); // 3s alarm
        //printf("Awaiting acknowledgment...\n\n");
        is_transmitter = 1;
    }
    else
    {
        C = 0x07;
        expected_C_value = 0x03;
        is_transmitter = 0;
    }

    BCC = A ^ C;

    while (state != STOP_SM)
    {
        bytes_read = read(port, buf, 1);
        //printf("%d: read '%x', %d byte.\n", i++, *buf, bytes_read);
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
            //printf("buf: %x -- A_r: %x -- C_e: %x\n", *buf, received_A_value, expected_C_value);
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
                if (is_transmitter)
                {
                    alarm_flag = 0;
                    //printf("\nAcknowledgment received.\n");
                }
                else
                {
                    //printf("\nSet-up frame received.\n");
                }
            }
            else
            {
                state = START;
            }
            break;

        default:
            //printf("default \n");
            break;
        }
    }

    if (!is_transmitter)
    {
        //printf("Sending acknowledgment... ");
        send();
        //sleep(1);
    }

    C = 0x00;

    return return_value;
}

int llwrite(int fdesc, char *buffer, int length)
{
    bytes_to_write = length;
    //printf("%d\n", bytes_to_write);
    write_buffer = buffer;
    fd = fdesc;

    states_t state = START;
    unsigned char received_A_value, rr_buf[BUFFER_SIZE], received_C_value, RR, REJ;

    if (C == 0x00)
    {
        RR = 0x85;
        REJ = 0x81;
    }
    else if (C == 0x40)
    {
        RR = 0x05;
        REJ = 0x01;
    }
    else
    {
        RR = 0x00;
        REJ = 0x00;
    }

    (void)signal(SIGALRM, llwrite_alarm_handler);
    alarm_flag = 1;
    conn_attempts = 0;

    written = send_w();
    alarm(3);

    while (state != STOP_SM) //&& conn_attempts < 3
    {
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
            if (*rr_buf == FLAG)
            {
                state--;
            }
            else if (*rr_buf == RR || *rr_buf == REJ)
            {
                received_C_value = *rr_buf;
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
            else if ((received_A_value ^ received_C_value) == *rr_buf)
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
                if (received_C_value == RR)
                {
                    state = STOP_SM;
                    alarm_flag = 0;
                    conn_attempts = 0;
                    C ^= (1 << 6);
                    //printf("\nReceiver ready. C = %c\n", C);
                }
                else if (received_C_value == REJ)
                {
                    state = START;
                    written = send_w();
                    alarm(3);
                    sleep(2);

                    //printf("REJ received, resending I frame... C = %c\n", C);
                }
            }
            else
            {
                state = START;
            }
            break;

        default:
            //printf("default \n");
            break;
        }
    }

    //sleep(1);

    return written;
}

int llread(int fd, char *buffer)
{
    states_t state = START;
    unsigned char received_A_value = 0x03, BCC2_calc = 0x00, RR, REJ, store_c;
    unsigned char rr_buf[BUFFER_SIZE];
    int dn = 0;

    if (C == 0x00)
    {
        RR = 0x85;
        REJ = 0x81;
    }
    else if (C == 0x40)
    {
        RR = 0x05;
        REJ = 0x01;
    }
    else
    {
        RR = 0x00;
        REJ = 0x00;
    }
    int rr_rec = 0;
    while (state != STOP_SM)
    {
        read(fd, rr_buf, 1);
        printf(" # read '%x', bbc2 = %x.\n", *rr_buf, BCC2_calc);
        switch (state)
        {
        case START:

            printf("start\n\n** C is %x\n", C);
            BCC2_calc = 0;
            if (*rr_buf == FLAG)
            {
                state++;
            }
            break;

        case FLAG_RCV:
            printf("\n** C is %x\n", C);
            BCC2_calc = 0;
            dn = 0;
            printf("flag\n");
            if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
            }
            else if (*rr_buf == A)
            {
                received_A_value = *rr_buf;
                state++;
            }
            else
            {
                state = START;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            break;

        case A_RCV:
            printf("A_rcv\n\n** C is %x\n", C);
            printf("C is %x and last is : %x\n", C, last_C_received);
            if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            else if (*rr_buf == C)
            {
                last_C_received = C;
                printf("last is : %x\n", last_C_received);
                state++;
            }
            else if (*rr_buf == last_C_received)
            {
                printf("RR\n");
                store_c = C;
                RR ^= (1 << 7);
                C = RR;
                send();
                C = last_C_received;
                RR ^= (1 << 7);
                state++;
                rr_rec = 1;
            }
            else
            {
                state = START;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            break;

        case C_RCV:
            printf("C rcv\n\n** C is %x\n", C);
            if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            else if ((received_A_value ^ C) == *rr_buf)
            {
                state++;
            }
            else
            {
                state = START;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            break;

        case BCC_OK:
            printf("bcc1_ok\n\n** C is %x\n", C);
            if (*rr_buf == ESCAPE)
            {
                state = STUFF;
            }
            else if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            else
            {
                buffer[dn++] = *rr_buf;
                BCC2_calc = *rr_buf;
                state = DATA;
            }
            break;

        case DATA:
            printf("DATA\n** C is %x\n", C);
            if (*rr_buf == ESCAPE)
            {
                state = STUFF;
            }

            else if (*rr_buf == FLAG)
            {
                state = FLAG_RCV;
                dn = 0;
                store_c = C;
                C = REJ;
                send();
                printf("\n SENDING REJ\n");
                C = store_c;
            }
            else if (*rr_buf == BCC2_calc)
            {
                state = BCC2_OK;
            }
            else
            {
                //printf("BCC2: %x rr_buf: %x\n", BCC2_calc, *rr_buf);
                BCC2_calc ^= *rr_buf;
                //printf("BCC2: %x \n", BCC2_calc);
                buffer[dn++] = *rr_buf;
            }
            break;

        case BCC2_OK:
            printf("bcc2\n\n** C is %x\n", C);
            if (*rr_buf == FLAG)
            {
                if (rr_rec)
                {
                    state = START;
                    C = store_c;
                    rr_rec = 0;
                    break;
                }
                store_c = C;
                C = RR;
                send();
                C = store_c;
                printf("\n\n\nC was %x\n\n\n", C);
                C ^= (1 << 6);
                printf("\n\n\nC is now %x\n\n\n", C);
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
            printf("stuff\n \n** C is %x\n", C);
            if (*rr_buf == 0x5d) // 0x5D = ESCAPE ^ OCT
            {
                if (ESCAPE == BCC2_calc)
                {
                    state = BCC2_OK;
                    printf("\n** C is %x\n", C);
                    //C = store_c;
                    //printf("faz faz faz C = %x \n", C);
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
                    printf("flag flag flag C = %x \n", C);
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
                store_c = C;
                C = REJ;
                send();
                C = store_c;
                printf("\n SENDING REJ\n");
            }
            break;

        default:
            printf("default \n");
            break;
        }
    }

    printf("%s\n", buffer);

    return dn;
}

int llclose(int fd)
{
    states_t state = START;
    unsigned char buf[BUFFER_SIZE], received_A_value;
    C = DISC;
    (void)signal(SIGALRM, llopen_alarm_handler);
    alarm_flag = 1;
    conn_attempts = 0;
    return_value = fd;

    if (is_transmitter)
    {
        send();
        alarm(3);
    }

    while (state != STOP_SM)
    {
        read(fd, buf, 1);

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
            else if (*buf == C)
            {
                state++;
            }
            else
            {
                state = START;
            }
            break;

        case C_RCV:
            //printf("buf: %x -- A_r: %x -- C_e: %x\n", *buf, received_A_value, C);
            if (*buf == FLAG)
            {
                state = FLAG_RCV;
            }
            else if ((received_A_value ^ C) == *buf)
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
                if (is_transmitter)
                {
                    alarm_flag = 0;
                    //printf("\nTransmitter received DISC sending UA\n");
                    C = UA;
                    send();
                }
                else if (!is_transmitter)
                {
                    if (C != UA)
                    {
                        alarm(3);
                        send();
                        C = UA;
                        state = START;
                        //printf("\nReceiver received DISC sending DISC\n");
                        break;
                    }
                    alarm_flag = 0;
                    //printf("\nReceiver received UA, closing serial port\n");
                }
            }
            else
            {
                state = START;
            }
            break;

        default:
            //printf("default \n");
            break;
        }
    }

    return return_value;
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
    //printf("%d bytes sent.\n\n", bytes_written);
}

int send_w()
{
    int bytes_written, write_index = 0;
    unsigned char iFrame[512], BCC2, stuff;

    // fill the frame
    //printf("IFRAME:\n");
    iFrame[0] = FLAG;
    //printf("%x\n", iFrame[0]);
    iFrame[1] = A;
    //printf("%x\n", iFrame[1]);
    iFrame[2] = C;
    //printf("%x\n", iFrame[2]);
    iFrame[3] = A ^ C;
    //printf("%x\n", iFrame[3]);

    BCC2 = write_buffer[0];

    //printf("Bytes to write %d\n", bytes_to_write);
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
            //printf("%d: %x\t%d\n", i, iFrame[i], bytes_to_write);
        }

        if (write_index > 0)
        {
            //printf("BCC2: %x\n", BCC2);
            //printf("Stuff: %X\n", stuff);
            BCC2 = BCC2 ^ stuff;
            //printf("BCC2: %x\n", BCC2);
        }
    }

    if (BCC2 == FLAG || BCC2 == ESCAPE)
    {
        iFrame[bytes_to_write + 4] = ESCAPE;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 4]);
        iFrame[bytes_to_write + 5] = BCC2 ^ OCT;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 5]);
        iFrame[bytes_to_write + 6] = FLAG;
        printf("IFRAME: %x \n", iFrame[bytes_to_write + 6]);
    }
    else
    {
        iFrame[bytes_to_write + 4] = BCC2;
        //printf("IFRAME: %x \n", iFrame[bytes_to_write + 4]);
        iFrame[bytes_to_write + 5] = FLAG;
        //printf("IFRAME: %x \n", iFrame[bytes_to_write + 5]);
    }

    bytes_written = write(fd, iFrame, bytes_to_write + 6);
    //printf("%d bytes sent.\n\n", bytes_written);
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
        printf("Unsuccessful WRITE! New attempt...Attempt %d \n", conn_attempts);
        send_w();
        alarm(3);
        conn_attempts++;
        //printf("Awaiting acknowledgment...\n\n");
    }
    else
    {
        printf("Connection Timed Out!\n");
        written = -1;
        exit(1);
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
        //printf("Unsuccessful! New attempt... ");
        send();
        alarm(3);
        conn_attempts++;
        //printf("Awaiting acknowledgment...\n\n");
    }
    else
    {
        //printf("Connection Timed Out!\n");
        return_value = -1;
        exit(1);
    }
}
