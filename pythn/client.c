#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main() {
    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(12345);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server\n");

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 60) {
        char message[100];
        snprintf(message, sizeof(message), "Message at %ld", time(NULL));
        send(client_socket, message, strlen(message), 0);
        sleep(1);  // Send a message every 1 second
    }

    // Send an empty message to signal the server to close
    send(client_socket, "", 0, 0);
    printf("Sent empty message to close the server socket.\n");

    // Clean up
    close(client_socket);
    printf("Client socket is closed.\n");

    return 0;
}

