import socket
import struct
import time
import argparse

# Define constants for client roles
UPLINK = 1
DOWNLINK = 2

# Define constants for traffic types
UDP = 1
TCP = 2

# Define constants for traffic modes
CONTINUOUS = 1
DELAYED = 2

#in seconds
EXTRA_TIMEOUT = 5

# Define the message structure using a Python dictionary
config_data = {
    'role': 0,
    'type': 0,
    'mode': 0,
    'duration': 0,
    'payload_len': 0,
}

def tcp_server(ctrl_socket, config_data):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('0.0.0.0', 7777)  # Listen on all available network interfaces
    server_socket.bind(server_address)
    server_socket.listen(1)
    print(f"TCP server is listening on {server_address[0]}:{server_address[1]}...")

    client_socket, client_address = server_socket.accept()
    print(f"Connected to client: {client_address}")

    packet_count = 0
    total_bytes_received = 0
    start_time = time.time()
    remaining_timeout = config_data['duration'] + EXTRA_TIMEOUT

    try:
        while True:
            client_socket.settimeout(remaining_timeout)  # Set the current remaining timeout
            try:
                data = client_socket.recv(1024)
                if not data:
                    print("Received empty message. Closing the connection.")
                    break
                packet_count += 1
                total_bytes_received += len(data)
#                print(f"Received message: {data.decode('utf-8')}")

            except socket.timeout:
                print(f"No data received for {remaining_timeout:.2f} seconds. Closing connection.")
                break
            except BlockingIOError:
                print("BlockingIOError caught. Setting socket to non-blocking.")
                client_socket.setblocking(False)  # Set the socket to non-blocking
                break
            finally:
                elapsed_time = time.time() - start_time
                remaining_timeout = max(0, (config_data['duration'] + EXTRA_TIMEOUT) - elapsed_time)
                #print(f"Remaining timeout for data: {remaining_timeout:.2f} seconds.")

    finally:
        # Calculate statistics
        end_time = time.time()
        total_seconds = int(end_time - start_time)
        throughput_kbps = int((total_bytes_received * 8) / (total_seconds * 1024))  # Kbps calculation
        average_jitter = 0  # Calculate jitter as needed

        # Prepare the report data
        report_data = {
                'bytes_received': total_bytes_received,
                'packets_received': packet_count,
                'elapsed_time': total_seconds,
                'throughput': throughput_kbps,
                'average_jitter': average_jitter
                }

        # Print the report
        print_report(report_data)

        time.sleep(5)

        # Send the report back to the client using the 6666 server socket
        send_server_report(ctrl_socket, report_data)

        client_socket.close()
        server_socket.close()
        print("TCP server is closed.")

def send_server_report(client_socket, report_data):
    report_format = '!iiiii'
    report_packed = struct.pack(report_format, 
                                report_data['bytes_received'],
                                report_data['packets_received'],
                                report_data['elapsed_time'],
                                report_data['throughput'],
                                report_data['average_jitter'])
    client_socket.sendall(report_packed)
    print("Report sent to client.")

def print_report(report_data):
    print("TCP Server Report:")
    print(f"   Bytes Received: {report_data['bytes_received']} bytes")
    print(f"   Packets Received: {report_data['packets_received']}")
    print(f"   Elapsed Time: {report_data['elapsed_time']} Seconds")
    print(f"   Throughput: {report_data['throughput']} Kbps")
    print(f"   Average Jitter: {report_data['average_jitter']}")  # Calculate and provide actual value

def tcp_client(config_data):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('192.168.1.211', 7777)  # Change this to the appropriate server address
    client_socket.connect(server_address)
    
    try:
        start_time = time.time()
        while time.time() - start_time < config_data['duration']:
            message = b'x' * config_data['payload_len']  # Creating a message with specified frame length
            client_socket.sendall(message)
            time.sleep(1/1000)  # Sending messages every 1 second

        print("Data transmission finished.")
        
        # Send an empty message to signal termination
        client_socket.sendall(b'')

    finally:
        client_socket.close()

def process_client_config_data(ctrl_socket):
    global config_data
    
    print("Waiting for client config info\n");
    buffer = ctrl_socket.recv(1024)
    cf = buffer[:24]  # Assuming sizeof(struct config_data) is 24 bytes
    config_data = {
        'role': int.from_bytes(cf[0:4], byteorder='big'),
        'type': int.from_bytes(cf[4:8], byteorder='big'),
        'mode': int.from_bytes(cf[8:12], byteorder='big'),
        'duration': int.from_bytes(cf[12:16], byteorder='big'),
        'payload_len': int.from_bytes(cf[16:20], byteorder='big'),
        'reserved': int.from_bytes(cf[20:24], byteorder='big')
    }

    print("Client config data received")
    print("\tClient Role:", config_data['role'])
    print("\tTraffic type:", config_data['type'])
    print("\tTraffic Mode:", config_data['mode'])
    print("\tDuration:", config_data['duration'])
    print("\tFrame len:", config_data['payload_len'])

    # Implement logic for starting servers/clients and processing traffic
    if config_data['role'] == UPLINK:
        print("UPLINK:")
        if config_data['type'] == TCP:
            print("TCP SERVER STARTED")
            tcp_server(ctrl_socket, config_data)
        else:
            print("Invalid Traffic type")
    elif config_data['role'] == DOWNLINK:
        print("DOWNLINK:")
        time.sleep(10)
        if config_data['type'] == TCP:
            print("TCP CLIENT STARTED")
            tcp_client(config_data)
            print("TCP CLIENT FINISHED")
        else:
            print("Invalid Traffic Type")

def parse_args():
    parser = argparse.ArgumentParser(description="Server for handling UDP and TCP traffic")
    parser.add_argument("--tcp-port", type=int, default=7777, help="TCP server port")
    parser.add_argument("--client-port", type=int, default=6666, help="Client command port")
    parser.add_argument("--client-ip", default="0.0.0.0", help="Client command IP address")

    # Customize the help message
    parser.format_help = lambda: f"{parser.description}\n\n" \
                                    f"Default TCP Port: 7777\n" \
                                    f"Default Client Command Port: 6666\n" \
                                    f"Default Client Command IP: 0.0.0.0\n"

    return parser.parse_args()

def main():
    args = parse_args()  # Parse command-line arguments
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('0.0.0.0', 6666)
    server_socket.bind(server_address)
    server_socket.listen(1)
    print(f"Server is listening on {server_address[0]}:{server_address[1]}...")

    client_socket, client_address = server_socket.accept()
    print(f"Connected to client: {client_address}")

    try:
        process_client_config_data(client_socket)
    finally:
        time.sleep(2);
        client_socket.close()
        server_socket.close()
        print("Server is closed.")

if __name__ == "__main__":
    main()

