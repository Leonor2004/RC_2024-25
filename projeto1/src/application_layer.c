// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionParameters;


    switch (role[0]) {
        case 't':
            connectionParameters.role=LlTx;
            break;
        case 'r':
            connectionParameters.role=LlRx;
            break;
        default:
            printf("error: unknown user.role");
            return;
    }
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions=nTries;
    connectionParameters.timeout=timeout;


    if (llopen(connectionParameters)!=1) {
        printf("error: llopen failed");
    }


    switch (connectionParameters.role) {
        case LlTx:
            //TODO

            break;
        case LlRx:
            //TODO
            
            break;
        default:
            printf("error: role unknown");
            return;
    }


}
