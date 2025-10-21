#include "Socket.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 4096 //buffer size for the request

void handleClient(int client_fd) {
    Socket client_socket(client_fd); //create a socket object for the client
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE); //initialize the buffer to 0
    
    //receive the HTTP request
    int bytes_received = client_socket.receive(buffer, BUFFER_SIZE - 1);
    
    if (bytes_received <= 0) {
        std::cerr << "Error receiving request or client disconnected" << std::endl;
        client_socket.close();
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string raw_request(buffer); //convert the buffer to a string
    
    std::cout << "\nRAW REQUEST: " << std::endl;
    std::cout << raw_request << std::endl;
    
    //parse HTTP request
    HttpRequest request;
    if (!request.parse(raw_request)) {
        std::cerr << "Failed to parse HTTP request" << std::endl;
        
        //send 400 Bad Request
        HttpResponse response;
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        
        std::string response_str = response.build(); //build the response
        client_socket.send(response_str.c_str(), response_str.length()); //send the response to the client
        client_socket.close();
        return; 
    }
    
    //print parsed request
    request.print();
    
    //create HTTP response
    HttpResponse response;
    response.setStatus(200);
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Server", "MyHTTPServer/1.0");
    
    // Create a simple HTML response
    std::string html = "<html>\n"
                      "<head><title>Hello from HTTP Server</title></head>\n"
                      "<body>\n"
                      "<h1>Hello, World!</h1>\n"
                      "<p>You requested: " + request.getPath() + "</p>\n" //path of the request
                      "<p>Method: " + request.getMethod() + "</p>\n" //method of the request
                      "</body>\n"
                      "</html>";
    
    response.setBody(html); //set the body of the response
    
    std::string response_str = response.build(); //build the response
    
    std::cout << "\nSENDING RESPONSE... " << std::endl;
    std::cout << response_str << std::endl;
    
    client_socket.send(response_str.c_str(), response_str.length()); //send the response to the client
    client_socket.close(); //close the connection
}

int main() {
    std::cout << "HTTP SERVER: " << std::endl;
    
    //create server socket
    Socket server_socket;
    server_socket.create();
    server_socket.bind(PORT); //bind the socket to the port
    server_socket.listen(5); //listen for connections, 5 = backlog (max number of pending connections)
    
    std::cout << "\nHTTP server running on http://localhost:" << PORT << std::endl;
    std::cout << "Waiting for connections...\n" << std::endl;
    
    while (true) {
        //accept new connection
        int client_fd = server_socket.accept();
        
        if (client_fd < 0) {
            continue;
        }
        
        //handle the client
        handleClient(client_fd);
    }
    
    return 0;
}