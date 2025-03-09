#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 25664
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    
    // Check if file exists
    const char *filename = "server_file.html";  // Change this to your file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        error("ERROR opening file");
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    
    printf("Server starting...\n");
    printf("File size: %ld bytes\n", filesize);
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        error("ERROR setting socket options");
    }
    
    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }
    
    // Listen for connections
    listen(sockfd, 5);
    printf("Server listening on port %d...\n", PORT);
    
    // Accept a connection
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        error("ERROR on accept");
    }
    
    printf("Connection accepted from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    
    // Prepare HTTP response header
    char http_header[BUFFER_SIZE];
    sprintf(http_header, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"  // Change this based on your file type
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n", filesize);
    
    // Send HTTP header
    if (send(newsockfd, http_header, strlen(http_header), 0) < 0) {
        error("ERROR sending HTTP header");
    }
    printf("HTTP header sent\n");
    
    // Send file content
    size_t bytes_read;
    size_t total_sent = 0;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        size_t bytes_sent = send(newsockfd, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            error("ERROR sending file");
        }
        total_sent += bytes_sent;
        printf("Sent %zu bytes (Total: %zu/%ld)\n", bytes_sent, total_sent, filesize);
    }
    
    printf("File transfer complete. Sent %zu bytes total.\n", total_sent);
    
    // Close file and sockets
    fclose(file);
    close(newsockfd);
    close(sockfd);
    
    printf("Server shutting down.\n");
    return 0;
}