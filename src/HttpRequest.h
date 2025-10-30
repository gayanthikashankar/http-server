#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>

class HttpRequest {
private:
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    
    static std::string trim(const std::string& str);
    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    
public:
    HttpRequest();
    
    //parsing method main - parses the raw request into a HttpRequest object
    bool parse(const std::string& raw_request);
    
    //form data parsing - parses the form data into a map of key-value pairs
    std::map<std::string, std::string> parseFormData();
    static std::string urlDecode(const std::string& str);
    
    //cookie parsing - parses the cookies into a map of key-value pairs
    std::map<std::string, std::string> parseCookies() const;
    std::string getCookie(const std::string& name) const;
    
    std::string getMethod() const { return method; }
    std::string getPath() const { return path; }
    std::string getVersion() const { return version; }
    std::string getHeader(const std::string& name) const;
    std::string getBody() const { return body; }
    
    bool isValid() const { return !method.empty() && !path.empty(); }
    void print() const;
};

#endif