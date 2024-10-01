#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char *hello = "Hello from client";
    char buffer[1024] = {0};

    // Creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Setting up the server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8002);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        return -1;
    }

    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Sending a message to the server
    send(sock, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // Reading the server's response
    read(sock, buffer, 1024);
    printf("Message received: %s\n", buffer);

    // Closing the socket
    close(sock);

    return 0;
}
