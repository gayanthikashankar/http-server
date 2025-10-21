#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>

class HttpRequest {
private:
    std::string method;      //GET, POST, DELETE etc
    std::string path;        // /index.html
    std::string version;     // HTTP/1.1
    std::map<std::string, std::string> headers;  // Header name -> value
    std::string body;        //request body (for POST)
    
    //helper methods
    std::string trim(const std::string& str);
    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    
public:
    HttpRequest();
    
    //main parsing method
    bool parse(const std::string& raw_request);
    
    //getters
    std::string getMethod() const { return method; }
    std::string getPath() const { return path; }
    std::string getVersion() const { return version; }
    std::string getHeader(const std::string& name) const;
    std::string getBody() const { return body; }
    
    //utility
    bool isValid() const { return !method.empty() && !path.empty(); }
    void print() const;  
};

#endif