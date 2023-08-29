import socket
import struct
import time

# Define constants for client roles
UPLINK = 1
DOWNLINK = 2

# Define constants for traffic types
UDP = 1
TCP = 2

# Define constants for traffic modes
CONTINUOUS = 1
DELAYED = 2

# Define the message structure using a Python dictionary
cmd_structure = {
    'client_role': 0,
    'traffic_type': 0,
    'traffic_mode': 0,
    'duration': 0,
    'frame_len': 0,
    'reserved': 0
}

def tcp_server(ctrl_socket):
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

    try:
        while True:
            data = client_socket.recv(1024)
            if not data:
                print("Received empty message. Closing the connection.")
                break
            packet_count += 1
            total_bytes_received += len(data)
            print(f"Received message: {data.decode('utf-8')}")

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
#    report_format = '!iiQfQ'
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

def tcp_client(cmd_structure):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('192.168.1.211', 7777)  # Change this to the appropriate server address
    client_socket.connect(server_address)
    
    try:
        start_time = time.time()
        while time.time() - start_time < cmd_structure['duration']:
            message = b'x' * cmd_structure['frame_len']  # Creating a message with specified frame length
            client_socket.sendall(message)
            time.sleep(10/1000)  # Sending messages every 1 second

        print("Data transmission finished.")
        
        # Send an empty message to signal termination
        client_socket.sendall(b'')

    finally:
        client_socket.close()

def process_client_cmd(ctrl_socket):
    global cmd_structure
    
    print("Waiting for client config info\n");
    buffer = ctrl_socket.recv(1024)
    cf = buffer[:24]  # Assuming sizeof(struct cmd) is 24 bytes
    cmd_structure = {
        'client_role': int.from_bytes(cf[0:4], byteorder='big'),
        'traffic_type': int.from_bytes(cf[4:8], byteorder='big'),
        'traffic_mode': int.from_bytes(cf[8:12], byteorder='big'),
        'duration': int.from_bytes(cf[12:16], byteorder='big'),
        'frame_len': int.from_bytes(cf[16:20], byteorder='big'),
        'reserved': int.from_bytes(cf[20:24], byteorder='big')
    }

    print("Client CMD received")
    print("\tClient Role:", cmd_structure['client_role'])
    print("\tTraffic type:", cmd_structure['traffic_type'])
    print("\tTraffic Mode:", cmd_structure['traffic_mode'])
    print("\tDuration:", cmd_structure['duration'])
    print("\tFrame len:", cmd_structure['frame_len'])
    print("\tReserved:\n", cmd_structure['reserved'])

    # Implement logic for starting servers/clients and processing traffic
    if cmd_structure['client_role'] == UPLINK:
        if cmd_structure['traffic_type'] == UDP:
            print("UDP SERVER STARTED")
            # Implement udp_server()
        elif cmd_structure['traffic_type'] == TCP:
            print("TCP SERVER STARTED")
            tcp_server(ctrl_socket)  # Call the TCP server function
            # Implement tcp_server()
    elif cmd_structure['client_role'] == DOWNLINK:
        print("DOWNLINK:")
        time.sleep(10)
        if cmd_structure['traffic_type'] == UDP:
            print("UDP CLIENT STARTED")
            # Implement udp_client()
            print("UDP CLIENT FINISHED")
        elif cmd_structure['traffic_type'] == TCP:
            print("TCP CLIENT STARTED")
            tcp_client(cmd_structure)
            print("TCP CLIENT FINISHED")

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('0.0.0.0', 6666)
    server_socket.bind(server_address)
    server_socket.listen(1)
    print(f"Server is listening on {server_address[0]}:{server_address[1]}...")

    client_socket, client_address = server_socket.accept()
    print(f"Connected to client: {client_address}")

    try:
        #while True:
        process_client_cmd(client_socket)
            # Add logic to break the loop when needed
    finally:
        client_socket.close()
        server_socket.close()
        print("Server is closed.")

if __name__ == "__main__":
    main()

