#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
private:
    std::string version;
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::vector<std::string> cookies;  //multiple set cookie headers
    std::string body;
    
    std::string getStatusMessage(int code);
    
public:
    HttpResponse();

    void setStatus(int code);
    void setHeader(const std::string& name, const std::string& value);
    void setCookie(const std::string& name, const std::string& value,
                   int max_age = -1, const std::string& path = "/");  // NEW
    void setBody(const std::string& content);
    
    //build the raw HTTP response
    std::string build() const;
};

#endif