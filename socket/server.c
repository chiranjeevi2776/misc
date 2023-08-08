#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <math.h>
#include "common.h"

#define SERVER_IP "192.168.1.2"  // Replace with the IP address of the server
#define UDP_PORT 6788      // Replace with the port number used by the server
#define MAXLINE 1024

#define TCP_PORT 6789
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE] = {0};
struct sockaddr_in servaddr, cliaddr;
struct server_report report;

int udp_server(void) 
{
	int sockfd;
	char *hello = "Hello";
	int len, bytes_received, client_addr_len;
	struct timeval start_time, end_time;
	double jitter_sum = 0.0;
	struct timeval prev_packet_time, curr_packet_time;	

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

	len = sizeof(cliaddr); //len is value/result

	memset((char *)&report, 0, sizeof(struct server_report));
	printf("UDP SERVER STARTED\n");

	gettimeofday(&start_time, NULL);
	gettimeofday(&prev_packet_time, NULL);

	while(1) {
		bytes_received = recvfrom(sockfd, (char *)buffer, MAXLINE,
				MSG_WAITALL, (struct sockaddr *)&cliaddr,
				&len);
		if(bytes_received < 0)
		{
			printf("UDP_SERVER: Rcv Failed\n");
			exit(EXIT_FAILURE);
		} else if(bytes_received == 0) {
			printf("UDP_SERVER: End of Transmission\n");
			break;
		}

		report.bytes_received += bytes_received;
		report.packets_received++;

		gettimeofday(&curr_packet_time, NULL);

		// Calculate jitter for all packets after the first one
		if (report.packets_received > 1) {
			double curr_jitter = fabs((curr_packet_time.tv_sec - prev_packet_time.tv_sec) * 1000.0 +
					(curr_packet_time.tv_usec - prev_packet_time.tv_usec) / 1000.0);
			jitter_sum += curr_jitter;
		}

		prev_packet_time = curr_packet_time;

		//buffer[bytes_received] = '\0';
		//printf("Message client : %s\n", buffer);
	}

	/* server run time */
	gettimeofday(&end_time, NULL);
	report.elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;	
	report.throughput = (double)report.bytes_received / (1024 * 1024) / report.elapsed_time;
	report.average_jitter = (report.packets_received > 1) ? (jitter_sum / (report.packets_received - 1)) : 0.0;



	close(sockfd);
	return 0;
}

int udp_client()
{
	printf("UDP Client Not Yet Implemented\n");
}

int tcp_server()
{
	printf("TCP Server Not Yet Implemented\n");
}

int tcp_client()
{
	printf("TCP Client Not Yet Implemented\n");
}

int send_report(int sock)
{
	printf("Sending Report to the Client\n");
	printf("Bytes rcvd %d Pkts rcvd %d\n", report.bytes_received, report.packets_received);
	report.bytes_received = htonl(report.bytes_received);
	report.packets_received = htonl(report.packets_received);

	// Send the report to the client
	send(sock, &report, sizeof(struct server_report), 0);
}	

/* Server will continuously listens client cmds and process the cmds
 * starting udp serer/client
 * starting tcp server/client
 * sending periodic/continuous traffics
 * sendig reports to the client
 */
int wait_for_cmd(int sock)
{
	int valread;
	struct cmd *cf;

	printf("Waiting for Client CMD\n");
	valread = read(sock, buffer, BUFFER_SIZE);

	printf("Client CMD received\n");
	cf = (struct cmd *)buffer;
	printf("\tClient Role %d\n", ntohl(cf->client_role));
	printf("\tTraffic type %d\n", ntohl(cf->traffic_type));
	printf("\tTraffic Mode %d\n", ntohl(cf->traffic_mode));
	printf("\tDuration %d\n", ntohl(cf->duration));
	printf("\tFrame len %d\n", ntohl(cf->frame_len));
	printf("\treserved %d\n\n", ntohl(cf->reserved));

	/* Client ==> Server */
	if(ntohl(cf->client_role) == UPLINK)
	{
		printf("UPLINK:\n\t");
		if(ntohl(cf->traffic_type) == UDP)
		{
			printf("UDP SERVER STARTED\n\t");
			udp_server();
		} else if(ntohl(cf->traffic_type) == TCP) {
			printf("TCP SERVER STARTED\n\t");
			tcp_server();
		}
	} else if(ntohl(cf->client_role) == DOWNLINK) {
		printf("DOWNLINK:\n\t");
		if(ntohl(cf->traffic_type) == UDP)
		{
			printf("UDP CLIENT STARTED\n\t");
			udp_client();
		} else if(ntohl(cf->traffic_type) == TCP) {
			printf("TCP CLEINT STARTED\n\t");
			tcp_client();
		}
	}

	send_report(sock);

	return 0;
}

int init_tcp() 
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

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

	/* Accept a new connection */
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("Accept failed");
		exit(EXIT_FAILURE);
	}

	printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

	/* wait for client cmds */
	while(1)
	{
		wait_for_cmd(new_socket);
		sleep(10);
	}

	/* Close the socket for this client */
	close(new_socket);

	return 0;
}

int main() {

    init_tcp();

    return 0;
}
