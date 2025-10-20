#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Socket {
private:
    int socket_fd;
    struct sockaddr_in address;
    
public:
    Socket();
    Socket(int fd);  //constructor for client sockets as socket_fd is private and we cannot access it from the main function
    ~Socket();
    
    //server socket methods
    void create();
    void bind(int port);
    void listen(int backlog);
    int accept();
    
    //client socket methods (for accepted connections)
    int receive(char* buffer, int size);
    int send(const char* data, int size);
    void close();
    
    //getters
    int getFd() const { return socket_fd; }
    bool isValid() const { return socket_fd != -1; }
};

#endif