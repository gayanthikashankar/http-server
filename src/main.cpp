#include "Socket.h"
#include <iostream>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 1024

void handleClient(int client_fd) {
    char buffer[BUFFER_SIZE];

    //creating a socket wrapper for the client using the new constructor
    Socket client_socket(client_fd);
    
    while (true) {
        //clear buffer
        memset(buffer, 0, BUFFER_SIZE);
        
        //recv data from client
        int bytes_received = client_socket.receive(buffer, BUFFER_SIZE - 1);
        
        if (bytes_received < 0) {
            std::cerr << "Error receiving data" << std::endl;
            break;
        }
        
        if (bytes_received == 0) {
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        
        //null terminate the received data
        buffer[bytes_received] = '\0';
        
        std::cout << "Received " << bytes_received << " bytes: " << buffer;
        
        //echo back to client
        int bytes_sent = client_socket.send(buffer, bytes_received);
        
        if (bytes_sent < 0) {
            std::cerr << "Error sending data" << std::endl;
            break;
        }
        
        std::cout << "Echoed " << bytes_sent << " bytes back to client" << std::endl;
    }
    
    client_socket.close();
}

int main() {
    std::cout << "ECHO SERVER" << std::endl;
    
    //server sokcet
    Socket server_socket;
    server_socket.create();
    server_socket.bind(PORT);
    server_socket.listen(5);
    
    std::cout << "\nEcho server running on port " << PORT << std::endl;
    std::cout << "Waiting for connections...\n" << std::endl;
    
    //main server loop
    while (true) {
        //accept new connection
        int client_fd = server_socket.accept();
        
        if (client_fd < 0) {
            continue;  //skip this iteration if accept failed
        }
        
        //handle the client (blocking - only one client at a time for now)
        handleClient(client_fd);
    }
    
    return 0;
}