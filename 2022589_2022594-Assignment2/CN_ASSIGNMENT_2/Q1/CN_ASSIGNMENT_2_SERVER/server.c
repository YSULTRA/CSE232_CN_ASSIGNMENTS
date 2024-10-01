#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10000

int serverSocket;
unsigned long totalResponsesSent = 0;

struct ProcessInfo {
    char name[256];
    int pid;
    unsigned long cpuUser;
    unsigned long cpuKernel;
};

// Function to parse the content of /proc/[pid]/stat using open() and read()
void extractProcessInfo(int pid, struct ProcessInfo *process) {
    char path[256], buffer[BUFFER_SIZE];
    int fd, bytesRead;

    sprintf(path, "/proc/%d/stat", pid);
    fd = open(path, O_RDONLY);
    if (fd == -1) return;

    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        close(fd);
        return;
    }
    buffer[bytesRead] = '\0';
    close(fd);

    sscanf(buffer, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %lu %lu",
           &process->pid, process->name, &process->cpuUser, &process->cpuKernel);
}

void getTopTwoProcesses(struct ProcessInfo *topProcesses) {
    unsigned long maxCpu1 = 0, maxCpu2 = 0;
    struct ProcessInfo process1 = {0}, process2 = {0}, currentProcess = {0};

    DIR *dir;
    struct dirent *entry;

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Unable to open /proc directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            extractProcessInfo(atoi(entry->d_name), &currentProcess);
            unsigned long totalCpu = currentProcess.cpuUser + currentProcess.cpuKernel;

            if (totalCpu > maxCpu1) {
                process2 = process1;
                maxCpu2 = maxCpu1;
                process1 = currentProcess;
                maxCpu1 = totalCpu;
            } else if (totalCpu > maxCpu2) {
                process2 = currentProcess;
                maxCpu2 = totalCpu;
            }
        }
    }

    closedir(dir);
    topProcesses[0] = process1;
    topProcesses[1] = process2;
}

void sendProcessInfo(int clientSocket) {
    struct ProcessInfo topProcesses[2];
    getTopTwoProcesses(topProcesses);

    char buffer[BUFFER_SIZE];
    sprintf(buffer, "Process 1: %s (PID: %d, CPU User: %lu, CPU Kernel: %lu)\n"
                    "Process 2: %s (PID: %d, CPU User: %lu, CPU Kernel: %lu)\n",
            topProcesses[0].name, topProcesses[0].pid, topProcesses[0].cpuUser, topProcesses[0].cpuKernel,
            topProcesses[1].name, topProcesses[1].pid, topProcesses[1].cpuUser, topProcesses[1].cpuKernel);

    send(clientSocket, buffer, strlen(buffer), 0);
    __sync_fetch_and_add(&totalResponsesSent, 1);
}

void *handleClient(void *socketDesc) {
    int clientSocket = *(int *)socketDesc;
    sendProcessInfo(clientSocket);
    close(clientSocket);
    free(socketDesc);
    return NULL;
}

void handleSignalInterrupt(int sig) {
    printf("\nCaught signal %d. Closing server socket and exiting...\n", sig);
    printf("\nAnalytics Report:\n");
    printf("Total responses sent: %lu\n", totalResponsesSent);

    if (serverSocket > 0) {
        close(serverSocket);
        printf("Server socket closed.\n");
    }
    exit(0);
}

void setupServerSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
}

void acceptClients() {
    int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in address;

    while (1) {
        int newSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (newSocket < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Client connected from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        pthread_t clientThread;
        int *newSock = malloc(1);
        *newSock = newSocket;

        if (pthread_create(&clientThread, NULL, handleClient, (void *)newSock) < 0) {
            perror("Could not create thread");
            free(newSock);
            exit(EXIT_FAILURE);
        }

        pthread_detach(clientThread);
    }
}

int main() {
    signal(SIGINT, handleSignalInterrupt);
    setupServerSocket();
    printf("Server is listening on port %d...\n", PORT);
    acceptClients();
    return 0;
}
