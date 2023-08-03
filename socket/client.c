#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

#define SERVER_IP "127.0.0.1"  // Replace with the IP address of the server
#define SERVER_PORT 6788      // Replace with the port number used by the server

#define TCP_PORT 6789
#define BUFFER_SIZE 1024

char buffer[1024];
unsigned char packet[1024];
unsigned int nb_packets = 0, packet_size = 100;
int sockfd;

int init_tcp();

int send_control_frame(int sock)
{
    struct control cf;
    cf.mode = htonl(MODE_TX);
    cf.duration = htonl(10);
    cf.type = htonl(TYPE_UDP);
    cf.packet_len = htonl(100);
    cf.reserved = 0;

    send(sock, &cf, sizeof(struct control), 0);
    printf("Control frame sent to server\n");
}

#if 0
void prepare_packet()
{
	struct timeval tv;
	struct zperf_udp_datagram *datagram;
	struct zperf_client_hdr_v1 *hdr;
	uint64_t usecs64;
	uint32_t secs, usecs; 
	struct zperf_results results;

	gettimeofday(&tv, NULL);
	// Get seconds
	secs = tv.tv_sec;
	// Get microseconds
	usecs = tv.tv_usec;

	/* Fill the packet header */
	memset(&packet, 'z', sizeof(packet));
	datagram = (struct zperf_udp_datagram *)packet;

	if(nb_packets == 5000 )
		datagram->id = htonl(-nb_packets);
	else
	datagram->id = htonl(nb_packets);
	datagram->tv_sec = htonl(secs);
	datagram->tv_usec = htonl(usecs);

	hdr = (struct zperf_client_hdr_v1 *)(packet + sizeof(*datagram));
	hdr->flags = 0;
	hdr->num_of_threads = htonl(1);
	hdr->port = htonl(SERVER_PORT);
	hdr->buffer_len = sizeof(packet) -
		sizeof(*datagram) - sizeof(*hdr);
	hdr->bandwidth = 0;//htonl(rate_in_kbps);
	hdr->num_of_bytes = htonl(packet_size);
}
#endif

int init_udp() 
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);
    //struct zperf_results results;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));
    

    // Configure client address
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);  // Bind to any available port

    // Bind the socket to the client address
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    // Send data to the server
    while(1)
    {
    //printf("Enter message: ");
    //fgets(buffer, sizeof(buffer), stdin);

    //prepare_packet();

    sendto(sockfd, packet, packet_size, 0, (struct sockaddr *)&server_addr, addr_len);

    nb_packets++;

    if(nb_packets == 5001)
	    break;

    usleep(1000);
    }

    printf("Waiting for server response\n");
    memset(buffer, 0, sizeof(buffer));
    ssize_t num_bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (num_bytes < 0) {
        perror("recvfrom failed");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return 0;
}

int init_tcp() 
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    const char* message = "Hello, Server!";

    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TCP_PORT);

    /* Convert IPv4 and IPv6 addresses from text to binary form */
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected To Server \n");
    send_control_frame(sock);

    /* Send a message to the server */
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);

    /* Receive a response from the server */
    read(sock, buffer, BUFFER_SIZE);
    printf("Server response: %s\n", buffer);

    close(sock);
    return 0;
}

int main()
{
    init_tcp();

//    init_udp();

}
