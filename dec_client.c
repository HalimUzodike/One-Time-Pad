#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>

// Function to handle errors by printing a message and exiting
void handleError(const char *msg) {
    perror(msg); // Prints the error message passed to the function
    exit(1); // Exits the program with a status code of 1 indicating an error
}

// Function to configure the server's address
void configureServerAddress(struct sockaddr_in *serverAddr, int port, char *hostName) {
    memset(serverAddr, '\0', sizeof(*serverAddr)); // Clears the serverAddr structure
    serverAddr->sin_family = AF_INET; // Sets the address family to AF_INET (IPv4)
    serverAddr->sin_port = htons(port); // Sets the port number after converting it to network byte order

    struct hostent *serverHost = gethostbyname(hostName); // Retrieves host information
    if (!serverHost) {
        fprintf(stderr, "CLIENT: ERROR, host not found\n"); // Error if host not found
        exit(1);
    }
    memcpy(&serverAddr->sin_addr.s_addr, serverHost->h_addr, serverHost->h_length); // Copies the host's IP address to serverAddr
}

// Function to validate file contents for valid characters
void validateFileContents(const char *cipherFile, const char *keyFile) {
    char validChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // List of valid characters
    FILE *file;
    char ch;

    // Validates the cipher file
    file = fopen(cipherFile, "r");
    if (!file) handleError("CLIENT: Error opening cipher text file");

    while ((ch = fgetc(file)) != EOF && ch != '\n') {
        if (!strchr(validChars, ch)) { // Check if character is in validChars
            fclose(file);
            handleError("CLIENT: Invalid character in cipher text");
        }
    }
    fclose(file);

    // Validates the key file
    file = fopen(keyFile, "r");
    if (!file) handleError("CLIENT: Error opening key file");

    while ((ch = fgetc(file)) != EOF && ch != '\n') {
        if (!strchr(validChars, ch)) { // Check if character is in validChars
            fclose(file);
            handleError("CLIENT: Invalid character in key");
        }
    }
    fclose(file);
}

// Function to compose a message from cipher text and key files
void composeMessage(char *msg, const char *cipherFile, const char *keyFile) {
    FILE *file;
    char line[800000]; // Large buffer to store lines from files

    strcpy(msg, "D\n"); // Start message with 'D' indicating decryption request

    // Append cipher text to message
    file = fopen(cipherFile, "r");
    fgets(line, 800000, file); // Reads a line from cipher file
    fclose(file);
    strcat(msg, line);
    strcat(msg, "\n");

    // Append key to message
    file = fopen(keyFile, "r");
    fgets(line, 800000, file); // Reads a line from key file
    fclose(file);
    strcat(msg, line);
    strcat(msg, "\neom\n"); // Append end-of-message marker
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddr;
    char buffer[800000]; // Buffer for sending and receiving data

    // Check for correct number of command-line arguments
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s ciphertext key port\n", argv[0]); // Prompt correct usage
        exit(1);
    }

    // Open cipher text and key files to compare their sizes
    int cipherTextFile = open(argv[1], O_RDONLY);
    int keyFile = open(argv[2], O_RDONLY);

    // Check if key is long enough
    if (lseek(cipherTextFile, 0, SEEK_END) > lseek(keyFile, 0, SEEK_END)) {
        handleError("CLIENT: Key file is too short");
    }

    // Validate the contents of cipher text and key files
    validateFileContents(argv[1], argv[2]);

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) handleError("CLIENT: Error opening socket");

    // Configure server address
    configureServerAddress(&serverAddr, atoi(argv[3]), "localhost");

    // Connect to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        handleError("CLIENT: Error connecting");
    }

    // Compose and send a message to the server
    composeMessage(buffer, argv[1], argv[2]);
    charsWritten = send(socketFD, buffer, strlen(buffer), 0);
    if (charsWritten < 0) handleError("CLIENT: Error writing to socket");
    if (charsWritten < strlen(buffer)) {
        printf("CLIENT: Warning: Not all data written to socket\n");
    }

    // Clear buffer and read the response from the server
    memset(buffer, '\0', 800000);
    while (!strstr(buffer, "eom\n")) {
        char inBuffer[800000] = {'\0'};
        charsRead = recv(socketFD, inBuffer, 800000 - 1, 0);
        if (charsRead < 0) handleError("CLIENT: Error reading from socket");
        strcat(buffer, inBuffer); // Append received data to buffer
    }

    // Process server response
    char *responseCode = strtok(buffer, "\n");
    if (responseCode && responseCode[0] == 'B') {
        handleError("CLIENT: Received error from server");
    } else {
        printf("%s\n", strtok(NULL, "\n")); // Print the decrypted message
    }

    // Close socket and end program
    close(socketFD);
    return 0;
}
