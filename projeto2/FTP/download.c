#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

/*
Codes:
LOGED_IN_CODE 220
PASSWORD_CODE 331
LOGIN_SUCCESSFULL_CODE 230
PASSIVE_MODE_CODE 227
START_TRANSFER_CODE 150
TRANSFER_COMPLETED_CODE 226 
*/

#define LOGED_IN_CODE 220
#define USERNAME_OK_PASSWORD_NOW 331
#define LOGIN_SUCCESSFULL_CODE 230
#define PASSIVE_MODE_CODE 227
#define START_TRANSFER_CODE 150
#define DATA_CONNECTION_ESTABLISHED_CODE 125
#define TRANSFER_COMPLETED_CODE 226
#define NUM_TRIES 3 // Number of tries to read response

#define AT              "%*[^/]//%s@"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define PATH_REGEX      "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"


// Function prototypes
int connect_to_server(const char *host, int port);
int read_response_code(int sockfd);
int read_response (int socket, char * response, int response_len);
int enter_passive_mode(int sockfd, char *ip, int *port);
void download_file(int data_sock, const char *filename);
const char *get_filename(const char *path);
int parse_url(char * url, char * username, char * password, char * host, char * path);

/**
 * @brief Main function: FTP protocol
 * 
 * @param argc : Number of arguments
 * @param argv : Arguments
 * @return int : 0 on success and 1 on error
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Missing URL as argument!\n");
        return EXIT_FAILURE;
    }

    if (NUM_TRIES <= 0) {
        printf("Error: Number of tries to read response has to be more than 0!\n");
        return EXIT_FAILURE;
    }

    // Parameters for the FTP server
    char host[256] = "";
    char user[256] = "\0";
    char password[256] = "";
    char filepath[256] = "";

    // Parse the URL
    if (parse_url(argv[1], user, password, host, filepath) < 0) {
        printf("Invalid URL: %s!\n", argv[1]);
        return -1;
    }

    // Get file name
    const char *filename = get_filename(filepath);

    // Set default values
    if (strcmp(user, "") == 0){
        strcpy(user, "anonymous");
    }
    if(strcmp(password, "") == 0){
        strcpy(password, "anonymous");
    }

    //Print the information
    printf("------------------------------------------------------------\n");
    printf("Information: \n");
    printf("------------------------------------------------------------\n");
    printf("Username: %s\n", user);
    printf("Password: %s\n", password);
    printf("Host: %s\n", host);
    printf("File path: %s\n", filepath);
    printf("File name: %s\n", filename);
    printf("------------------------------------------------------------\n");

    // Connect to the FTP server
    int control_sock = connect_to_server(host, 21);
    if (control_sock < 0) {
        fprintf(stderr, "Failed to connect to FTP server\n");
        close(control_sock);
        return EXIT_FAILURE;
    }

    int response_code = read_response_code(control_sock); // Initial server message
    printf("Initial response code: %d\n", response_code);

    if(!response_code) { // Problems
        close(control_sock);
        return EXIT_FAILURE;
    } else if(response_code == LOGIN_SUCCESSFULL_CODE) { // Already logged in
        printf("Already Logged in \n");
    } else if(response_code == LOGED_IN_CODE || response_code == 20) {
        // Send login credentials
        char login_cmd[BUFFER_SIZE];
        snprintf(login_cmd, sizeof(login_cmd), "USER %s\r\n", user);
        write(control_sock, login_cmd, strlen(login_cmd));
        int user_response = read_response_code(control_sock);
        if (user_response != USERNAME_OK_PASSWORD_NOW && user_response != 31) {
            printf("Problems with username. Response code: %d\n", user_response);
            close(control_sock);
            return EXIT_FAILURE;
        }

        snprintf(login_cmd, sizeof(login_cmd), "PASS %s\r\n", password);
        write(control_sock, login_cmd, strlen(login_cmd));
        int pass_response = read_response_code(control_sock);
        if (pass_response != LOGIN_SUCCESSFULL_CODE) {
            printf("Problems with password, won't give the loging sucessfull code. Response code: %d\n", pass_response);
            close(control_sock);
            return EXIT_FAILURE;
        }
    }

    // Enter passive mode and get data connection details
    char ip[BUFFER_SIZE];
    int data_port;
    if (!enter_passive_mode(control_sock, ip, &data_port)) {
        fprintf(stderr, "Failed to enter passive mode\n");
        close(control_sock);
        return EXIT_FAILURE;
    }
    
    int data_sock = connect_to_server(ip, data_port);
    if (data_sock < 0) {
        fprintf(stderr, "Failed to connect to data socket\n");
        close(control_sock);
        close(data_sock);
        return EXIT_FAILURE;
    }

    // Request the file
    char retr_cmd[BUFFER_SIZE];
    snprintf(retr_cmd, sizeof(retr_cmd), "RETR %s\r\n", filepath);
    write(control_sock, retr_cmd, strlen(retr_cmd));
    int prov = read_response_code(control_sock);
    if (prov != START_TRANSFER_CODE && prov != DATA_CONNECTION_ESTABLISHED_CODE) {
        fprintf(stderr, "Failed to start file transfer\n");
        close(control_sock);
        close(data_sock);
        return EXIT_FAILURE;
    }


    // Download the file using the extracted filename
    download_file(data_sock, filename);

    // Check if the file was downloaded successfully 
    int response_code_sucess = read_response_code(control_sock);

    if (response_code_sucess == TRANSFER_COMPLETED_CODE) {
        printf("File transfer completed successfully.\n");
    } else if (response_code_sucess != 1000) {
        fprintf(stderr, "File transfer failed. Server response code: %d\n", response_code_sucess);
        close(control_sock);
        return EXIT_FAILURE;
    }

    close(control_sock); // Close control socket

    return EXIT_SUCCESS;
}


/**
 * @brief Connects to the server
 * 
 * @param host : Hostname to connect to
 * @param port : Port number to connect to
 * @return int : Socket file descriptor or -1 if an error occurred
 */
int connect_to_server(const char *host, int port) {
    struct addrinfo hints; // Criteria for selecting socket addresses
    struct addrinfo *res;  // Where the result is saved
    struct addrinfo *prov2; // Pointer to iterate throung the list of potential addresses
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Specify address family and socket type
    hints.ai_socktype = SOCK_STREAM;

    // Save the port number as a string
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    // getaddrinfo -> Get a list of potential addresses
    if (getaddrinfo(host, port_str, &hints, &res) != 0) { // 0 in getaddrinfo is the sucess, othewise gives an nonzero error codes
        perror("getaddrinfo failed");
        return -1;
    }

    int sockfd = -1;
    for (prov2 = res; prov2 != NULL; prov2 = prov2->ai_next) { // Iterate the addresses
        sockfd = socket(prov2->ai_family, prov2->ai_socktype, prov2->ai_protocol);
        
        if (sockfd == -1) { // Error creating the socket
            continue;
        }

        if (connect(sockfd, prov2->ai_addr, prov2->ai_addrlen) == 0) { // Connect the socket to the address
            break; // Success
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res); // Clean
    return sockfd;
}


/**
 * @brief Read the response from the server
 * 
 * @param socket : Socket file descriptor
 * @param response : Where the response will be saved
 * @param response_len : Lenth of the response buffer
 * @return int : 0 if sucessfull and 1 if empty response
 */
int read_response (int socket, char * response, int response_len){
    int total_bytes_read = 0, bytes_read = 0; // Where the number of total and partial bytes read will be saved
    int n_tries = NUM_TRIES; // Numbered of tries
    usleep(100000); // Wait for a bit to ensure it can start with no problems

    // Read form server while number of tries > 0
    while (n_tries > 0) {
        // Try to read data from the socket
        bytes_read = recv(socket, response + total_bytes_read,response_len - total_bytes_read, MSG_DONTWAIT);
        
        if(bytes_read <= 0) { // Error, no bytes read
            n_tries--;
            usleep(100000);
        } else { // Could read bytes, yay
            n_tries = NUM_TRIES;
        }

        // Accumulate the total bytes read 
        total_bytes_read += bytes_read;
        bytes_read = 0; // Clean before next iteration in case of problems
    }

    response[total_bytes_read] = '\0';
    if(total_bytes_read > 0) { // Print the response or no response
        printf("Server Response: %s\n", response); 
    } else {
        printf("Empty response\n");
        return 1;
    }

    return 0;
}


/**
 * @brief Read the response code from the server
 * 
 * @param socket : Socket file descriptor
 * @return int : Response code
 */
int read_response_code(int socket){
    char response [BUFFER_SIZE]; // Buffer to save the response
    int response_code = 0; // Parsed response code

    int sucess = read_response(socket, response, BUFFER_SIZE); // Read the response
    if (sucess == 1) { // If empty response
        return 1000; // Is not a valid response code, chosen by us
    }

    sscanf(response, "%d", &response_code); // Get the response code

    return response_code;
}


/**
 * @brief Entering passive mode
 * 
 * @param sockfd : Socket
 * @param ip : IP
 * @param port : Port
 * @return int : 1 on success and 0 on error
 */
int enter_passive_mode(int sockfd, char *ip, int *port) {
    char buffer[BUFFER_SIZE]; // Buffer to save the response

    write(sockfd, "PASV\r\n", 6); // Send PASV command to enter passive mode
    read_response(sockfd, buffer, BUFFER_SIZE);  // Get the response

    // Parse the response to extract the IP and port information
    int val1, val2, val3, val4, p1, p2;
    int prov = sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &val1, &val2, &val3, &val4, &p1, &p2);
    if (prov != 6) {
        fprintf(stderr, "Error parsing PASV response: %s\n", buffer);
        return 0;   
    }

    // Format the IP address form the parced vals
    sprintf(ip, "%d.%d.%d.%d", val1, val2, val3, val4);

    // Calculate the port number
    *port = p1 * 256 + p2;

    return 1;
}


/**
 * @brief This function downloads the file from the data socket to our final file
 * 
 * @param control_sock : Control socket
 * @param data_sock : Data socket
 * @param filename : File name
 */
void download_file(int data_sock, const char *filename) {
    FILE *file = fopen(filename, "wb"); // Open file in wb mode

    if (!file) { // Problems opening the file
        perror("Error opening file");
        return;
    }

    char buffer[BUFFER_SIZE]; // Buffer to save the data read from the socket
    int bytes_read;

    // Read form data socket and write to the file
    while ((bytes_read = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_read, file);
    }

    fclose(file); // Close file
    close(data_sock); // Close data socket
}


/**
 * @brief Get the filename
 * 
 * @param path : Path to the file
 * @return const char* : Pointer to the filename portion of the path
 */
const char *get_filename(const char *path) {
    const char *filename = strrchr(path, '/'); // Find the last /
    return (filename != NULL) ? filename + 1 : path;
}


/**
 * @brief This function parses the URL
 * 
 * @param url : URL given
 * @param username : Where the username will be stored
 * @param password : Where the password will be stored
 * @param host : Where the host will be stored
 * @param path : Where the path will be stored
 * @return int : Return 0 indicating successful parsing
 */
int parse_url(char * url, char * username, char * password, char * host, char * path){
    char *aux = NULL; // auxiliar

    // Find the @ is there is any -> it has a username and a password
    sscanf(url,AT,aux);

    if(sscanf(url, "%*[^@]@%c", &aux) == 1){  // Extract the username if any
        sscanf(url,USER_REGEX,username);
    }

    if(username[0]!='\0') { // If username extracted then try extracting the password
        sscanf(url,PASS_REGEX,password);
    }

    if(username[0]!='\0') { // if there exists an username (and password), extract the host
        sscanf(url, HOST_AT_REGEX, host);
    } else { // if no username, extract the host
        sscanf(url, HOST_REGEX, host);
    }

    // Extract the path
    sscanf(url, PATH_REGEX, path);
   
    return 0;
}
