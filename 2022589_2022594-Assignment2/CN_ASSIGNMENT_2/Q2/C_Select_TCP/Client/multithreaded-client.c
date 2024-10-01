#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void* client_thread(void* arg) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    const char* message = "Get CPU process info";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return NULL;
    }

    printf("Connected to server. Sending request...\n");

    send(sock, message, strlen(message), 0);
    printf("Request sent: %s\n", message);

    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Server replied:\n%s\n", buffer);
    }

    close(sock);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_clients>\n", argv[0]);
        return -1;
    }

    int num_clients = atoi(argv[1]);

    if (num_clients <= 0) {
        fprintf(stderr, "Number of clients must be a positive integer.\n");
        return -1;
    }

    pthread_t* threads = (pthread_t*)malloc(num_clients * sizeof(pthread_t));

    for (int i = 0; i < num_clients; ++i) {
        if (pthread_create(&threads[i], NULL, client_thread, NULL) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return -1;
        }
    }

    for (int i = 0; i < num_clients; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}
