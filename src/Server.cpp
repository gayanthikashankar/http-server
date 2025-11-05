#include "Server.h"
#include "HttpRequest.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <cstdio>
#include <algorithm>
#include <ctime>
#include <cstdlib>


Server::Server(const std::string& root, const std::string& uploads) 
    : www_root(root), uploads_root(uploads) {
    std::cout << "Server root directory: " << www_root << std::endl;
    std::cout << "Uploads directory: " << uploads_root << std::endl;
    
    //create uploads directory if it doesn't exist
    mkdir(uploads_root.c_str(), 0755);
}

std::string Server::getContentType(const std::string& path) {
    //find file extension
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";  // Default binary type
    }
    
    std::string extension = path.substr(dot_pos);
    
    if (extension == ".html" || extension == ".htm") {
        return "text/html";
    } else if (extension == ".css") {
        return "text/css";
    } else if (extension == ".js") {
        return "application/javascript";
    } else if (extension == ".json") {
        return "application/json";
    } else if (extension == ".txt") {
        return "text/plain";
    } else if (extension == ".png") {
        return "image/png";
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    } else if (extension == ".gif") {
        return "image/gif";
    } else if (extension == ".svg") {
        return "image/svg+xml";
    } else if (extension == ".pdf") {
        return "application/pdf";
    } else if (extension == ".zip") {
        return "application/zip";
    }
    
    return "application/octet-stream";
}

bool Server::isPathSafe(const std::string& path) {
    
    //block ".."
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    //block encoded dots (%2e%2e or %2E%2E)
    std::string lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    if (lower_path.find("%2e%2e") != std::string::npos) {
        return false;
    }
    
    //path should start with /
    if (path.empty() || path[0] != '/') {
        return false;
    }
    
    return true;
}

bool Server::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

std::string Server::readFile(const std::string& path, bool& success) {
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        success = false;
        return "";
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    
    success = true;
    return ss.str();
}

void Server::handleGET(const HttpRequest& request, HttpResponse& response) {
    std::string path = HttpRequest::urlDecode(request.getPath());
    
    if (path == "/dashboard") {
        handleDashboard(request, response);
        return;
    } else if (path == "/logout") {
        handleLogout(request, response);
        return;
    }
    
    //default to index.html if path is /
    if (path == "/") {
        path = "/index.html";
    }
    
    std::string file_path = www_root + path;
    
    std::cout << "Looking for file: " << file_path << std::endl;
    
    if (!fileExists(file_path)) {
        std::cout << "File not found: " << file_path << std::endl;
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1>"
                        "<p>The requested resource " + path + " was not found.</p>"
                        "</body></html>");
        return;
    }
    
    bool success;
    std::string content = readFile(file_path, success);
    
    if (!success) {
        std::cout << "Failed to read file: " << file_path << std::endl;
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1>"
                        "<p>Failed to read file.</p></body></html>");
        return;
    }
    
    response.setStatus(200);
    response.setHeader("Content-Type", getContentType(path));
    response.setBody(content);
    
    std::cout << "Served file: " << file_path 
              << " (" << content.length() << " bytes)" << std::endl;
}

void Server::handlePOST(const HttpRequest& request, HttpResponse& response) {
    std::string path = HttpRequest::urlDecode(request.getPath());
    
    std::cout << "POST request to: " << path << std::endl;
    
    //route to appropriate handler
    if (path == "/login") {
        handleLogin(request, response);
        return;
    } else if (path == "/submit") {
        //parse form data
        std::map<std::string, std::string> form_data = 
            const_cast<HttpRequest&>(request).parseFormData();
        
        std::cout << "Form data received:" << std::endl;
        for (const auto& pair : form_data) {
            std::cout << "  " << pair.first << " = " << pair.second << std::endl;
        }
        
        if (form_data.empty()) {
            std::cout << " Warning: No form data received!" << std::endl;
            std::cout << "Body content: [" << request.getBody() << "]" << std::endl;
            
            response.setStatus(400);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<!DOCTYPE html><html><head><title>Error</title></head><body>"
                            "<h1>400 Bad Request</h1>"
                            "<p>No form data received.</p>"
                            "<p><a href='/form.html'>Try Again</a></p>"
                            "</body></html>");
            return;
        }
        
        std::string log_path = uploads_root + "/submissions.txt";
        std::ofstream log_file(log_path, std::ios::app);
        
        if (log_file.is_open()) {
            log_file << "NEW SUBMISSION: " << std::endl;
            for (const auto& pair : form_data) {
                log_file << pair.first << ": " << pair.second << std::endl;
            }
            log_file << std::endl;
            log_file.close();
            
            response.setStatus(200); //success response
            response.setHeader("Content-Type", "text/html");
            
            std::string html = "<!DOCTYPE html><html><head>"
                              "<title>Success</title>"
                              "<style>"
                              "body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }"
                              "h1 { color: #28a745; }"
                              "ul { background: #f8f9fa; padding: 20px; border-radius: 5px; list-style: none; }"
                              "li { margin: 10px 0; padding: 10px; background: white; border-radius: 3px; }"
                              "strong { color: #007bff; }"
                              "a { display: inline-block; margin-top: 20px; color: #007bff; text-decoration: none; }"
                              "</style>"
                              "</head><body>"
                              "<h1>✓ Form Submitted Successfully!</h1>"
                              "<h2>Received Data:</h2>"
                              "<ul>";
            
            for (const auto& pair : form_data) {
                html += "<li><strong>" + pair.first + ":</strong> " + pair.second + "</li>";
            }
            
            html += "</ul>"
                   "<p><a href='/form.html'>Submit Another</a> | "
                   "<a href='/'>Back to Home</a></p>"
                   "</body></html>";
            
            response.setBody(html);
            
            std::cout << "Form data saved to " << log_path << std::endl;
        } else {
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<!DOCTYPE html><html><head><title>Error</title></head><body>"
                            "<h1>500 Internal Server Error</h1>"
                            "<p>Could not save submission.</p>"
                            "</body></html>");
        }
    } else {
        //unknown POST endpoint
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><head><title>Not Found</title></head><body>"
                        "<h1>404 Not Found</h1>"
                        "<p>POST endpoint " + path + " not found.</p>"
                        "<p><a href='/'>Back to Home</a></p>"
                        "</body></html>");
    }
}

void Server::handleDELETE(const HttpRequest& request, HttpResponse& response) {
    std::string path = HttpRequest::urlDecode(request.getPath());
    
    std::cout << "DELETE request for: " << path << std::endl;
    
    //only allow deleting from uploads directory
    if (path.find("/uploads/") != 0) {
        std::cout << "  DELETE denied: Not in uploads directory" << std::endl;
        response.setStatus(403);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>403 Forbidden</h1>"
                        "<p>Can only delete files from /uploads/ directory.</p>"
                        "<p>Attempted to delete: " + path + "</p>"
                        "</body></html>");
        return;
    }
    
    //build full file path (relative to project root, not www)
    std::string file_path = "." + path;  // ./uploads/filename.txt
    
    std::cout << "Attempting to delete: " << file_path << std::endl;
    
    if (!fileExists(file_path)) {
        std::cout << "File not found: " << file_path << std::endl;
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>404 Not Found</h1>"
                        "<p>File not found: " + path + "</p>"
                        "</body></html>");
        return;
    }
    
    if (std::remove(file_path.c_str()) == 0) {
        std::cout << " File deleted: " << file_path << std::endl;
        
        response.setStatus(200);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body>"
                        "<h1> File Deleted</h1>"
                        "<p>Successfully deleted: " + path + "</p>"
                        "<p><a href='/'> Back to Home</a></p>"
                        "</body></html>");
    } else {
        std::cout << " Failed to delete: " << file_path << std::endl;
        
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1>"
                        "<p>Failed to delete file: " + path + "</p>"
                        "</body></html>");
    }
}

void Server::handleRequest(const HttpRequest& request, HttpResponse& response) {
    std::string method = request.getMethod();
    std::string path = HttpRequest::urlDecode(request.getPath());
    
    std::cout << "\n Handling: " << method << " " << path << std::endl;
    
    if (!isPathSafe(path)) {
        std::cout << " BLOCKED: Path traversal attempt in: " << path << std::endl;
        response.setStatus(403);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>403 Forbidden</h1>"
                        "<p>Path traversal attempt detected.</p></body></html>");
        return;
    }
    
    //route to appropriate handler
    if (method == "GET" || method == "HEAD") {
        handleGET(request, response);
    } else if (method == "POST") {
        handlePOST(request, response);
    } else if (method == "DELETE") {
        handleDELETE(request, response);
    } else {
        response.setStatus(501);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>501 Not Implemented</h1>"
                        "<p>Method " + method + " is not supported.</p>"
                        "</body></html>");
    }
}

std::string Server::generateSessionId() {
    //simple session ID generator (not cryptographically secure - just for demo)
    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(nullptr));
        seeded = true;
    }
    
    std::ostringstream ss;
    ss << "sess_";
    for (int i = 0; i < 16; i++) {
        ss << "0123456789abcdef"[std::rand() % 16];
    }
    
    return ss.str();
}

void Server::handleLogin(const HttpRequest& request, HttpResponse& response) {
    //parse form data
    std::map<std::string, std::string> form_data = 
        const_cast<HttpRequest&>(request).parseFormData();
    
    std::string username = form_data["username"];
    std::string password = form_data["password"];
    
    std::cout << "Login attempt: username=" << username << std::endl;
    
    //simple validation (accept any non-empty username/password for demo)
    if (username.empty() || password.empty()) {
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body><h1>400 Bad Request</h1>"
                        "<p>Username and password required.</p>"
                        "<p><a href='/login.html'>Try Again</a></p>"
                        "</body></html>");
        return;
    }
    
    std::string session_id = generateSessionId();
    
    sessions[session_id] = username;
    
    std::cout << "Login successful. Session ID: " << session_id << std::endl;
    std::cout << "  Active sessions: " << sessions.size() << std::endl;
    
    //set session cookie (expires in 1 hour)
    response.setCookie("session_id", session_id, 3600, "/");
    response.setCookie("username", username, 3600, "/");
    
    response.setStatus(302);
    response.setHeader("Location", "/dashboard");
    response.setHeader("Content-Type", "text/html");
    response.setBody("<html><body>Redirecting to dashboard...</body></html>");
}

void Server::handleDashboard(const HttpRequest& request, HttpResponse& response) {
    std::string session_id = request.getCookie("session_id");
    std::string username = request.getCookie("username");
    
    std::cout << "Dashboard access attempt:" << std::endl;
    std::cout << "  Session ID: " << (session_id.empty() ? "(none)" : session_id) << std::endl;
    std::cout << "  Username: " << (username.empty() ? "(none)" : username) << std::endl;
    
    if (session_id.empty() || sessions.find(session_id) == sessions.end()) {
        std::cout << "  No valid session - redirecting to login" << std::endl;
        
        response.setStatus(302);
        response.setHeader("Location", "/login.html");
        response.setHeader("Content-Type", "text/html");
        response.setBody("<html><body>Please login first...</body></html>");
        return;
    }
    
    std::cout << "  Valid session found" << std::endl;
    
    response.setStatus(200); 
    response.setHeader("Content-Type", "text/html");
    
    std::string html = "<!DOCTYPE html><html><head><title>Dashboard</title>"
                      "<link rel='stylesheet' href='/style.css'>"
                      "<style>"
                      ".dashboard { max-width: 800px; margin: 50px auto; padding: 20px; }"
                      ".welcome { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
                      "color: white; padding: 30px; border-radius: 8px; margin-bottom: 20px; }"
                      ".info-box { background: #f8f9fa; padding: 20px; border-radius: 8px; margin-bottom: 20px; }"
                      ".logout-btn { background-color: #dc3545; color: white; padding: 10px 20px; "
                      "text-decoration: none; border-radius: 4px; display: inline-block; }"
                      ".logout-btn:hover { background-color: #c82333; }"
                      "</style>"
                      "</head><body>"
                      "<div class='dashboard'>"
                      "<div class='welcome'>"
                      "<h1> Welcome, " + username + "!</h1>"
                      "<p>You are successfully logged in.</p>"
                      "</div>"
                      "<div class='info-box'>"
                      "<h2>Session Information</h2>"
                      "<ul>"
                      "<li><strong>Username:</strong> " + username + "</li>"
                      "<li><strong>Session ID:</strong> " + session_id + "</li>"
                      "<li><strong>Active Sessions:</strong> " + std::to_string(sessions.size()) + "</li>"
                      "</ul>"
                      "</div>"
                      "<div class='info-box'>"
                      "<h2>Your Cookies</h2>"
                      "<p>Your browser is storing these cookies:</p>"
                      "<ul>"
                      "<li><strong>session_id:</strong> " + session_id + "</li>"
                      "<li><strong>username:</strong> " + username + "</li>"
                      "</ul>"
                      "</div>"
                      "<p><a href='/logout' class='logout-btn'>Logout</a></p>"
                      "<p><a href='/'> Back to Home</a></p>"
                      "</div>"
                      "</body></html>";
    
    response.setBody(html);
}

void Server::handleLogout(const HttpRequest& request, HttpResponse& response) {
    std::string session_id = request.getCookie("session_id");
    
    if (!session_id.empty()) {
        //remove session
        sessions.erase(session_id);
        std::cout << "Logged out session: " << session_id << std::endl;
    }
    
    //clear cookies by setting Max-Age=0
    response.setCookie("session_id", "", 0, "/");
    response.setCookie("username", "", 0, "/");
    
    response.setStatus(302);
    response.setHeader("Location", "/");
    response.setHeader("Content-Type", "text/html");
    response.setBody("<html><body>Logging out...</body></html>");
}