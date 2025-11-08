#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

struct UploadedFile {
    std::string field_name;      
    std::string filename;        
    std::string content_type;    //MIME type
    std::string content;         //binary
};

class HttpRequest {
    private:
        std::string method;
        std::string path;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
        
        // Helper methods
        std::string trim(const std::string& str) const;
        void parseRequestLine(const std::string& line);
        void parseHeader(const std::string& line);
        
    public:
        HttpRequest();
        
        //main parsing method
        bool parse(const std::string& raw_request);
        
        std::map<std::string, std::string> parseFormData();
        static std::string urlDecode(const std::string& str);
        static std::string urlEncode(const std::string& str);
        
        //multipart form data parsing 
        std::map<std::string, std::string> parseMultipartFormData(std::vector<UploadedFile>& files);
        
        std::map<std::string, std::string> parseCookies() const;
        std::string getCookie(const std::string& name) const;
        
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