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
  perror(msg); // Prints a system error message
  exit(1); // Exits the program with a non-zero status, indicating an error
}

// Function to initialize the server address structure
void initializeServerAddress(struct sockaddr_in *address, int portNumber) {
  memset(address, '\0', sizeof(*address)); // Clears the address structure
  address->sin_family = AF_INET; // Sets the address family to IPv4
  address->sin_port = htons(portNumber); // Converts the port number to network byte order
  address->sin_addr.s_addr = INADDR_ANY; // Binds the server to all available interfaces
}

// Function to perform decryption on a message
char *performDecryption(char *buffer) {
  char *plain_text = calloc(800000, sizeof(char)); // Allocates memory for plaintext
  char *cipher_text = calloc(800000, sizeof(char)); // Allocates memory for ciphertext
  char *key = calloc(800000, sizeof(char)); // Allocates memory for key

  cipher_text = strtok(buffer, "\n"); // Extracts ciphertext from buffer
  strcat(plain_text, cipher_text); // Starts the plaintext with the ciphertext
  strcat(plain_text, "\n");

  cipher_text = strtok(NULL, "\n"); // Extracts next part of ciphertext
  key = strtok(NULL, "\n"); // Extracts key

  char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // Character set for decryption
  int cipher_index, key_index;

  // Decrypt each character using the key
  for (int i = 0; i < strlen(cipher_text); ++i) {
    for (cipher_index = 0; cipher_text[i] != chars[cipher_index]; ++cipher_index) {}
    for (key_index = 0; key[i] != chars[key_index]; ++key_index) {}

    int diff = cipher_index - key_index; // Calculate difference for decryption
    if (diff < 0) diff += 27; // Adjust if difference is negative
    diff %= 27; // Ensure the result is within the character set

    strncat(plain_text, &chars[diff], 1); // Append decrypted character
  }

  strcat(plain_text, "\neom\n"); // Append end-of-message marker
  return plain_text; // Return the decrypted message
}

int main(int argc, char *argv[]) {
  int connectionSocket;
  char buffer[800000]; // Buffer for incoming and outgoing messages
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Validate the number of command-line arguments
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s port\n", argv[0]); // Display usage instructions
    exit(1); // Exit with an error if port is not specified
  }

  // Create a listening socket
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    reportError("ERROR opening socket");
  }

  // Initialize server address
  initializeServerAddress(&serverAddress, atoi(argv[1]));

  // Bind the listening socket to the server address
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    reportError("ERROR on binding");
  }

  // Listen for incoming connections
  listen(listenSocket, 5);

  // Main loop to handle incoming connections
  for (;;) {
    // Accept a connection
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
    if (connectionSocket < 0) {
      reportError("ERROR on accept");
    }

    // Create a child process to handle the connection
    pid_t pid = fork();
    switch (pid) {
      case -1: // Fork failed
        reportError("Fork failed\n");
        exit(1);

      case 0: // Child process
        memset(buffer, '\0', 800000); // Clear the buffer
        char inbuffer[800000] = {'\0'}; // Buffer for reading data

        // Read data until the end-of-message marker is found
        while (!strstr(buffer, "eom\n")) {
          int charsRead = recv(connectionSocket, inbuffer, 800000, 0);
          if (charsRead < 0) {
            reportError("ERROR reading from socket");
          }
          strcat(buffer, inbuffer); // Append data to buffer
        }

        // Decrypt the message if it starts with 'D'
        if (buffer[0] == 'D') {
          strcpy(buffer, performDecryption(buffer));
        } else {
          strcpy(buffer, "B\neom\n"); // Respond with a basic message otherwise
        }

        // Send the response back to the client
        int size = strlen(buffer);
        for (int charsSent = 0; charsSent < size; ) {
          int charsRead = send(connectionSocket, buffer + charsSent, size - charsSent, 0);
          if (charsRead < 0) {
            reportError("CLIENT: ERROR writing to socket.\n");
          }
          charsSent += charsRead;
        }

        // Close the connection and exit the child process
        close(connectionSocket);
        exit(0);

      default: // Parent process
        wait(NULL); // Wait for the child process to finish
    }
  }

  // Close the listening socket
  close(listenSocket);
  return 0;
}
