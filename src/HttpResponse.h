#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>

class HttpResponse {
private:
    std::string version;     // HTTP/1.1
    int status_code;         
    std::string status_message;  
    std::map<std::string, std::string> headers;
    std::string body;
    
    std::string getStatusMessage(int code);
    
public:
    HttpResponse();
    
    //setters
    void setStatus(int code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& content);
    
    //build the raw HTTP response
    std::string build() const;
};

#endif