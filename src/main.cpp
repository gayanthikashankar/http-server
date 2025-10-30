#include "Socket.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Server.h"
#include <iostream>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 4096 //buffer size for the HTTP request

void handleClient(int client_fd, Server& server) {
    Socket client_socket(client_fd);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytes_received = client_socket.receive(buffer, BUFFER_SIZE - 1);
    
    if (bytes_received <= 0) {
        std::cerr << "Error receiving request or client disconnected" << std::endl;
        client_socket.close();
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string raw_request(buffer);
    
    size_t first_newline = raw_request.find('\n');
    std::string request_line = raw_request.substr(0, first_newline);
    std::cout << "\n RAW REQUEST LINE: " << request_line << std::endl;
    
    HttpRequest request;
    if (!request.parse(raw_request)) {
        std::cerr << "Failed to parse HTTP request" << std::endl;
        
        HttpResponse response;
        response.setStatus(400); //bad request
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        
        std::string response_str = response.build();
        client_socket.send(response_str.c_str(), response_str.length());
        client_socket.close();
        return;
    }
    
    std::cout << " PARSED PATH: " << request.getPath() << std::endl;
    
    //create http response and let server handle it
    HttpResponse response;
    response.setHeader("Server", "MyHTTPServer/1.0");
    server.handleRequest(request, response);
    
    //build and send response
    std::string response_str = response.build();
    client_socket.send(response_str.c_str(), response_str.length());
    client_socket.close();
}

int main() {
    std::cout << " HTTP SERVER RUNNING ON PORT " << PORT << std::endl;
    
    //create server with www root   
    Server server("./www");
    
    //create server socket
    Socket server_socket;
    server_socket.create();
    server_socket.bind(PORT); //bind the socket to the port - this is where the server listens for incoming connections
    server_socket.listen(5); //listen for incoming connections - backlog is the number of connections that can be queued up (set to 5)
    
    std::cout << "\nHTTP server running on http://localhost:" << PORT << std::endl;
    std::cout << "Press Ctrl+C to stop the server\n" << std::endl;
    
    //main server loop
    while (true) {
        //accept new connection
        int client_fd = server_socket.accept();
        
        if (client_fd < 0) {
            continue; //if the connection is not accepted, continue the loop        
        }
        
        //handle the client
        handleClient(client_fd, server);
    }
    
    return 0;
}