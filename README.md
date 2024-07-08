# One-Time Pad Encryption System

This project implements a secure communication system using the One-Time Pad encryption technique. It consists of five main components:

## Components

1. **Key Generator (keygen.c)**
   - Generates random keys of specified length
   - Produces uppercase letters A-Z and spaces

2. **Encryption Client (enc_client.c)**
   - Reads plaintext and key from files
   - Validates input characters (A-Z and space)
   - Connects to the encryption server
   - Sends plaintext and key for encryption
   - Receives and outputs the ciphertext

3. **Encryption Server (enc_server.c)**
   - Listens for client connections
   - Performs encryption using the One-Time Pad method
   - Handles multiple client requests concurrently using forking

4. **Decryption Client (dec_client.c)**
   - Reads ciphertext and key from files
   - Validates input characters
   - Connects to the decryption server
   - Sends ciphertext and key for decryption
   - Receives and outputs the decrypted plaintext

5. **Decryption Server (dec_server.c)**
   - Listens for client connections
   - Performs decryption using the One-Time Pad method
   - Handles multiple client requests concurrently using forking

## Key Features

- Implements socket programming for client-server communication
- Uses `fork()` for concurrent processing of multiple client requests
- Ensures secure communication by validating input and using One-Time Pad encryption
- Handles large messages (up to 800,000 characters)
- Implements error handling and reporting
