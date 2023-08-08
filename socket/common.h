#ifndef COMMON_H
#define COMMON_H

#define MAX_TEST_CASES  20

/* client role */
#define UPLINK 1
#define DOWNLINK 2

/* Traffic Type */
#define UDP 1
#define TCP 2

/* Traffic Mode */
#define CONTINUOUS 1
#define DELAYED 2

struct cmd {
    int client_role;   /* uplink or downlink */
    int traffic_type;  /* udp or tcp */
    int traffic_mode;  /* continuous or delayed */
    int duration; 
    int frame_len;
    int reserved;
};

struct server_report {
    int bytes_received;
    int packets_received;
    double elapsed_time;
    double throughput;
    double average_jitter;
    char server_ip[INET_ADDRSTRLEN]; // IP address of the server
    int server_port;                 // Port number of the server
};

// Array of predefined test cases
struct cmd test_case[MAX_TEST_CASES] = {
	// 5 TX cases with different packet lengths
	{UPLINK, UDP, CONTINUOUS,  10, 64, 0},
	{UPLINK, UDP, CONTINUOUS,  20, 128, 0},
	{UPLINK, UDP, CONTINUOUS,  10, 256, 0},
	{UPLINK, UDP, CONTINUOUS,  60, 512, 0},
	// Add more TX cases here

	// 5 RX cases with different packet lengths
	{DOWNLINK, UDP, CONTINUOUS, 10, 64, 0},
	{DOWNLINK, UDP, CONTINUOUS, 20, 128, 0},
	{DOWNLINK, UDP, CONTINUOUS, 10, 256, 0},
	{DOWNLINK, UDP, CONTINUOUS, 60, 512, 0},
	// Add more RX cases here
};

#endif
