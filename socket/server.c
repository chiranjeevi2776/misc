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

#define SERVER_IP "192.168.1.211"  // Replace with the IP address of the server
#define DATA_PORT 6788      // Replace with the port number used by the server
#define MAXLINE 1024

#define TCP_PORT 6789
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE] = {0};
struct sockaddr_in servaddr, cliaddr;
struct server_report report;
struct cmd global_cmd;
int tcp_socket = 0;

double network_order_to_double(uint64_t value) {
    double result;
    uint64_t beValue = be64toh(value);
    memcpy(&result, &beValue, sizeof(double));
    return result;
}

uint64_t double_to_network_order(double value) {
    uint64_t result;
    memcpy(&result, &value, sizeof(uint64_t));
    return htobe64(result);
}

int udp_server(void) 
{
	int sockfd;
	char *hello = "Hello";
	int len, bytes_received, client_addr_len;
	struct timeval start_time, end_time;
	double jitter_sum = 0.0, throughput = 0.0;
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
	servaddr.sin_port = htons(DATA_PORT);

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
			printf("UDP_SERVER: End of Rcv from client\n");
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
	report.elapsed_time = double_to_network_order((end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0);	
	throughput = (double)(report.bytes_received * 8)/ (1000000 * ntohl(global_cmd.duration));
#if 0
	printf("ELAPSED TIME %f \n",  (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0);
	printf("Jitter %f \n", ((report.packets_received > 1) ? (jitter_sum / (report.packets_received - 1)) : 0.0));
	printf("Throughput %f \n", throughput);
#endif
	report.throughput = double_to_network_order(throughput);
	report.average_jitter = double_to_network_order((report.packets_received > 1) ? (jitter_sum / (report.packets_received - 1)) : 0.0);
	close(sockfd);
	return 0;
}

int udp_client()
{
	int sockfd, pkt_count = 0;
	int total_duration = ntohl(global_cmd.duration); //Converting into ms
	socklen_t addr_len = sizeof(servaddr);
	struct timeval start_time, current_time;
	double elapsed_time;

	printf("UDP Clinet Started\n");
     
	// Create UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Configure client address
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = INADDR_ANY;
	cliaddr.sin_port = htons(0);  // Bind to any available port

	// Bind the socket to the client address
	if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
		printf("bind failed");
		exit(EXIT_FAILURE);
	}

	// Configure server address
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(DATA_PORT);
	if (inet_pton(AF_INET, SERVER_IP, &(servaddr.sin_addr)) <= 0) {
		perror("inet_pton failed");
		exit(EXIT_FAILURE);
	}
       
	// Get the current time
	gettimeofday(&start_time, NULL);

	memset(buffer, 'A', BUFFER_SIZE);

	// Send data for given seconds
	while (1) {
		sendto(sockfd, buffer, ntohl(global_cmd.frame_len), 0, (struct sockaddr *)&servaddr, addr_len);
		gettimeofday(&current_time, NULL);
		elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

		pkt_count++;
		if (elapsed_time >= total_duration) {
			break;
		}

		if(ntohl(global_cmd.traffic_mode) == CONTINUOUS)
			usleep(2000);
		else
			usleep(10*1000); /* 10msec */
	}

	printf("No of pkts sent %d \n", pkt_count);
	/* Send Empty Msg to indicate End of TX */
	{
		char empty_data = '\0';
		sleep(1);
		sendto(sockfd, &empty_data, 0, 0, (struct sockaddr *)&servaddr, addr_len);
		sendto(sockfd, &empty_data, 0, 0, (struct sockaddr *)&servaddr, addr_len);
		sleep(1);
		sendto(sockfd, &empty_data, 0, 0, (struct sockaddr *)&servaddr, addr_len);
		sendto(sockfd, &empty_data, 0, 0, (struct sockaddr *)&servaddr, addr_len);
	}

	// Close the socket
	close(sockfd);

	return 0;
}

int tcp_server()
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	int bytes_received, client_addr_len;
	struct timeval start_time, end_time;
	double jitter_sum = 0.0, throughput = 0.0;
	struct timeval prev_packet_time, curr_packet_time;	

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
	address.sin_port = htons(DATA_PORT);

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

	printf("Server is listening on port %d\n", DATA_PORT);

	/* Accept a new connection */
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("Accept failed");
		exit(EXIT_FAILURE);
	}

	printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

	memset((char *)&report, 0, sizeof(struct server_report));
	gettimeofday(&start_time, NULL);
	gettimeofday(&prev_packet_time, NULL);

	while(1)
	{
		// Receive data from client
		bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
		if(bytes_received < 0)
		{
			printf("TCP_SERVER: Rcv Failed\n");
			exit(EXIT_FAILURE);
		} else if(bytes_received == 0) {
			printf("TCP_SERVER: End of Recv from client \n");
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
	}

	/* server run time */
	gettimeofday(&end_time, NULL);
	report.elapsed_time = double_to_network_order((end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0);	
	throughput = (double)(report.bytes_received * 8)/ (1000000 * ntohl(global_cmd.duration));
#if 0
	printf("ELAPSED TIME %f \n",  (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0);
	printf("Jitter %f \n", ((report.packets_received > 1) ? (jitter_sum / (report.packets_received - 1)) : 0.0));
	printf("Throughput %f \n", throughput);
#endif
	report.throughput = double_to_network_order(throughput);
	report.average_jitter = double_to_network_order((report.packets_received > 1) ? (jitter_sum / (report.packets_received - 1)) : 0.0);

	close(new_socket);

	return 0;
}

int tcp_client()
{
	int sockfd;
	struct sockaddr_in server_addr;
	struct timeval start_time, current_time;
	double elapsed_time;
	int total_duration = ntohl(global_cmd.duration); //Converting into ms

	// Create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(DATA_PORT);

	// Connect to the server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	printf("TCP client is connected to %s:%d\n", SERVER_IP, DATA_PORT);

	// Initialize the buffer with some data
	memset(buffer, 'A', BUFFER_SIZE);

	// Get the current time
	gettimeofday(&start_time, NULL);

	// Send data for given seconds
	while (1) {
		send(sockfd, buffer, ntohl(global_cmd.frame_len), 0);
		gettimeofday(&current_time, NULL);
		elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

		if (elapsed_time >= total_duration) {
			break;
		}

		if(ntohl(global_cmd.traffic_mode) == CONTINUOUS)
			usleep(1000);
		else
			usleep(10*1000); /* 10msec */
	}

	/* Send Empty Msg to indicate End of TX */
	{
		char empty_data = '\0';
		sleep(1);
		send(sockfd, &empty_data, 0, 0);
		send(sockfd, &empty_data, 0, 0);
		sleep(1);
		send(sockfd, &empty_data, 0, 0);
		send(sockfd, &empty_data, 0, 0);
		sleep(1);
		send(sockfd, &empty_data, 0, 0);
		send(sockfd, &empty_data, 0, 0);
	}


	printf("Data sent for %d seconds\n", total_duration);

	close(sockfd);
	return 0;
}

int send_report(int sock)
{
	printf("Sending Report to the Client\n");
	printf("Bytes rcvd %d Pkts rcvd %d\n", report.bytes_received, report.packets_received);
	
	report.bytes_received = htonl(report.bytes_received);
	report.packets_received = htonl(report.packets_received);
#if 0
	report.elapsed_time = double_to_network_order(report.elapsed_time);
	report.throughput = double_to_network_order(report.throughput);
	report.average_jitter = double_to_network_order(report.average_jitter);
#endif

	printf("Elapsed Time %.2f Seconds\n\t",network_order_to_double(report.elapsed_time));
	printf("Throuhput %.2f Mbps\n\t",network_order_to_double(report.throughput));
	printf("Average Jitter %.2f ms\n\t",network_order_to_double(report.average_jitter));
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

	memcpy((uint8_t *)&global_cmd, buffer, sizeof(struct cmd));

	/* From Client ==> To Server */
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
		sleep(10);
		if(ntohl(cf->traffic_type) == UDP)
		{
			printf("UDP CLIENT STARTED\n\t");
			udp_client();
			printf("UDP CLIENT FINISHED\n\t");
		} else if(ntohl(cf->traffic_type) == TCP) {
			printf("TCP CLEINT STARTED\n\t");
			tcp_client();
			printf("TCP CLEINT FINISHED\n\t");
		}
	}

	return 0;
}

int init_tcp() 
{
	int server_fd, valread;
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
	if ((tcp_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		perror("Accept failed");
		exit(EXIT_FAILURE);
	}

	printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

	/* Don´t close this socket, server continuously waiting on this socket for new cmds */
	/* close(tcp_socket); */

	return 0;
}

void init_server(void)
{
	init_tcp();
}

int main() {

    init_server();

    /* Wait for cmds from the client */
    while(1)
    {
	    memset((uint8_t *)&global_cmd, 0, sizeof(struct cmd));
	    wait_for_cmd(tcp_socket);

	    /* send the report at the end of every test */
	    if(ntohl(global_cmd.client_role) == UPLINK)
		    send_report(tcp_socket);
    }

    return 0;
}
