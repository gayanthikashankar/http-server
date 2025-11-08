// src/main.cpp
#include "Socket.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Server.h"
#include <iostream>
#include <cstring>

#define PORT 8080
#define BUFFER_SIZE 8192  
std::string readFullRequest(Socket& client_socket) {
    std::string full_request;
    char buffer[BUFFER_SIZE];
    int content_length = -1;
    size_t headers_end_pos = std::string::npos;
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = client_socket.receive(buffer, BUFFER_SIZE - 1);
        
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        full_request.append(buffer, bytes_received);
        
        //find end of headers if we haven't yet
        if (headers_end_pos == std::string::npos) {
            headers_end_pos = full_request.find("\r\n\r\n");
            if (headers_end_pos == std::string::npos) {
                headers_end_pos = full_request.find("\n\n");
            }
            
            //parse Content-Length if we found headers
            if (headers_end_pos != std::string::npos && content_length == -1) {
                std::string headers = full_request.substr(0, headers_end_pos); //extract headers section
                
                //find Content-Length header 
                size_t cl_pos = headers.find("Content-Length:");
                if (cl_pos == std::string::npos) {
                    cl_pos = headers.find("content-length:");
                }
                
                if (cl_pos != std::string::npos) {
                    size_t line_end = headers.find("\n", cl_pos);
                    std::string cl_line = headers.substr(cl_pos, line_end - cl_pos);
                    size_t colon_pos = cl_line.find(":");
                    if (colon_pos != std::string::npos) {
                        std::string cl_value = cl_line.substr(colon_pos + 1);
                        cl_value.erase(0, cl_value.find_first_not_of(" \t\r\n"));
                        cl_value.erase(cl_value.find_last_not_of(" \t\r\n") + 1);
                        content_length = std::stoi(cl_value);
                        
                        std::cout << "📏 Content-Length: " << content_length << " bytes" << std::endl;
                    }
                }
            }
        }
        
        //received the complete request?
        if (headers_end_pos != std::string::npos && content_length >= 0) {
            size_t header_size = (full_request.find("\r\n\r\n") != std::string::npos) ? 
                                headers_end_pos + 4 : headers_end_pos + 2;
            size_t body_size = full_request.length() - header_size;
            
            std::cout << "RECIEVED: " << body_size << " / " << content_length << " bytes" << std::endl;
            
            if (body_size >= (size_t)content_length) {
                std::cout << "Complete request received" << std::endl;
                break;
            }
        }
        
        //no Content-Length -- check for common patterns that indicate end
        if (content_length == -1 && headers_end_pos != std::string::npos) {
            //for GET requests with no body
            if (full_request.find("GET ") == 0 || full_request.find("DELETE ") == 0) {
                break;
            }
            
            //wait a bit more for POST/PUT
            if (bytes_received < BUFFER_SIZE - 1) {
                break;
            }
        }
    }
    
    return full_request;
}

void handleClient(int client_fd, Server& server) {
    Socket client_socket(client_fd);
    
    //read full request
    std::string raw_request = readFullRequest(client_socket);
    
    if (raw_request.empty()) {
        std::cerr << "Error: Empty request" << std::endl;
        client_socket.close();
        return;
    }
    
    //show request info
    size_t first_line_end = raw_request.find('\n');
    std::string request_line = raw_request.substr(0, first_line_end);
    std::cout << "\n " << request_line;
    std::cout << "   Total size: " << raw_request.length() << " bytes" << std::endl;
    
    HttpRequest request;
    if (!request.parse(raw_request)) {
        std::cerr << "Failed to parse HTTP request" << std::endl;
        
        HttpResponse response;
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        
        std::string response_str = response.build();
        client_socket.send(response_str.c_str(), response_str.length());
        client_socket.close();
        return;
    }
    
    std::cout << "[" << request.getMethod() << " " << request.getPath() << "]" << std::endl;
    
    //create HTTP response and let server handle it
    HttpResponse response;
    response.setHeader("Server", "MyHTTPServer/1.0");
    response.setHeader("Connection", "close");
    server.handleRequest(request, response);
    
    std::string response_str = response.build();
    
    std::cout << "📤 Sending: " << response_str.length() << " bytes" << std::endl;
    
    client_socket.send(response_str.c_str(), response_str.length());
    client_socket.close();
}

int main() {
    std::cout << "=== HTTP SERVER ===" << std::endl;
    
    //create server with www root
    Server server("./www", "./uploads");
    
    Socket server_socket;
    server_socket.create();
    server_socket.bind(PORT);
    server_socket.listen(5);
    
    std::cout << "\nHTTP server running on http://localhost:" << PORT << std::endl;
    std::cout << "Press Ctrl+C to stop the server\n" << std::endl;
    
    //main server loop
    while (true) {
        int client_fd = server_socket.accept();
        
        if (client_fd < 0) {
            continue;
        }
        
        handleClient(client_fd, server);
    }
    
    return 0;
}