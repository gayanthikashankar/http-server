#ifndef SERVER_H
#define SERVER_H

#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>
#include <map>

class Server {
private:
    std::string www_root;
    std::string uploads_root;
    std::map<std::string, std::string> sessions;  //session_id -> username
    
    std::string getContentType(const std::string& path);
    std::string readFile(const std::string& path, bool& success);
    bool fileExists(const std::string& path);
    bool isPathSafe(const std::string& path);
    std::string generateSessionId();  
    
    void handleGET(const HttpRequest& request, HttpResponse& response);
    void handlePOST(const HttpRequest& request, HttpResponse& response);
    void handleDELETE(const HttpRequest& request, HttpResponse& response);
    
    //for cookies 
    void handleLogin(const HttpRequest& request, HttpResponse& response);
    void handleDashboard(const HttpRequest& request, HttpResponse& response);
    void handleLogout(const HttpRequest& request, HttpResponse& response);
    
public:
    Server(const std::string& root = "./www", const std::string& uploads = "./uploads");
    
    void handleRequest(const HttpRequest& request, HttpResponse& response);
};

#endif