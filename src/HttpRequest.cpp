#include "HttpRequest.h"
#include <iostream>
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() {
}

//helper method to trim the string
//remove leading and trailing whitespace
//returns the trimmed string
std::string HttpRequest::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void HttpRequest::parseRequestLine(const std::string& line) {
    //parse: GET /index.html HTTP/1.1
    std::istringstream iss(line);
    iss >> method >> path >> version;
    
    //validate the request line
    if (method.empty() || path.empty() || version.empty()) {
        std::cerr << "Error: Invalid request line" << std::endl;
    }
}

void HttpRequest::parseHeader(const std::string& line) {
    //parse: Host: localhost:8080
    size_t colon_pos = line.find(':');
    
    if (colon_pos == std::string::npos) {
        return;  //invalid header? skip it
    }
    
    std::string name = trim(line.substr(0, colon_pos));
    std::string value = trim(line.substr(colon_pos + 1));
    
    //convert header name to lowercase for case-insensitive comparison
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    headers[name] = value;
}

//main parsing method
//parse the raw request into a HttpRequest object
//returns true if successful, false otherwise
bool HttpRequest::parse(const std::string& raw_request) {
    if (raw_request.empty()) {
        return false;
    }
    
    //find the end of headers (blank line)
    size_t header_end = raw_request.find("\r\n\r\n");
    bool has_crlf = true;
    
    if (header_end == std::string::npos) {
        //try with just \n\n (for testing with nc) -----fallback for testing with nc
        header_end = raw_request.find("\n\n");
        has_crlf = false;
        
        if (header_end == std::string::npos) {
            std::cerr << "Error: Could not find end of headers" << std::endl;
            return false;
        }
    }
    
    //extract headers section
    std::string headers_section = raw_request.substr(0, header_end);
    
    //extract body (if any)
    if (has_crlf) {
        body = raw_request.substr(header_end + 4);  //skip \r\n\r\n
    } else {
        body = raw_request.substr(header_end + 2);  //skip \n\n
    }
    
    //split headers into lines
    std::istringstream stream(headers_section);
    std::string line;
    bool first_line = true;
    
    while (std::getline(stream, line)) {
        //remove \r if present (handles both \r\n and \n line endings)
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        if (line.empty()) {
            continue;  
        }
        
        if (first_line) {
            parseRequestLine(line);
            first_line = false;
        } else {
            parseHeader(line);
        }
    }
    
    return isValid();
}

//get header value by name
std::string HttpRequest::getHeader(const std::string& name) const {
    //convert to lowercase for case-insensitive lookup
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    auto it = headers.find(lower_name);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}

void HttpRequest::print() const {
    std::cout << "HTTP REQUEST: " << std::endl;
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << path << std::endl;
    std::cout << "Version: " << version << std::endl;
    std::cout << "Headers:" << std::endl;
    
    for (const auto& header : headers) {
        std::cout << "  " << header.first << ": " << header.second << std::endl;
    }
    
    if (!body.empty()) {
        std::cout << "Body: " << body << std::endl;
    }
}

std::string HttpRequest::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            //hex to char
            std::string hex = str.substr(i + 1, 2);
            try {
                char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
                result += ch;
                i += 2;
            } catch (...) {
                //invalid hex, keep the %
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::map<std::string, std::string> HttpRequest::parseFormData() {
    std::map<std::string, std::string> form_data;
    
    //if body is empty, return the form data
    if (body.empty()) {
        return form_data;
    }
    
    //parse application/x-www-form-urlencoded format
    // Format: key1=value1&key2=value2&key3=value3
    
    std::istringstream stream(body);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t equals_pos = pair.find('=');
        
        if (equals_pos != std::string::npos) {
            std::string key = pair.substr(0, equals_pos);
            std::string value = pair.substr(equals_pos + 1);
            
            key = urlDecode(key);
            value = urlDecode(value);
            
            form_data[key] = value;
        }
    }
    
    return form_data;
}

std::map<std::string, std::string> HttpRequest::parseCookies() const {
    std::map<std::string, std::string> cookies;
    
    std::string cookie_header = getHeader("cookie");
    
    if (cookie_header.empty()) {
        return cookies;
    }
    
    //cookie1=value1; cookie2=value2; cookie3=value3
    std::istringstream stream(cookie_header);
    std::string pair;
    
    while (std::getline(stream, pair, ';')) {
        // Trim whitespace
        pair = trim(pair);
        
        size_t equals_pos = pair.find('=');
        if (equals_pos != std::string::npos) {
            std::string name = pair.substr(0, equals_pos);
            std::string value = pair.substr(equals_pos + 1);
            
            cookies[trim(name)] = trim(value);
        }
    }
    
    return cookies;
}

std::string HttpRequest::getCookie(const std::string& name) const {
    std::map<std::string, std::string> cookies = parseCookies();
    
    auto it = cookies.find(name);
    if (it != cookies.end()) {
        return it->second;
    }
    
    return "";
}