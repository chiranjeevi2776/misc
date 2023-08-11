#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 20  // Timeout in seconds

int main() {
    int sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    struct timeval start_time;
    struct timeval current_time;
    struct timeval timeout;
    fd_set readfds;
    int ret;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("Listen failed");
        close(sockfd);
        return 1;
    }

    printf("TCP server is listening on port %d\n", SERVER_PORT);

    while (1) {
        socklen_t client_addr_len = sizeof(client_addr);

        new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_sockfd < 0) {
            perror("Accept failed");
            close(sockfd);
            return 1;
        }

        gettimeofday(&start_time, NULL);

        while (1) {
            int remaining_time;

            gettimeofday(&current_time, NULL);
            remaining_time = (TIMEOUT_SEC * 1000) - ((current_time.tv_sec - start_time.tv_sec) * 1000 +
                                                       (current_time.tv_usec - start_time.tv_usec) / 1000);

            if (remaining_time <= 0) {
                printf("Timeout: No data received within %d seconds\n", TIMEOUT_SEC);
                break;  // Exit the loop after timeout
            }

            timeout.tv_sec = remaining_time / 1000;
            timeout.tv_usec = (remaining_time % 1000) * 1000;

            FD_ZERO(&readfds);
            FD_SET(new_sockfd, &readfds);

            ret = select(new_sockfd + 1, &readfds, NULL, NULL, &timeout);

            if (ret == 0) {
                // Timeout occurred, no data received
                printf("Timeout: No data received within %d seconds\n", TIMEOUT_SEC);
                break;  // Exit the loop after timeout
            } else if (ret < 0) {
                perror("select() error");
                break;
            }

            if (FD_ISSET(new_sockfd, &readfds)) {
                int bytes_received = recv(new_sockfd, buffer, sizeof(buffer), 0);

                if (bytes_received > 0) {
                    printf("Received data: %s\n", buffer);
                } else if (bytes_received == 0) {
                    printf("Connection closed by client\n");
                    break;
                } else if (bytes_received < 0) {
                    perror("Receive failed");
                    break;
                }
            }
        }

        close(new_sockfd);
    }

    close(sockfd);
    return 0;
}

