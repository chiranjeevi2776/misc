#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

#define SERVER_IP "192.168.1.2"  // Replace with the IP address of the server
#define UDP_PORT 6788      // Replace with the port number used by the server
#define MAXLINE 1024

#define TCP_PORT 6789
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE] = {0};

int udp_server(void) 
{

          int sockfd;
          char *hello = "Hello";
          struct sockaddr_in servaddr, cliaddr;

          // Creating socket file descriptor
          if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                  printf("socket creation failed \n");
		  exit(EXIT_FAILURE);
          }

          memset(&servaddr, 0, sizeof(servaddr));
          memset(&cliaddr, 0, sizeof(cliaddr));

          // Filling server information
          servaddr.sin_family = AF_INET; // IPv4
          servaddr.sin_addr.s_addr = INADDR_ANY;
          servaddr.sin_port = htons(UDP_PORT);

          // Bind the socket with the server address
          if ( bind(sockfd, (const struct sockaddr *)&servaddr,
                                  sizeof(servaddr)) < 0 )
          {
                  printf("bind failed \n");
		  exit(EXIT_FAILURE);
          }

          int len, n;
          len = sizeof(cliaddr); //len is value/result

          printf("UDP SERVER STARTED\n");
          while(1) {
                  n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                                  MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                                  &len);
                  buffer[n] = '\0';
                  printf("Message client : %s\n", buffer);
          }

          return 0;
  }

int wait_for_control_frame(int sock)
{
    int valread;
    struct control *cf;

    valread = read(sock, buffer, BUFFER_SIZE);
    cf = (struct control *)buffer;
    printf("Mode %d\n", ntohl(cf->mode));
    printf("Duration %d\n", ntohl(cf->duration));
    printf("type %d\n", ntohl(cf->type));
    printf("packet len %d\n", ntohl(cf->packet_len));
    printf("reserved %d\n", ntohl(cf->reserved));
}

int init_tcp() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    const char* message = "Server received your message: ";

    /*  Create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Set socket options */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    /* Bind socket to the specified address and port */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    /* Listen for incoming connections */
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", TCP_PORT);

//    while(1)
    {
        /* Accept a new connection */
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

	    wait_for_control_frame(new_socket);

        /* Read data from the client */
        valread = read(new_socket, buffer, BUFFER_SIZE);
        printf("Received: %s\n", buffer);

        /* Send back a response to the client */
        send(new_socket, message, strlen(message), 0);
        send(new_socket, buffer, strlen(buffer), 0);

        /* Close the socket for this client */
        close(new_socket);
    }

    return 0;
}

int main() {

    init_tcp();

//    udp_server();

    return 0;
}
