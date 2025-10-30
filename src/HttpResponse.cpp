#include "HttpResponse.h"
#include <sstream>
#include <iostream>

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

void HttpResponse::setCookie(const std::string& name, const std::string& value,
                            int max_age, const std::string& path) {
    std::ostringstream cookie;
    cookie << name << "=" << value;
    
    if (!path.empty()) {
        cookie << "; Path=" << path;
    }
    
    if (max_age >= 0) {
        cookie << "; Max-Age=" << max_age;
    }
    
    cookies.push_back(cookie.str());
    
    std::cout << " Setting cookie: " << cookie.str() << std::endl;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
    setHeader("Content-Length", std::to_string(body.length()));
}

std::string HttpResponse::build() const {
    std::ostringstream response;
    
    response << version << " " << status_code << " " << status_message << "\r\n";
    
    for (const auto& header : headers) {
        response << header.first << ": " << header.second << "\r\n";
    }
    
    //cookies - each set cookie is a sep headre 
    for (const auto& cookie : cookies) {
        response << "Set-Cookie: " << cookie << "\r\n";
    }
    
    response << "\r\n";
    
    response << body;
    
    return response.str();
}