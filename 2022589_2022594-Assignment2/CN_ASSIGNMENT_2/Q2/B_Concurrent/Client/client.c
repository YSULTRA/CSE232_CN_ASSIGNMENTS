#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define LOG_FILE "client_log.txt"

FILE *logFile;
int totalResponseCount = 0;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

void logMessage(const char *message) {
    fprintf(logFile, "%s", message);
    fflush(logFile);
    printf("%s", message);
}

void *clientTask(void *arg) {
    int clientSocket = 0;
    struct sockaddr_in serverAddress;
    char receiveBuffer[BUFFER_SIZE] = {0};

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        logMessage("Socket creation error\n");
        return NULL;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        logMessage("Invalid address\n");
        close(clientSocket);
        return NULL;
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        logMessage("Connection failed\n");
        close(clientSocket);
        return NULL;
    }

    int bytesRead = read(clientSocket, receiveBuffer, BUFFER_SIZE);
    if (bytesRead > 0) {
        pthread_mutex_lock(&logMutex);
        totalResponseCount++;
        char logEntry[BUFFER_SIZE + 100];
        snprintf(logEntry, sizeof(logEntry), "Thread ID: %ld, Response Number: %d\nServer Response:\n%s\n", 
                 pthread_self(), totalResponseCount, receiveBuffer);
        logMessage(logEntry);
        pthread_mutex_unlock(&logMutex);
    } else {
        logMessage("Error reading from server\n");
    }

    close(clientSocket);
    return NULL;
}

void createClientThreads(int numClients) {
    pthread_t clientThreads[numClients];
    int threadIds[numClients];

    for (int i = 0; i < numClients; i++) {
        threadIds[i] = i;
        if (pthread_create(&clientThreads[i], NULL, clientTask, (void *)&threadIds[i]) != 0) {
            perror("Could not create client thread");
            fclose(logFile);
            exit(1);
        }
    }

    for (int i = 0; i < numClients; i++) {
        pthread_join(clientThreads[i], NULL);
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_clients>\n", argv[0]);
        return -1;
    }
    
    logFile = fopen(LOG_FILE, "w");
    if (logFile == NULL) {
        perror("Error opening log file");
        return 1;
    }

    int numberOfClients = atoi(argv[1]);
    createClientThreads(numberOfClients);

    fclose(logFile);
    return 0;
}
