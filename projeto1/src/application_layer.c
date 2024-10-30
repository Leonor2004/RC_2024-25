// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fenv.h>


unsigned char * createControlPacket(const char* filename, int fileSize, unsigned int* controlPacketSize, const unsigned int cField) {
    // The 'log2f' finds the number of bits needed to represent 'fileSize', and dividing by 8 converts bits to bytes.
    // 'ceil' ensures we round up to the nearest whole number of bytes.
    //const int L1 = (int) ceil(log2f((float)fileSize)/8.0); // Number of bytes needed to store the file size
    int L1 = 0;
    unsigned long tempSize = fileSize;
    while (tempSize > 0) {
        L1++;
        tempSize >>= 8;
    }
    
    const int L2 = strlen(filename); // Number of bytes needed to store the file name
    
    *controlPacketSize = 1+2+L1+2+L2; // Control packet total size

    unsigned char *packet = (unsigned char*)malloc(*controlPacketSize);
    packet[0] = cField; // Set the 'cField' field of the control packet. (values:  1 -> start or 3 -> end)
    packet[1] = 0; // Marking that we are going to place info of file size. (T1)
    packet[2] = L1; // Set the size of the fileSize content.
    
    // Now we insert the file size ('fileSize') into the packet, byte by byte (little-endian order).
    // The loop writes the least significant byte of 'fileSize' first, then shifts right by 8 bits
    // and repeats until 'L1' bytes are filled in the packet.
    for (unsigned char i = 0; i < L1; i++) {
        packet[2+L1-i] = fileSize & 0xFF;  // Write the least significant byte of 'length'.
        fileSize>>=8;                      // Shift 'length' to the right by 8 bits.
    }

    int packetPosition = 2 + L1;
    packet[packetPosition] = 1; // Marking that we are going to place info of file name. (T2)
    packetPosition++;
    packet[packetPosition] = L2; // Set the size of the filename content.
    packetPosition++;

    // Now we copy the filename into the packet at the current position.
    // The 'memcpy' function copies 'L2' bytes from 'filename' to 'packet + packetposition'.
    memcpy(packet+packetPosition, filename, L2); // Copy the filename into the packet.
    return packet;
}

unsigned char * createDataPacket(unsigned char* payload, int payloadSize, unsigned int* dataPacketSize, unsigned char sequenceNumber) {
    
    *dataPacketSize = 1+1+2+payloadSize; // Data packet total size
    
    unsigned char *packet = (unsigned char*)malloc(*dataPacketSize);
    packet[0] = 2; // Control field (value: 2 -> data)
    packet[1] = sequenceNumber; // Set the 'sequenceNumber' field of the data packet.
    packet[2] = payloadSize / 256; // Set L2.
    packet[3] = payloadSize % 256; // Set L1.

    memcpy(packet+4, payload, payloadSize); // Copy the payload into the packet.
    return packet;
}

unsigned char* analyseControlPacket(unsigned char* packet, int sizePacket) {
    
    unsigned char receivedFilesize;
    unsigned char L1 = packet [2]; // size of fileSize content
    unsigned char tempAux[L1]; // Auxiliar array to store the fileSize content
    memcpy(tempAux, packet+3, L1); // Copy the fileSize content into tempAux
    
    for (unsigned char i = 0; i < L1; i++) {
        receivedFilesize |= (tempAux[L1-i-1] << (8*i));
    }

    unsigned char L2 = packet[3+L1+1]; // size of filename contents
    unsigned char* receivedFilename = (unsigned char*) malloc (L2);
    memcpy(receivedFilename, packet+3+L1+1+1, L2);
    return receivedFilename;
}


void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename) {

    LinkLayer connectionParameters;

    switch (role[0]) {
        case 't':
            connectionParameters.role=LlTx;
            break;
        case 'r':
            connectionParameters.role=LlRx;
            break;
        default:
            printf("ERROR: unknown user.role");
            return;
    }

    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions=nTries;
    connectionParameters.timeout=timeout;

    if (llopen(connectionParameters) == -1) {
        printf("ERROR: llopen failed");
        return;
    }

    switch (connectionParameters.role) {
        case LlTx:
        
            FILE* file = fopen(filename, "rb"); // Open a binary file for reading. (The file must exist.)
            if (file == NULL) {
                perror("ERROR: file not found\n");
                return;
            }
            int prev = ftell(file); // Store current position
            fseek(file,0L,SEEK_END); // Go to end of file
            long int fileSize = ftell(file) - prev; // Get file size
            fseek(file,prev,SEEK_SET); // Go back to previous position


            unsigned int startingCPSize; // Control packet size
            unsigned char *startingControlPacket = createControlPacket(filename, fileSize, &startingCPSize, 1); // Get start control packet

            // Send data in buf with size bufSize.
            if (llwrite(startingControlPacket, startingCPSize) == -1) {
                printf("ERROR: llwrite starting control packet failed");
                return;
            }  
            

            unsigned char sequenceNum = 0;
            long int bytesRemaining = fileSize;
            unsigned char* content = (unsigned char* ) malloc (sizeof(unsigned char) * fileSize); // Allocate memory for file content
            fread(content, 1, fileSize, file); // Read file content into buffer
            
            while (bytesRemaining > 0) {
                
                int payloadSize=0;
                if (bytesRemaining > (long int) MAX_PAYLOAD_SIZE){
                    payloadSize = MAX_PAYLOAD_SIZE;
                } else {
                    payloadSize = bytesRemaining;
                }

                unsigned char* payload = (unsigned char* ) malloc(payloadSize); // Allocate memory for payload
                memcpy(payload, content, payloadSize); // Copy payload from content buffer
                
                unsigned int dataPacketSize; // Data packet size
                unsigned char *dataPacket = createDataPacket(payload, payloadSize, &dataPacketSize, sequenceNum);

                if (llwrite(dataPacket, dataPacketSize) == -1) {
                    printf("ERROR: llwrite data packet failed");
                    return;
                }

                sequenceNum = (sequenceNum+1) % 255; 

                bytesRemaining = bytesRemaining - payloadSize;
                content += payloadSize;
            }

            unsigned int endingCPSize; // Control packet size
            unsigned char *endingControlPacket = createControlPacket(filename, fileSize, &endingCPSize, 3); // Get start control packet
            // Send data in buf with size bufSize.
            if (llwrite(endingControlPacket, endingCPSize) == -1) {
                printf("ERROR: llwrite ending failed");
                return;
            }  
            break;

        case LlRx:

            unsigned char* packet = (unsigned char* ) malloc(MAX_PAYLOAD_SIZE); // Allocate memory for packet
            
            int sizePacket = -1;
            while (sizePacket == -1) {
                sizePacket = llread(packet);
            }
            
            unsigned char* receivedFilename = analyseControlPacket(packet, sizePacket);

            FILE* fileReceived = fopen((char*) filename, "wb+"); // Open an empty binary file for both reading and writing. (If file exists its contents are destroyed)
            if (fileReceived==NULL) printf("ERROR: fileReceived failed to open\n");

            
            while(TRUE) {
                int sizePacket2 = -1;
                while (sizePacket2 == -1) {
                    sizePacket2 = llread(packet);
                }
                
                int L1 = packet[3];
                int L2 = packet[2];
                int K = (256*L2) + L1;

                if(sizePacket2 == 0) {break;}
                else if (packet[0] == 3) {
                    break;
                }
                else if (packet[0] != 3) {  // If it is not an end control packet
                    unsigned char *bufReceived = (unsigned char* ) malloc (K);
                    memcpy(bufReceived,packet+4,K);
                    // bufReceived += sizePacket2+4;
                    fwrite(bufReceived, sizeof(unsigned char), K, fileReceived);
                    free(bufReceived);
                }
            }

            fclose(fileReceived);
            
            break;


        default:
            printf("ERROR: role unknown");
            return;
    }

    if(llclose(TRUE) == -1) {
        printf("ERROR: llclose failed");
        return;
    }

}
