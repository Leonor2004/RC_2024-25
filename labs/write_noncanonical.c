// Write to serial port in non-canonical mode

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5
#define FLAG 0x7E
#define ADDRESS_SEND 0x03
#define CONTROL_SEND 0x03

#define ADDRESS_RECIVE 0x01
#define CONTROLL_RECIVE 0x07

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;

typedef enum {
    START_STATE,
    FLAG_STATE,
    A_STATE,
    C_STATE,
    BCC_STATE,
    STOP_STATE
} state_t;

// Alarm function handler
void alarmHandler(int signal)
{
   alarmEnabled = FALSE;
   alarmCount++;

   printf("Alarm #%d\n", alarmCount);
}



int main(int argc, char *argv[])
{
  // Program usage: Uses either COM1 or COM2
  const char *serialPortName = argv[1];

  if (argc < 2)
  {
      printf("Incorrect program usage\n"
             "Usage: %s <SerialPort>\n"
             "Example: %s /dev/ttyS1\n",
             argv[0],
             argv[0]);
      exit(1);
  }

  // Open serial port device for reading and writing, and not as controlling tty
  // because we don't want to get killed if linenoise sends CTRL-C.
  int fd = open(serialPortName, O_RDWR | O_NOCTTY);

  if (fd < 0)
  {
      perror(serialPortName);
      exit(-1);
  }

  struct termios oldtio;
  struct termios newtio;

  // Save current port settings
  if (tcgetattr(fd, &oldtio) == -1)
  {
      perror("tcgetattr");
      exit(-1);
  }

  // Clear struct for new port settings
  memset(&newtio, 0, sizeof(newtio));

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  // Set input mode (non-canonical, no echo,...)
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0; // Inter-character timer unused
  newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

  // VTIME e VMIN should be changed in order to protect with a
  // timeout the reception of the following character(s)

  // Now clean the line and activate the settings for the port
  // tcflush() discards data written to the object referred to
  // by fd but not transmitted, or data received but not read,
  // depending on the value of queue_selector:
  //   TCIFLUSH - flushes data received but not read.
  tcflush(fd, TCIOFLUSH);

  // Set new port settings
  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
      perror("tcsetattr");
      exit(-1);
  }

  printf("New termios structure set\n");

  // Create string to send
  unsigned char buf[BUF_SIZE];
  
  unsigned char BCC1 = ADDRESS_SEND ^ CONTROL_SEND;

  buf[0] = FLAG;
  buf[1] = ADDRESS_SEND;
  buf[2] = CONTROL_SEND;
  buf[3] = BCC1;
  buf[4] = FLAG;
 

  // In non-canonical mode, '\n' does not end the writing.
  // Test this condition by placing a '\n' in the middle of the buffer.
  // The whole buffer must be sent even with the '\n'.
  //buf[5] = '\n';

  int bytes = write(fd, buf, BUF_SIZE);
  printf("%d bytes written\n", bytes);

  // Wait until all bytes have been written to the serial port
  sleep(1);
  (void)signal(SIGALRM, alarmHandler);

   // Loop for input
   unsigned char buf2[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

   /*while (STOP == FALSE && alarmCount < 3)
   {
       
       if (alarmEnabled == FALSE)
       {
           printf("alarme de 3 segundos \n");
           alarm(3); // Set alarm to be triggered in 3s
           alarmEnabled = TRUE;
       }

       // Returns after 5 chars have been input
       int bytes = read(fd, buf2, BUF_SIZE);
       //buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

       for (int i = 0; i < 5; i++){
           printf("var = 0x%02X\n", buf2[i]);
       }
       
       if(((buf2[0] == FLAG) && (buf2[4] == FLAG) && (buf2[1] == ADDRESS2) && ((buf2[1] ^ buf2[2]) == buf2[3]))){
           alarm(0);
           STOP = TRUE;
           alarmCount = 1000;
       }
       
       printf(":%s:%d\n", buf2, bytes);
   }*/

    state_t state = START_STATE;
    int a_prov = 0;
    int c_prov = 0;

    while(state != STOP_STATE && alarmCount < 3) {

        if (alarmEnabled == FALSE)
       {
           printf("alarme de 3 segundos \n");
           alarm(3); // Set alarm to be triggered in 3s
           alarmEnabled = TRUE;
       }

        unsigned char byte = 0;

        int bytes = read(fd, byte, BUF_SIZE);

        switch (state)
        {
        case START_STATE:
            if(byte == FLAG) {
                buf2[0] = byte;
                state = FLAG_STATE;
            }
            break;
        case FLAG_STATE:
            if(byte == FLAG) {
                buf2[0] = byte;
            } else if (byte == ADDRESS_RECIVE) {
                buf2[1] = byte;
                a_prov = byte;
                state = A_STATE;
            } else {
                state = START_STATE;
            }
            break;
        case A_STATE:
            if(byte == FLAG) {
                buf2[0] = byte;
                state = FLAG_STATE;
            } else if (byte == CONTROLL_RECIVE) {
                buf2[2] = byte;
                c_prov = byte;
                state = C_STATE;
            } else {
                state = START_STATE;
            }
            break;
        case C_STATE:
            if(byte == FLAG) {
                buf2[0] = byte;
                state = FLAG_STATE;
            } else if (byte == a_prov ^ c_prov) {
                buf2[3] = byte;
                state = BCC_STATE;
            } else {
                state = START_STATE;
            }
            break;
        case BCC_STATE:
            if(byte == FLAG) {
                buf2[4] = byte;
                state = STOP_STATE;
                alarm(0);
            } else {
                state = START_STATE;
            }
            break;
        default:
            break;
        }
    }


  // Restore the old port settings
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
      perror("tcsetattr");
      exit(-1);
  }

  close(fd);

  return 0;
}
