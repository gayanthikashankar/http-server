#include "Socket.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>


//derived class of socket class for server socket - constructor 
Socket::Socket() : socket_fd(-1) {
    memset(&address, 0, sizeof(address));
}

//constructor for client sockets from accept()
Socket::Socket(int fd) : socket_fd(fd) {
    memset(&address, 0, sizeof(address));
}

//destructor
Socket::~Socket() {
    if (isValid()) {
        close();
    }
}

//create the socket
void Socket::create() {
    //ipv4 socket, steam socket, protocol type 0 (default)
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    //socket file descriptor is less than 0, error
    if (socket_fd < 0) {
        std::cerr << "ERROR: Failed to create socket" << std::endl;
        exit(1);
    }
    
    //set socket options to reuse address (important for development)
    int opt = 1; //reuse address
    //sol_socket = socket level, SO_REUSEADDR = reuse address option, opt = value of the option, sizeof(opt) = size of the option
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "ERROR: Failed to set socket options" << std::endl;
        exit(1);
    }
    
    std::cout << "Socket created successfully" << std::endl;
}

//what port am i on?: associate the socket with a port
void Socket::bind(int port) {
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  //listen on all interfaces
    address.sin_port = htons(port);//host to network short, convert port to network byte order
    
    //bind the socket to the port
    if (::bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Failed to bind to port " << port << std::endl;
        exit(1);
    }
    
    std::cout << "Socket bound to port " << port << std::endl;
}

//tell a socket to listen for incoming connections 
void Socket::listen(int backlog) {
    //backlog = #pending connections we can have before the kernel starts rejecting new ones 
    if (::listen(socket_fd, backlog) < 0) {
        std::cerr << "ERROR: Failed to listen on socket" << std::endl;
        exit(1);
    }
    
    std::cout << "Server listening for connections..." << std::endl;
}

//get pending connections 
int Socket::accept() {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    
    int client_fd = ::accept(socket_fd, (struct sockaddr*)&client_address, &client_len);
    
    if (client_fd < 0) {
        std::cerr << "ERROR: Failed to accept connection" << std::endl;
        return -1;
    }
    
    //print client info
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "New connection from " << client_ip << ":" 
              << ntohs(client_address.sin_port) << std::endl;
    
    return client_fd;
}

//recieve: buffer - buffer to read the information into
int Socket::receive(char* buffer, int size) {
    int bytes_received = recv(socket_fd, buffer, size, 0);
    
    if (bytes_received < 0) {
        std::cerr << "ERROR: Failed to receive data" << std::endl;
        return -1;
    }
    
    if (bytes_received == 0) {
        std::cout << "Client disconnected" << std::endl;
    }
    
    return bytes_received;
}

//pointer to the data you want to send + len of data in bytes
int Socket::send(const char* data, int size) {
    int bytes_sent = ::send(socket_fd, data, size, 0);
    
    if (bytes_sent < 0) {
        std::cerr << "ERROR: Failed to send data" << std::endl;
        return -1;
    }
    
    return bytes_sent;
}

//prevent any more reads and writes to the socket
void Socket::close() {
    if (isValid()) {
        ::close(socket_fd);
        socket_fd = -1;
        std::cout << "Socket closed" << std::endl;
    }
}