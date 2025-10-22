//server logic for the HTTP server
#ifndef SERVER_H
#define SERVER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>

class Server {
private:
    std::string www_root;  //root directory for static files
    
    //helper methods
    std::string getContentType(const std::string& path);
    std::string readFile(const std::string& path, bool& success);
    bool fileExists(const std::string& path); 
    bool isPathSafe(const std::string& path); //check if the path is safe to serve
    
public:
    Server(const std::string& root = "./www");
    
    //handle incoming HTTP request
    //parse the request, handle the request, and send the response
    void handleRequest(const HttpRequest& request, HttpResponse& response);
};

#endif