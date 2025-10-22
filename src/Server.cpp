//implementing the server logic
#include "Server.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>

Server::Server(const std::string& root) : www_root(root) {
    std::cout << "Server root directory: " << www_root << std::endl;
}

//derived method to get the content type of the file from the path
std::string Server::getContentType(const std::string& path) {
    //find file extension
    size_t dot_pos = path.find_last_of('.'); //find the last dot in the path
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";  //return default binary type
    }
    
    std::string extension = path.substr(dot_pos); //get the extension from the path
    
    //map extensions to MIME types
    if (extension == ".html" || extension == ".htm") {
        return "text/html";
    } else if (extension == ".css") {
        return "text/css";
    } else if (extension == ".js") {
        return "application/javascript";
    } else if (extension == ".json") {
        return "application/json";
    } else if (extension == ".txt") {
        return "text/plain";
    } else if (extension == ".png") {
        return "image/png";
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    } else if (extension == ".gif") {
        return "image/gif";
    } else if (extension == ".svg") {
        return "image/svg+xml";
    } else if (extension == ".pdf") {
        return "application/pdf";
    } else if (extension == ".zip") {
        return "application/zip";
    }
    
    return "application/octet-stream";
}

//derived method to check if the file exists
bool Server::fileExists(const std::string& path) {
    struct stat buffer; //struct to store file information in the 'buffer'
    return (stat(path.c_str(), &buffer) == 0); //return true if the file exists, false otherwise
}

bool Server::isPathSafe(const std::string& path) {
    //prevent path traversal attacks
    //block paths containing ".."
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    return true; //if path is safe
}


std::string Server::readFile(const std::string& path, bool& success) {
    std::ifstream file(path, std::ios::binary);
    
    //if the file is not open, return false
    if (!file.is_open()) {
        success = false; 
        return "";
    }
    
    //read entire file into string
    std::ostringstream ss;
    ss << file.rdbuf(); //rdbuf = read buffer, read the file into the stringstream
    
    success = true; //set success to true
    return ss.str();
}

//handle incoming HTTP request
//parse the request, handle the request, and send the response
void Server::handleRequest(const HttpRequest& request, HttpResponse& response) {
    std::string method = request.getMethod();
    std::string path = request.getPath();
    
    std::cout << "Handling: " << method << " " << path << std::endl;
    
    //security check FIRST (before any path manipulation)
    if (!isPathSafe(path)) {
        std::cout << "  Path traversal attempt BLOCKED!: " << path << std::endl;
        response.setStatus(403);  //forbidden error
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>403 Forbidden</h1>"
                        "<p>Path traversal attempt detected.</p></body></html>");
        return;
    }
    
    //only handle GET and HEAD requests for now
    if (method != "GET" && method != "HEAD") {
        response.setStatus(501);  //not implemented error
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>501 Not Implemented</h1>"
                        "<p>Method " + method + " is not supported yet.</p>"
                        "</body></html>");
        return;
    }
    
    //default to index.html if path is /
    if (path == "/") {
        path = "/index.html";
    }
    
    //build full file path
    std::string file_path = www_root + path;
    
    std::cout << "Looking for file: " << file_path << std::endl;
    
    //check if file exists
    if (!fileExists(file_path)) {
        std::cout << "FILE NOT FOUND! " << file_path << std::endl;
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1>"
                        "<p>The requested resource " + path + " was not found.</p>"
                        "</body></html>");
        return;
    }
    
    //read file (skip for HEAD requests)
    std::string content;
    if (method == "GET") {
        bool success;
        content = readFile(file_path, success);
        
        if (!success) {
            std::cout << "FAILED TO READ FILE! " << file_path << std::endl;
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1>"
                            "<p>Failed to read file.</p></body></html>");
            return;
        }
    }
    
    //success - send file
    response.setStatus(200);
    response.setHeader("Content-Type", getContentType(path));
    
    if (method == "GET") {
        response.setBody(content);
        std::cout << "SERVED FILE: " << file_path 
                  << " (" << content.length() << " bytes)" << std::endl;
    } else {
        //head request - just set Content-Length without body
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            response.setHeader("Content-Length", std::to_string(size));
            std::cout << "✓ HEAD response for: " << file_path 
                      << " (" << size << " bytes)" << std::endl;
        }
    }
}