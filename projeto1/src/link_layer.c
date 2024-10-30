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
#define CONTROL_SET 0x03

#define ADDRESS_RECEIVE 0x01
#define CONTROL_UA 0x07

#define ESCAPE 0x7D
#define XOR_FLAG 0x5E
#define XOR_ESCAPE 0x5D


int alarmEnabled = FALSE;
int alarmCount = 0;

typedef enum {
    START_STATE,
    FLAG_STATE,
    A_STATE,
    C_STATE,
    BCC_STATE,
    STOP_STATE,
    PRE_STOP_STATE,
    DESTUFFING_STATE
} state_t;

LinkLayer connectionParametersCopy;

int Ns_t = 0;  // Transmiter
int Nr_r = 1; // Receiver
#define CONTROL_I_N0 0x00
#define CONTROL_I_N1 0x80
#define CONTROL_RR0  0xAA
#define CONTROL_RR1  0xAB
#define CONTROL_REJ0 0x54
#define CONTROL_REJ1 0x55

#define CONTROL_DISC 0x0B

int frameCounter = 0;
int frameCounterReceived = 0;

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
    connectionParametersCopy.role = connectionParameters.role;
    connectionParametersCopy.baudRate = connectionParameters.baudRate;
    connectionParametersCopy.nRetransmissions = connectionParameters.nRetransmissions;
    //connectionParametersCopy.serialPort = connectionParameters.serialPort;
    connectionParametersCopy.timeout = connectionParameters.timeout;

    switch (connectionParameters.role) {

        case LlTx: //WRITE
            //WRITE ESCREVE
            unsigned char bufW[BUF_SIZE];
  
            unsigned char BCC1W = ADDRESS_SEND ^ CONTROL_SET;
            bufW[0] = FLAG;
            bufW[1] = ADDRESS_SEND; //0X03
            bufW[2] = CONTROL_SET; //0X03
            bufW[3] = BCC1W;
            bufW[4] = FLAG;
            printf("------------------------------------------------------------------------\n");
            printf("Write SET (llopen):\n");
            printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
            int bytesW = writeBytesSerialPort(bufW, BUF_SIZE);
            printf("%d bytes written (SET)(llopen)\n", bytesW);
            printf("------------------------------------------------------------------------\n");
            frameCounter++;

            //WRITE RECEBE DE VOLTA
        
            // Wait until all bytes have been written to the serial port
            sleep(5);
            (void)signal(SIGALRM, alarmHandler);

            // Loop for input
            unsigned char bufW2[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

            state_t stateW = START_STATE;
            int a_prov1 = 0;
            int c_prov1 = 0;

            while(stateW != STOP_STATE && alarmCount < connectionParameters.nRetransmissions) {

                if (alarmEnabled == FALSE) {
                    if(alarmCount != 0){
                        unsigned char bufW[BUF_SIZE];

                        unsigned char BCC1W = ADDRESS_SEND ^ CONTROL_SET;
                        bufW[0] = FLAG;
                        bufW[1] = ADDRESS_SEND; //0X03
                        bufW[2] = CONTROL_SET; //0X03
                        bufW[3] = BCC1W;
                        bufW[4] = FLAG;
                        printf("------------------------------------------------------------------------\n");
                        printf("Write SET again (llopen)\n");
                        printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
                        int bytesW = writeBytesSerialPort(bufW, BUF_SIZE);
                        printf("%d bytes written (SET)(llopen)\n", bytesW);
                        printf("------------------------------------------------------------------------\n");
                        frameCounter++;
                    }
                    
                    //printf("alarme do write\n");
                    alarm(connectionParameters.timeout); // Set alarm to be triggered in Xs
                    alarmEnabled = TRUE;
                    
                }

                unsigned char byte;

                if(readByteSerialPort(&byte) > 0){
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
                            } else if (byte == CONTROL_UA) {
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
                                frameCounterReceived++;
                                alarm(0);
                            } else {
                                stateW = START_STATE;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
            
            printf("------------------------------------------------------------------------\n");
            printf("Collected from Read (llopen): ");
            for (int i = 0; i < 5; i++){
                printf("0x%02X |", bufW2[i]);
            }
            printf("\n"); 
            printf("------------------------------------------------------------------------\n");
            
            alarmEnabled = FALSE;
            alarmCount = 0;
            if (stateW != STOP_STATE) { return -1;}

            break;

        case LlRx: //READ
            // READ RECEBE
            // Loop for input
            unsigned char bufR[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

            state_t stateR = START_STATE;
            int a_prov2 = 0;
            int c_prov2 = 0;

            while(stateR != STOP_STATE) {
                unsigned char byte;

                if(readByteSerialPort(&byte)){
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
                            } else if (byte == CONTROL_SET) {
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
                                frameCounterReceived++;
                            } else {
                                stateR = START_STATE;
                            }
                            break;
                        default:
                            break;
                        }
                }
                
            }

            printf("------------------------------------------------------------------------\n");
            printf("Collected from Write (llopen) : ");
            for (int i = 0; i < 5; i++){
                printf("0x%02X |", bufR[i]);
            }
            printf("\n");  
            printf("------------------------------------------------------------------------\n");

            //READ RESPONDE DE VOLTA
            // Create string to send
            unsigned char bufR2[BUF_SIZE];
            unsigned char BCC1R = ADDRESS_RECEIVE ^ CONTROL_UA;
            bufR2[0] = FLAG;
            bufR2[1] = ADDRESS_RECEIVE; //0X01
            bufR2[2] = CONTROL_UA; //0X07
            bufR2[3] = BCC1R;
            bufR2[4] = FLAG;
            printf("------------------------------------------------------------------------\n");
            printf("Read UA (llopen) \n");
            printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
            int bytesR = writeBytesSerialPort(bufR2, BUF_SIZE);
            printf("%d bytes written (UA)(llopen)\n", bytesR);
            printf("------------------------------------------------------------------------\n");
            frameCounter++;    
            
            break;
            
        default:
            printf("ERROR: role unknown");
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
    
    if (Ns_t == 0){
        frame[2] = CONTROL_I_N0; //C = 0x00
    } else if (Ns_t == 1) {
        frame[2] = CONTROL_I_N1; //C = 0x80
    } else {
        printf("ERROR: INFO FRAME C\n");
        return -1;
    }
    
    frame[3] = frame[1] ^ frame[2]; // A^C
    
    memcpy (frame+4, buf, bufSize); //D1...Dn
    
    int k = 4;
    for(int i = 0; i < bufSize; i++){
        if(buf[i] == FLAG){ // se 0x7e -> 0x7d 0x5e
            frame = realloc(frame,++sizeOfFrame);
            frame[k] = ESCAPE;
            k++;
            frame[k] = XOR_FLAG;
            k++;
        } else if (buf[i] == ESCAPE) {  // se 0x7d -> 0x7d 0x5dc
            frame = realloc(frame,++sizeOfFrame);
            frame[k] = ESCAPE;
            k++; 
            frame[k] = XOR_ESCAPE;
            k++;
        } else {
            frame[k] = buf[i];
            k++;
        }
    }

    unsigned char BCC2 = buf[0];
    for (unsigned int i = 1 ; i < bufSize ; i++) { BCC2 = BCC2 ^ buf[i]; } // BCC2 = D1^D2^...^Dn
    frame[k] = BCC2;
    k++;
    frame[k] = FLAG; //flag
    k++;

    printf("------------------------------------------------------------------------\n");
    printf("Write I (llwrite)\n");
    int bytesW = writeBytesSerialPort(frame, k);
    printf("%d bytes written (llwrite)\n", bytesW);
    printf("------------------------------------------------------------------------\n");
    frameCounter++;

    // Loop for input
    unsigned char bufllwrite[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

    state_t statellwrite = START_STATE;
    int a_prov_llwrite = 0;
    int c_prov_llwrite = 0;
    alarmEnabled = FALSE;
    alarmCount = 0;

    while(statellwrite != STOP_STATE && alarmCount < connectionParametersCopy.nRetransmissions) {

        if (alarmEnabled == FALSE) {
            if(alarmCount != 0){
                printf("------------------------------------------------------------------------\n");
                printf("Write I again (llwrite)\n");
                int bytesW = writeBytesSerialPort(frame, k);
                printf("%d bytes written (llwrite)\n", bytesW);
                printf("------------------------------------------------------------------------\n");
                frameCounter++;
            }
            
            ///printf("alarme do write\n");
            alarm(connectionParametersCopy.timeout); // Set alarm to be triggered in Xs
            alarmEnabled = TRUE;
        }

        unsigned char byte;

        if(readByteSerialPort(&byte) > 0){
            switch (statellwrite) {
                case START_STATE:
                    if(byte == FLAG) {
                        bufllwrite[0] = byte;
                        statellwrite = FLAG_STATE;
                    }
                    break;
                case FLAG_STATE:
                    if(byte == FLAG) {
                        bufllwrite[0] = byte;
                    } else if (byte == ADDRESS_RECEIVE) {
                        bufllwrite[1] = byte;
                        a_prov_llwrite = byte;
                        statellwrite = A_STATE;
                    } else {
                        statellwrite = START_STATE;
                    }
                    break;
                case A_STATE:
                    if(byte == FLAG) {
                        bufllwrite[0] = byte;
                        statellwrite = FLAG_STATE;
                    } else if (Ns_t == 0 && byte == CONTROL_RR1 && Nr_r == 1) {
                        bufllwrite[2] = byte;
                        c_prov_llwrite = byte;
                        statellwrite = C_STATE;
                        Ns_t++;
                        Nr_r = 0;
                    } else if (Ns_t == 1 && byte == CONTROL_RR0 && Nr_r == 0) {
                        bufllwrite[2] = byte;
                        c_prov_llwrite = byte;
                        statellwrite = C_STATE;
                        Ns_t = 0;
                        Nr_r++;
                    } else if (Ns_t == 0 && byte == CONTROL_REJ0 && Nr_r == 1) {
                        statellwrite = START_STATE;
                        alarmEnabled = FALSE;
                    } else if (Ns_t == 1 && byte == CONTROL_REJ1 && Nr_r == 0) {
                        statellwrite = START_STATE;
                        alarmEnabled = FALSE;
                    } else {
                        statellwrite = START_STATE;
                    }
                    break;
                case C_STATE:
                    if(byte == FLAG) {
                        bufllwrite[0] = byte;
                        statellwrite = FLAG_STATE;
                    } else if (byte == (a_prov_llwrite ^ c_prov_llwrite)) {
                        bufllwrite[3] = byte;
                        statellwrite = BCC_STATE;
                    } else {
                       statellwrite = START_STATE;
                    }
                    break; 
                case BCC_STATE:
                    if(byte == FLAG) {
                        bufllwrite[4] = byte;
                        statellwrite = STOP_STATE;
                        frameCounterReceived++;
                        alarm(0);
                    } else {
                        statellwrite = START_STATE;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    printf("------------------------------------------------------------------------\n");
    printf("Collected from Read (llwrite): ");
    for (int i = 0; i < 5; i++){
        printf("0x%02X |", bufllwrite[i]);
    }
    printf("\n");  
    printf("------------------------------------------------------------------------\n");

    free(frame);
    
    alarmEnabled = FALSE;
    alarmCount = 0;
    if(statellwrite == STOP_STATE) {return sizeOfFrame;} // correto

    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
    // READ RECEBE
    // Loop for input
    state_t stateR = START_STATE;
    int a_prov2 = 0;
    int c_prov2 = 0;
    int i = 0;

    while(stateR != STOP_STATE) {
        unsigned char byte;

        if(readByteSerialPort(&byte) > 0){
            switch (stateR) {
                case START_STATE:
                    if(byte == FLAG) {
                        stateR = FLAG_STATE;
                    }
                    break;
                case FLAG_STATE:
                    if(byte == FLAG) {
                    } else if (byte == ADDRESS_SEND) {
                        a_prov2 = byte;
                        stateR = A_STATE;
                    } else if (byte == 0x00) {
                        // Ignore and stay
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case A_STATE:
                    if(byte == FLAG) {
                        stateR = FLAG_STATE;
                    } else if ((Ns_t == 0 && byte == CONTROL_I_N0) || (Ns_t == 1 && byte == CONTROL_I_N1)){
                        c_prov2 = byte;
                        stateR = C_STATE;
                    } else if (byte == CONTROL_DISC ) {
                        stateR = PRE_STOP_STATE;
                        
                    } else if (byte == 0x00) {
                        // Ignore and stay
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case PRE_STOP_STATE:
                    if(byte == FLAG) {
                        stateR = STOP_STATE;
                        return 5;
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                    break;
                case C_STATE:
                    if(byte == FLAG) {
                        stateR = FLAG_STATE;
                    } else if (byte == (a_prov2 ^ c_prov2)) {
                        stateR = BCC_STATE;
                    } else if (byte == 0x00) { 
                        // Ignore and stay
                    } else {
                        stateR = START_STATE;
                    }
                    break;
                case BCC_STATE: // DESTUFF AND BCC CHECKING
                    if (byte == ESCAPE) {
                        stateR = DESTUFFING_STATE;
                    } else if (byte == FLAG) {
                        unsigned int bcc2 = packet[--i];
                        packet[i] = '\0';

                        unsigned int bcc_calculo = packet[0];
                        for(int j = 1 ; j<i ; j++){
                            bcc_calculo = bcc_calculo ^ packet[j];
                        }

                        if(bcc2 == bcc_calculo) { // TUDO CORRECTO YAYYYYYYY
                            stateR = STOP_STATE;
                            frameCounterReceived++;
                            
                            //responder :)
                            if (Nr_r == 0){ // responder com RR0
                                unsigned char frame[5] = {FLAG, ADDRESS_RECEIVE, CONTROL_RR0, ADDRESS_RECEIVE ^ CONTROL_RR0, FLAG};
                                writeBytesSerialPort(frame, 5);
                                printf("------------------------------------------------------------------------\n");
                                printf("Read RR0 (llread)\n");
                                printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", frame[0], frame[1], frame[2], frame[3], frame[4]);
                                printf("%d bytes written (RR0)(llread)\n", 5);
                                printf("------------------------------------------------------------------------\n");
                                frameCounter++;
                                Nr_r = 1;
                                Ns_t = 0;
                            } else { // responder com RR1
                                unsigned char frame[5] = {FLAG, ADDRESS_RECEIVE, CONTROL_RR1, ADDRESS_RECEIVE ^ CONTROL_RR1, FLAG};
                                writeBytesSerialPort(frame, 5);
                                printf("------------------------------------------------------------------------\n");
                                printf("Read RR1 (llread)\n");
                                printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", frame[0], frame[1], frame[2], frame[3], frame[4]);
                                printf("%d bytes written (RR1)(llread)\n", 5);
                                printf("------------------------------------------------------------------------\n");
                                frameCounter++;
                                Nr_r = 0;
                                Ns_t = 1;
                            }
                            /*printf("recebi do write (llread) : ");
                            for (int j = 0; j < i; i++){
                                printf("0x%02X |", frame[j]);
                            } 
                            printf("\n");*/
                            return i;
                        } else {
                            //responder erro :(
                            if (Nr_r == 0){ // rejeitar com REJ0
                                unsigned char frame[5] = {FLAG, ADDRESS_RECEIVE, CONTROL_REJ0, ADDRESS_RECEIVE ^ CONTROL_REJ0, FLAG};
                                writeBytesSerialPort(frame, 5);
                                printf("------------------------------------------------------------------------\n");
                                printf("Read REJ0 (llread)\n");
                                printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", frame[0], frame[1], frame[2], frame[3], frame[4]);
                                printf("%d bytes written (REJ0)(llread)\n", 5);
                                printf("------------------------------------------------------------------------\n");
                                frameCounter++;
                            } else { // rejeitar com REJ1
                                unsigned char frame[5] = {FLAG, ADDRESS_RECEIVE, CONTROL_REJ1, ADDRESS_RECEIVE ^ CONTROL_REJ1, FLAG};
                                writeBytesSerialPort(frame, 5);
                                printf("------------------------------------------------------------------------\n");
                                printf("Read REJ1 (llread)\n");
                                printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", frame[0], frame[1], frame[2], frame[3], frame[4]);
                                printf("%d bytes written (REJ1)(llread)\n", 5);
                                printf("------------------------------------------------------------------------\n");
                                frameCounter++; 
                            }
                        }

                    } else {
                        packet[i++] = byte;
                    }

                    break;
                case DESTUFFING_STATE:
                    if(byte == XOR_ESCAPE){
                        packet[i++] = ESCAPE;
                        stateR = BCC_STATE;
                    } else if (byte == XOR_FLAG) {
                        packet[i++] = FLAG;
                        stateR = BCC_STATE;
                    } else {
                        stateR = STOP_STATE;
                        return -1;
                    }
                    
                    break;
                default:
                    break;
                }
        }
        
    }
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {    

    switch (connectionParametersCopy.role)
    {
    case LlTx: // transmiter
        //WRITE ESCREVE
        sleep(1);
        unsigned char bufW[BUF_SIZE];

        unsigned char BCC1W = ADDRESS_SEND ^ CONTROL_DISC;
        bufW[0] = FLAG;
        bufW[1] = ADDRESS_SEND; //0X03
        bufW[2] = CONTROL_DISC; //0x0B
        bufW[3] = BCC1W;
        bufW[4] = FLAG;
        printf("------------------------------------------------------------------------\n");
        printf("Write DISC (llclose)\n");
        printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
        int bytesW = writeBytesSerialPort(bufW, BUF_SIZE);
        printf("%d bytes written (DISC)(llclose)\n", bytesW);
        printf("------------------------------------------------------------------------\n");
        frameCounter++;

        //WRITE RECEBE DE VOLTA
    
        // Wait until all bytes have been written to the serial port
        sleep(5);
        (void)signal(SIGALRM, alarmHandler);
        alarmEnabled = FALSE;
        alarmCount = 0;

        // Loop for input
        unsigned char bufW2[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

        state_t stateW = START_STATE;
        int a_prov1 = 0;
        int c_prov1 = 0;

        while(stateW != STOP_STATE && alarmCount < connectionParametersCopy.nRetransmissions) {

            if (alarmEnabled == FALSE) {
                if(alarmCount != 0){
                    unsigned char bufW[BUF_SIZE];

                    unsigned char BCC1W = ADDRESS_SEND ^ CONTROL_DISC;
                    bufW[0] = FLAG;
                    bufW[1] = ADDRESS_SEND; //0X03
                    bufW[2] = CONTROL_DISC; //0x0B
                    bufW[3] = BCC1W;
                    bufW[4] = FLAG;
                    printf("------------------------------------------------------------------------\n");
                    printf("Write DISC again (llclose)\n");
                    printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
                    int bytesW = writeBytesSerialPort(bufW, BUF_SIZE);
                    printf("%d bytes written (DISC)(llclose)\n", bytesW);
                    printf("------------------------------------------------------------------------\n");
                    frameCounter++;
                }
                
                // printf("alarme do write\n");
                alarm(connectionParametersCopy.timeout); // Set alarm to be triggered in Xs
                alarmEnabled = TRUE;
                
            }

            unsigned char byte;

            if(readByteSerialPort(&byte) > 0){
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
                        } else if (byte == CONTROL_DISC) {
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
                            frameCounterReceived++;
                            alarm(0);
                            
                            printf("------------------------------------------------------------------------\n");
                            printf("Collected from Read (llclose): ");
                            for (int i = 0; i < 5; i++){
                                printf("0x%02X |", bufW2[i]);
                            }
                            printf("\n");  
                            printf("------------------------------------------------------------------------\n");   

                            //printf("write vai escrever\n");
                            unsigned char bufAU[BUF_SIZE];

                            unsigned char BCC1AU = ADDRESS_SEND ^ CONTROL_UA;
                            bufAU[0] = FLAG;
                            bufAU[1] = ADDRESS_SEND; //0X03
                            bufAU[2] = CONTROL_UA; //0X07
                            bufAU[3] = BCC1AU;
                            bufAU[4] = FLAG;
                            printf("------------------------------------------------------------------------\n");
                            printf("Write UA (llclose)\n");
                            printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
                            int bytesAU = writeBytesSerialPort(bufAU, BUF_SIZE);
                            printf("%d bytes written (UA)(llclose)\n", bytesAU);
                            printf("------------------------------------------------------------------------\n");
                            frameCounter++;


                        } else {
                            stateW = START_STATE;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

             
        alarmEnabled = FALSE;
        alarmCount = 0;
        if (stateW != STOP_STATE) { return -1;}

        break;
    case LlRx: //receiver

        // READ RECEBE
        // Loop for input
        unsigned char bufR[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

        state_t stateR = START_STATE;
        int a_prov2 = 0;
        int c_prov2 = 0;

        while(stateR != STOP_STATE) {
            unsigned char byte;

            if(readByteSerialPort(&byte) > 0){
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
                        } else if (byte == CONTROL_DISC) {
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
                            frameCounterReceived++;
                        } else {
                            stateR = START_STATE;
                        }
                        break;
                    default:
                        break;
                    }
            }
            
        }

        printf("------------------------------------------------------------------------\n");
        printf("Collected from Write (llclose): ");
        for (int i = 0; i < 5; i++){
            printf("0x%02X |", bufR[i]);
        }
        printf("\n");  
        printf("------------------------------------------------------------------------\n");

        //READ RESPONDE DE VOLTA
        // Create string to send
        unsigned char bufR2[BUF_SIZE];
        unsigned char BCC1R = ADDRESS_RECEIVE ^ CONTROL_DISC;
        bufR2[0] = FLAG;
        bufR2[1] = ADDRESS_RECEIVE; //0X01
        bufR2[2] = CONTROL_DISC; //0X0B
        bufR2[3] = BCC1R;
        bufR2[4] = FLAG;
        printf("------------------------------------------------------------------------\n");
        printf("Read DISC (llclose): \n");
        printf("Flag: 0x%02X | Address: 0x%02X | Control: 0x%02X | BCC: 0x%02X | Flag: 0x%02X\n", bufW[0], bufW[1], bufW[2], bufW[3], bufW[4]);
        int bytesR = writeBytesSerialPort(bufR2, BUF_SIZE);
        printf("%d bytes written (DISC)(llclose)\n", bytesR);
        printf("------------------------------------------------------------------------\n");
        frameCounter++;    
        
        stateR = START_STATE;
        a_prov2 = 0;
        c_prov2 = 0;

        while(stateR != STOP_STATE) {
            unsigned char byte;

            if(readByteSerialPort(&byte)){
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
                        } else if (byte == CONTROL_UA) {
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
                            frameCounterReceived++;
                        } else {
                            stateR = START_STATE;
                        }
                        break;
                    default:
                        break;
                    }
            }
            
        }

        printf("------------------------------------------------------------------------\n");
        printf("Collected from Write (llclose): ");
        for (int i = 0; i < 5; i++){
            printf("0x%02X |", bufR[i]);
        }
        printf("\n");  
        printf("------------------------------------------------------------------------\n");

        break;
         
    default:
        printf("error: role unknown");
        return -1;
    }
    
    if(showStatistics == TRUE) {
        printf("\n");
        printf("------------------------------------------------------------------------\n");
        switch (connectionParametersCopy.role)
        {
        case LlTx:
            printf("Transmitter sent %u frames in this file transfer.\n", frameCounter);
            printf("Transmitter collected %u frames in this file transfer.\n", frameCounterReceived);
            break;
        case LlRx:
            printf("Receiver sent %u frames in this file transfer.\n", frameCounter);
            printf("Receiver collected %u frames in this file transfer.\n", frameCounterReceived);
            break;
    
        default:
            break;
        }
        printf("------------------------------------------------------------------------\n");
    }
    
    int clstat = closeSerialPort();
    return clstat;
}
