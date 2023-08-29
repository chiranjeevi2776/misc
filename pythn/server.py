import socket

def main():
    # Create a socket object
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # Bind the socket to a specific address and port
    server_address = ('0.0.0.0', 12345)
    server_socket.bind(server_address)
    
    # Listen for incoming connections
    server_socket.listen(1)
    print(f"Server is listening on {server_address[0]}:{server_address[1]}...")
    
    # Accept a connection
    client_socket, client_address = server_socket.accept()
    print(f"Connected to client: {client_address}")
    
    try:
        while True:
            data = client_socket.recv(1024).decode('utf-8')
            if not data:
                print("Received empty message. Closing the connection.")
                break
            print(f"Received message: {data}")
            
    finally:
        # Clean up the connection
        client_socket.close()
        server_socket.close()
        print("Server is closed.")

if __name__ == "__main__":
    main()

