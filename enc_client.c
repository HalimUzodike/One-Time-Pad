#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <fcntl.h>
#include <ctype.h>

// Function to display error messages and exit the program
void reportError(const char *msg) { 
  perror(msg); // Prints the system error message
  exit(1); // Exits with a status code indicating an error
} 

// Function to initialize the server address structure
void initServerAddress(struct sockaddr_in* address, int portNumber, char* hostname) {
  memset(address, '\0', sizeof(*address)); // Clears the structure
  address->sin_family = AF_INET; // Sets the address family to IPv4
  address->sin_port = htons(portNumber); // Sets the port number, converting it to network byte order

  struct hostent* hostInfo = gethostbyname(hostname); // Resolves the hostname to an IP address
  if (!hostInfo) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }

  memcpy(&address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length); // Copies the IP address
}

// Function to validate that characters in plaintext and key files are allowed
bool validateCharacters(const char *plaintext, const char *key) {
  char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "; // Allowed characters
  FILE *fp;
  char ch;

  // Validate plaintext
  fp = fopen(plaintext, "r"); // Opens the plaintext file
  if (!fp) return false;

  while ((ch = fgetc(fp)) != EOF && ch != '\n') { // Reads characters until EOF or newline
    if (!strchr(chars, ch)) { // Checks if the character is in the allowed list
      fclose(fp);
      return false;
    }
  }
  fclose(fp);

  // Validate key
  fp = fopen(key, "r"); // Opens the key file
  if (!fp) return false;

  while ((ch = fgetc(fp)) != EOF && ch != '\n') { // Reads characters until EOF or newline
    if (!strchr(chars, ch)) { // Checks if the character is in the allowed list
      fclose(fp);
      return false;
    }
  }
  fclose(fp);
  return true;
}

// Function to create a message by concatenating plaintext and key
void createMessage(char *buffer, const char *plaintext, const char *key) {
  FILE *fp;
  char line[800000]; // Buffer for reading from files

  strcpy(buffer, "E\n"); // Starts the buffer with 'E' indicating encryption request

  // Append plaintext to the buffer
  fp = fopen(plaintext, "r");
  fgets(line, sizeof(line), fp); // Reads a line from the plaintext file
  fclose(fp);
  strcat(buffer, line);
  strcat(buffer, "\n");

  // Append key to the buffer
  fp = fopen(key, "r");
  fgets(line, sizeof(line), fp); // Reads a line from the key file
  fclose(fp);
  strcat(buffer, line);
  strcat(buffer, "\neom\n"); // Adds end-of-message marker
}

int main(int argc, char *argv[]) {
  int socketFD, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[800000]; // Buffer for sending and receiving data

  if (argc < 3) { 
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]); // Usage instructions
    exit(1); 
  } 

  // Opening plaintext and key files to compare their sizes
  int plainLength = open(argv[1], O_RDONLY);
  int keyLength = open(argv[2], O_RDONLY);

  // Check if key is long enough
  if (lseek(plainLength, 0 , SEEK_END) > lseek(keyLength, 0, SEEK_END)) {
    reportError("Error: Key is too short");
  }

  // Validate the characters in the plaintext and key
  if (!validateCharacters(argv[1], argv[2])) {
    reportError("CLIENT: Invalid character in files");
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0) reportError("CLIENT: ERROR opening socket");

  // Initialize server address
  initServerAddress(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to the server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    reportError("CLIENT: ERROR connecting");

  // Create a message from the plaintext and key
  createMessage(buffer, argv[1], argv[2]);

  // Send the message to the server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0) reportError("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

  // Prepare to receive a response
  memset(buffer, '\0', sizeof(buffer)); // Clear the buffer

  // Read response from the server
  while (!strstr(buffer, "eom\n")) {
    char inbuffer[800000] = {'\0'};
    charsRead = recv(socketFD, inbuffer, sizeof(inbuffer), 0); 
    if (charsRead < 0) reportError("CLIENT: ERROR reading from socket");
    strcat(buffer, inbuffer); // Append received data to buffer
  }

  // Check if the response is valid and print the decrypted message
  char *from = strtok(buffer, "\n");
  if (from && from[0] == 'B') reportError("Error: Message not from enc\n");
  else fprintf(stdout, "%s\n", strtok(NULL, "\n"));

  // Close the socket and end the program
  close(socketFD); 
  return 0;
}
