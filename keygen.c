#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// The main function of the program
int main(int argc, char *argv[]) {
    // Initialize the random number generator using the current time
    srand((unsigned int)time(NULL));

    int keyLength; // Variable to store the length of the key
    
    // Check if the number of arguments is correct
    if (argc < 2) {
        // Print usage instructions if the required argument is missing
        fprintf(stderr, "Usage: %s <key_length>\n", argv[0]);
        return 1; // Return with an error code
    }
    
    // Convert the key length argument from string to integer
    keyLength = atoi(argv[1]);
    // Check if the key length is a positive number
    if (keyLength <= 0) {
        // Print an error message if key length is not positive
        fprintf(stderr, "Error: Key length must be positive.\n");
        return 1; // Return with an error code
    }

    // Generate and print the random key
    for (int i = 0; i < keyLength; i++) {
        // Generate a random uppercase character
        char randomChar = 'A' + (rand() % 26);
        // Randomly decide whether to convert it to lowercase
        if (rand() % 2 == 0) {
            randomChar = 'a' + (randomChar - 'A'); // Convert to lowercase
        }
        printf("%c", randomChar); // Print the character
    }
    printf("\n"); // Print a newline character at the end

    return 0; // Return success
}
