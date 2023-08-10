#include <zephyr.h>
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 20  // Timeout in seconds

void main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    struct timeval start_time;
    struct k_poll_event events[1];
    int ret;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        printk("Failed to create socket\n");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printk("Bind failed\n");
        close(sockfd);
        return;
    }

    printf("UDP server is listening on port %d\n", SERVER_PORT);

    gettimeofday(&start_time, NULL);

    while (1) {
        struct timeval current_time;
        k_timeout_t remaining_time;
        k_timeout_t timeout;

        remaining_time = k_timeout_subtract(k_uptime_get(), start_time, timeout);

        if (remaining_time <= 0) {
            printf("Timeout: No data received within %d seconds\n", TIMEOUT_SEC);
            break;  // Exit the loop after timeout
        }

        // Use k_poll() for non-blocking event polling
        events[0].obj = &sockfd;
        events[0].type = K_POLL_TYPE_DATA_AVAILABLE;

        ret = k_poll(events, 1, K_MSEC(100));  // Poll every 100ms

        if (ret == 0) {
            // Timeout occurred, no data received
            continue;
        } else if (ret < 0) {
            printk("k_poll() error\n");
            break;
        }

        socklen_t client_addr_len = sizeof(server_addr);
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                      (struct sockaddr *)&server_addr, &client_addr_len);

        if (bytes_received > 0) {
            printf("Received data: %s\n", buffer);

            // Add your processing logic here

            // Check for end-of-reception signal
            if (strcmp(buffer, "end") == 0) {
                printf("End of reception signal received\n");
                break;  // Exit the loop when end signal is received
            }
        } else if (bytes_received == 0) {
            printf("Received empty frame\n");
        } else if (bytes_received < 0) {
            printk("Receive failed\n");
        }
    }

    close(sockfd);
}

