// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_SIZE 5

#define FLAG 0x7E
#define ADDRESS_SEND 0x03
#define CONTROL_SEND 0x03

#define ADDRESS_RECEIVE 0x01
#define CONTROL_RECEIVE 0x07

int alarmEnabled = FALSE;
int alarmCount = 0;


// Alarm function handler
void alarmHandler(int signal) {
   alarmEnabled = FALSE;
   alarmCount++;

   printf("Alarm #%d\n", alarmCount);
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) { return -1;}

    switch (connectionParameters.role) {

        case LlTx: //WRITE
            //WRITE ESCREVE
            printf("write vai escrever?\n");
            unsigned char bufW[BUF_SIZE];
  
            unsigned char BCC1W = ADDRESS_SEND ^ CONTROL_SEND;
            bufW[0] = FLAG;
            bufW[1] = ADDRESS_SEND; //0X03
            bufW[2] = CONTROL_SEND; //0X03
            bufW[3] = BCC1W;
            bufW[4] = FLAG;
            printf("write preparou o buffer\n");
            printf("flag: 0x%02X\n", bufW[0]);
            printf("address: 0x%02X\n", bufW[1]);
            printf("control: 0x%02X\n", bufW[2]);
            printf("bcc: 0x%02X\n", bufW[3]);
            printf("flag: 0x%02X\n", bufW[4]);
            int bytesW = write(fd, bufW, BUF_SIZE);
            printf("%d bytes written\n", bytesW);

            //WRITE RECEBE DE VOLTA
        
            // Wait until all bytes have been written to the serial port
            sleep(10);
            (void)signal(SIGALRM, alarmHandler);

            // Loop for input
            unsigned char bufW2[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

            state_t stateW = START_STATE;
            int a_prov1 = 0;
            int c_prov1 = 0;

            while(stateW != STOP_STATE && alarmCount < connectionParameters.nRetransmissions) {

                if (alarmEnabled == FALSE) {
                    printf("alarme do write\n");
                    alarm(connectionParameters.timeout); // Set alarm to be triggered in 3s
                    alarmEnabled = TRUE;
                }

                unsigned char byte = 0;

                read(fd, &byte, 1);

                switch (stateW) {
                case START_STATE:
                    if(byte == FLAG) {
                        bufW2[0] = byte;
                        stateW = FLAG_STATE;
                    }
                    break;
                case FLAG_STATE:
                    if(byte == FLAG) {
                        bufW2[0] = byte;
                    } else if (byte == ADDRESS_RECEIVE) {
                        bufW2[1] = byte;
                        a_prov1 = byte;
                        stateW = A_STATE;
                    } else {
                        stateW = START_STATE;
                    }
                    break;
                case A_STATE:
                    if(byte == FLAG) {
                        bufW2[0] = byte;
                        stateW = FLAG_STATE;
                    } else if (byte == CONTROL_RECEIVE) {
                        bufW2[2] = byte;
                        c_prov1 = byte;
                        stateW = C_STATE;
                    } else {
                        stateW = START_STATE;
                    }
                    break;
                case C_STATE:
                    if(byte == FLAG) {
                        bufW2[0] = byte;
                        stateW = FLAG_STATE;
                    } else if (byte == (a_prov1 ^ c_prov1)) {
                        bufW2[3] = byte;
                        stateW = BCC_STATE;
                    } else {
                        stateW = START_STATE;
                    }
                    break;
                case BCC_STATE:
                    if(byte == FLAG) {
                        bufW2[4] = byte;
                        stateW = STOP_STATE;
                        alarm(0);
                    } else {
                        stateW = START_STATE;
                    }
                    break;
                default:
                    break;
                }
            }

            for (int i = 0; i < 5; i++){
                printf("var = 0x%02X\n", bufW2[i]);
            } 
            
            if (stateW != STOP_STATE) { return -1;}

            break;

        case LlRx: //READ
            // READ RECEBE

            (void)signal(SIGALRM, alarmHandler);

            // Loop for input
            unsigned char bufR[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

            state_t stateR = START_STATE;
            int a_prov2 = 0;
            int c_prov2 = 0;

            while(stateR != STOP_STATE && alarmCount < connectionParameters.nRetransmissions) {

                if (alarmEnabled == FALSE) {
                    printf("alarme do read\n");
                    alarm(connectionParameters.timeout); // Set alarm to be triggered in 3s
                    alarmEnabled = TRUE;
                }
                sleep(1);
                unsigned char byte = 0;

                read(fd, &byte, 1);
                
                switch (stateR) {
                case START_STATE:
                    if(byte == FLAG) {
                        bufR[0] = byte;
                        stateR = FLAG_STATE;
                    }
                    break;
                case FLAG_STATE:
                    if(byte == FLAG) {
                        bufR[0] = byte;
                    } else if (byte == ADDRESS_SEND) {
                        bufR[1] = byte;
                        a_prov2 = byte;
                        stateR = A_STATE;
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case A_STATE:
                    if(byte == FLAG) {
                        bufR[0] = byte;
                        stateR = FLAG_STATE;
                    } else if (byte == CONTROL_SEND) {
                        bufR[2] = byte;
                        c_prov2 = byte;
                        stateR = C_STATE;
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case C_STATE:
                    if(byte == FLAG) {
                        bufR[0] = byte;
                        stateR = FLAG_STATE;
                    } else if (byte == (a_prov2 ^ c_prov2)) {
                        bufR[3] = byte;
                        stateR = BCC_STATE;
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case BCC_STATE:
                    if(byte == FLAG) {
                        bufR[4] = byte;
                        stateR = STOP_STATE;
                        alarm(0);
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                default:
                    break;
                }
            }

            for (int i = 0; i < 5; i++){
                printf("var = 0x%02X\n", bufR[i]);
            } 

            if (stateR != STOP_STATE) { return -1;}

            //READ RESPONDE DE VOLTA
            // Create string to send
            unsigned char bufR2[BUF_SIZE];
            unsigned char BCC1R = ADDRESS_RECEIVE ^ CONTROL_RECEIVE;
            bufR2[0] = FLAG;
            bufR2[1] = ADDRESS_RECEIVE; //0X01
            bufR2[2] = CONTROL_RECEIVE; //0X07
            bufR2[3] = BCC1R;
            bufR2[4] = FLAG;
            printf("read preparou o buffer: \n");
            printf("flag: 0x%02X\n", bufR2[0]);
            printf("address: 0x%02X\n", bufR2[1]);
            printf("control: 0x%02X\n", bufR2[2]);
            printf("bcc: 0x%02X\n", bufR2[3]);
            printf("flag: 0x%02X\n", bufR2[4]);
            int bytesR = write(fd, bufR2, BUF_SIZE);
            printf("%d bytes written\n", bytesR);    
            
            break;
            
        default:
            printf("error: role unknown");
            return -1;
    }
    
    return fd;

}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
    
    unsigned int sizeOfFrame = bufSize + 6; // 6 = 2*FLAG + ADDRESS + CONTROL + BCC1 + BCC2
    unsigned char *frame = (unsigned char *) malloc(sizeOfFrame);
    frame[0] = FLAG; //flag
    frame[1] = ADDRESS_SEND; //A = 0X03
    frame[2] = CONTROL_SEND; //C = 0X03
    frame[3] = ADDRESS_SEND ^ CONTROL_SEND; // A^C
    
    memcpy (frame+4, buf, bufSize); //D1 Dn
    
    //TODO


    unsigned char BCC2 = buf[0];
    for (unsigned int i = 1 ; i < bufSize ; i++) { BCC2 = BCC2 ^ buf[i]; } // BCC2 = D1^D2^...^Dn



   //TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
