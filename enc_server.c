#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>

// Function to display error messages and terminate the program
void reportError(const char *msg) {
  perror(msg); // Prints a descriptive error message
  exit(1); // Exits the program with a non-zero status to indicate an error
}

// Function to initialize the server's address structure
void initializeServerAddress(struct sockaddr_in *address, int portNumber) {
  memset((char *)address, '\0', sizeof(*address)); // Clears the structure
  address->sin_family = AF_INET; // Sets the address family to IPv4
  address->sin_port = htons(portNumber); // Sets the port number, converting it to network byte order
  address->sin_addr.s_addr = INADDR_ANY; // Allows the server to bind to any available interface
}

// Function to perform basic encryption on a message
char *performEncryption(char *buffer) {
  // Allocating memory for the cipher text, plain text, and key
  char *cipher_text = calloc(800000, sizeof(char));
  char *plain_text = calloc(800000, sizeof(char));
  char *key = calloc(800000, sizeof(char));

  // Extracting the plain text and key from the buffer
  plain_text = strtok(buffer, "\n");
  strcat(cipher_text, plain_text);
  strcat(cipher_text, "\n");

  plain_text = strtok(NULL, "\n");
  key = strtok(NULL, "\n");

  // Character set for the encryption
  char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  int plain_index, key_index;

  // Encrypting each character by shifting it according to the key
  for (int i = 0; i < strlen(plain_text); ++i) {
    for (plain_index = 0; plain_text[i] != chars[plain_index]; ++plain_index) {}
    for (key_index = 0; key[i] != chars[key_index]; ++key_index) {}

    int sum = (plain_index + key_index) % 27;
    strncat(cipher_text, &chars[sum], 1);
  }

  strcat(cipher_text, "\neom\n"); // Appending end-of-message marker
  return cipher_text; // Returning the encrypted message
}

int main(int argc, char *argv[]) {
  int connectionSocket;
  char buffer[800000];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Checking command line arguments
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]); // Prompting for the correct usage
    exit(1); // Exiting if the port number is not provided
  }

  // Creating a listening socket
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    reportError("ERROR opening socket");
  }

  // Initializing server address
  initializeServerAddress(&serverAddress, atoi(argv[1]));

  // Binding the listening socket to the server address
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    reportError("ERROR on binding");
  }

  // Listening for incoming connections, with a maximum of 5 pending connections
  listen(listenSocket, 5);

  for (;;) { // Infinite loop to handle multiple connections
    // Accepting a connection
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    if (connectionSocket < 0) {
      reportError("ERROR on accept");
    }

    // Creating a child process to handle the connection
    pid_t pid = fork();
    switch (pid) {
      case -1:
        reportError("Fork failed\n");
        exit(1);

      case 0:
        // Child process code
        memset(buffer, '\0', 800000); // Clearing the buffer
        char inbuffer[800000] = {'\0'}; // Buffer for incoming messages

        // Reading messages until the end-of-message marker is found
        while (!strstr(buffer, "eom\n")) {
          int charsRead = recv(connectionSocket, inbuffer, 80000, 0);
          if (charsRead < 0) {
            reportError("ERROR reading from socket");
          }
          strcat(buffer, inbuffer);
        }

        // Encrypting the message if it starts with 'E'
        if (buffer[0] == 'E') {
          strcpy(buffer, performEncryption(buffer));
        } else {
          strcpy(buffer, "B\neom\n"); // Sending a basic response if the message doesn't start with 'E'
        }

        // Sending the response back to the client
        int size = strlen(buffer);
        for (int charsSent = 0; charsSent < size; ) {
          int charsRead = send(connectionSocket, buffer + charsSent, size - charsSent, 0);
          if (charsRead < 0) {
            reportError("CLIENT: ERROR writing to socket.\n");
          }
          charsSent += charsRead;
        }

        // Closing the connection and exiting the child process
        close(connectionSocket);
        exit(0);

      default:
        // Parent process code
        wait(NULL); // Waiting for the child process to finish
    }
  }

  // Closing the listening socket
  close(listenSocket);
  return 0;
}
