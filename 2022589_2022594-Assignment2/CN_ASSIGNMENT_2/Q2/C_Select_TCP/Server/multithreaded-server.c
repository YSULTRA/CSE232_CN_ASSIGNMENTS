#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_PROCESSES 256

struct ProcessInfo {
    char name[256];
    int pid;
    long cpu_time;
};

// Updated get_process_info function
struct ProcessInfo get_process_info(int pid) {
    char path[256];
    struct ProcessInfo pinfo = { .pid = -1, .cpu_time = 0 };

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* stat_file = fopen(path, "r");

    if (stat_file) {
        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), stat_file)) {
            sscanf(buffer, "%d %s %*s %*s %*s %*s %*s %*s %*s %*s %ld %*s",
                   &pinfo.pid, pinfo.name, &pinfo.cpu_time);
            pinfo.name[strlen(pinfo.name) - 1] = '\0'; // Remove trailing ')'
            memmove(pinfo.name, pinfo.name + 1, strlen(pinfo.name)); // Remove leading '('
        }
        fclose(stat_file);
    }
    return pinfo;
}

// Comparison function for sorting processes by CPU time
int compare_process_info(const void* a, const void* b) {
    return ((struct ProcessInfo*)b)->cpu_time - ((struct ProcessInfo*)a)->cpu_time;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set readfds;
    int client_sockets[MAX_CLIENTS] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }

    printf("Listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > max_sd) {
                    max_sd = client_sockets[i];
                }
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                continue;
            }
            printf("New connection accepted\n");

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (FD_ISSET(client_sockets[i], &readfds)) {
                char buffer[BUFFER_SIZE];
                int valread = read(client_sockets[i], buffer, BUFFER_SIZE);
                
                if (valread <= 0) {
                    printf("Client disconnected\n");
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Received request: %s\n", buffer);

                    struct ProcessInfo processes[MAX_PROCESSES];
                    int count = 0;
                    DIR* dir = opendir("/proc");
                    struct dirent* entry;

                    if (dir) {
                        while ((entry = readdir(dir)) != NULL) {
                            if (isdigit(entry->d_name[0])) {
                                int pid = atoi(entry->d_name);
                                struct ProcessInfo pinfo = get_process_info(pid);
                                if (pinfo.pid > 0) {
                                    processes[count++] = pinfo;
                                    if (count >= MAX_PROCESSES) {
                                        break;
                                    }
                                }
                            }
                        }
                        closedir(dir);
                    }

                    qsort(processes, count, sizeof(struct ProcessInfo), compare_process_info);
                    char response[BUFFER_SIZE] = {0};

                    for (int j = 0; j < (count < 2 ? count : 2); ++j) {
                        char temp[256];
                        snprintf(temp, sizeof(temp), "PID: %d, Name: %.100s, CPU Time: %ld\n",processes[j].pid, processes[j].name, processes[j].cpu_time);
                        strncat(response, temp, sizeof(response) - strlen(response) - 1);
                    }


                    if (send(client_sockets[i], response, strlen(response), 0) < 0) {
                        perror("Send failed");
                    }
                }
            }
        }
    }

    return 0;
}
