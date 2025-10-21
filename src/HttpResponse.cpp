#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() 
    : version("HTTP/1.1"), status_code(200), status_message("OK") {
}

std::string HttpResponse::getStatusMessage(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "Unknown";
    }
}

void HttpResponse::setStatus(int code) {
    status_code = code;
    status_message = getStatusMessage(code);
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers[name] = value;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
    
    //automatically set Content-Length
    setHeader("Content-Length", std::to_string(body.length()));
}

//build the raw HTTP response following the HTTP protocol and formatting rules
std::string HttpResponse::build() const {
    std::ostringstream response;
    
    response << version << " " << status_code << " " << status_message << "\r\n";
    
    for (const auto& header : headers) {
        response << header.first << ": " << header.second << "\r\n";
    }
    
    response << "\r\n";
    
    response << body;
    
    return response.str();
}