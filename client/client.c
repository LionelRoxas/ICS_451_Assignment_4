#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 25664
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Function to parse Content-Length from HTTP header
long parse_content_length(const char *header) {
    const char *ptr = strstr(header, "Content-Length:");
    if (ptr) {
        ptr += 15; // Length of "Content-Length:"
        while (*ptr == ' ') ptr++; // Skip spaces
        return atol(ptr);
    }
    return -1; // Not found
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    
    printf("Client starting...\n");
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    
    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        error("ERROR invalid address");
    }
    
    // Connect to server
    printf("Connecting to server at %s:%d...\n", SERVER_IP, PORT);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting to server");
    }
    
    printf("Connected to server!\n");
    
    // Open output file
    FILE *file = fopen("file_client.html", "wb");  // Change extension based on server file
    if (file == NULL) {
        error("ERROR opening file for writing");
    }
    
    // Receive HTTP header
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t bytes_received = 0;
    ssize_t header_end = 0;
    ssize_t total_header_size = 0;
    
    // Receive until we find the end of header marker "\r\n\r\n"
    while ((bytes_received = recv(sockfd, buffer + total_header_size, 
                                BUFFER_SIZE - total_header_size - 1, 0)) > 0) {
        total_header_size += bytes_received;
        buffer[total_header_size] = '\0';
        
        // Search for end of HTTP header
        char *header_end_ptr = strstr(buffer, "\r\n\r\n");
        if (header_end_ptr) {
            header_end = header_end_ptr - buffer + 4;  // +4 for "\r\n\r\n"
            break;
        }
        
        // If buffer is full and still no end of header, that's an error
        if (total_header_size >= BUFFER_SIZE - 1) {
            error("ERROR HTTP header too large");
        }
    }
    
    if (bytes_received < 0) {
        error("ERROR receiving HTTP header");
    }
    
    // Extract and display the HTTP header
    printf("Received HTTP header:\n%.*s\n", (int)header_end, buffer);
    
    // Parse content length
    long content_length = parse_content_length(buffer);
    printf("Content-Length: %ld bytes\n", content_length);
    
    // Write any data already received after the header
    size_t data_received = total_header_size - header_end;
    if (data_received > 0) {
        fwrite(buffer + header_end, 1, data_received, file);
        printf("Received %zu bytes of file data with header\n", data_received);
    }
    
    // Continue receiving the file
    size_t total_received = data_received;
    
    while (total_received < content_length && 
           (bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
        printf("Received %zd bytes (Total: %zu/%ld)\n", 
               bytes_received, total_received, content_length);
    }
    
    if (bytes_received < 0) {
        error("ERROR receiving file");
    }
    
    // Close file and socket
    fclose(file);
    close(sockfd);
    
    printf("File transfer complete. Received %zu bytes total.\n", total_received);
    printf("File saved as file_client.html\n");
    printf("Client shutting down.\n");
    
    return 0;
}