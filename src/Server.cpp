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
#include <dirent.h>  

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
    } else if (path == "/files") {
        handleFilesList(request, response);
        return;
    } else if (path == "/delete-all") {
        handleDeleteAll(request, response);
        return;
    }
    
    //handle /uploads/ routes - serve from uploads directory
    if (path.find("/uploads/") == 0) {
        std::string clean_path = path;
        size_t query_pos = clean_path.find('?');
        bool force_download = false;
        
        if (query_pos != std::string::npos) {
            std::string query = clean_path.substr(query_pos + 1);
            clean_path = clean_path.substr(0, query_pos);
            
            //check if download=1 is in query
            if (query.find("download=1") != std::string::npos) {
                force_download = true;
            }
        }
        
        std::string file_path = "." + clean_path;  // ./uploads/filename.ext
        
        std::cout << "SERVING UPLOADED FILE " << file_path 
                  << (force_download ? " (download)" : " (view)") << std::endl;
        
        if (!fileExists(file_path)) {
            std::cout << "FILE NOT FOUND " << file_path << std::endl;
            response.setStatus(404);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                            "<p>The requested resource " + path + " was not found.</p>"
                            "</body></html>");
            return;
        }
        
        bool success;
        std::string content = readFile(file_path, success);
        
        if (!success) {
            std::cout << "FAILED TO READ FILE " << file_path << std::endl;
            response.setStatus(500);
            response.setHeader("Content-Type", "text/html");
            response.setBody("<!DOCTYPE html><html><body><h1>500 Internal Server Error</h1>"
                            "<p>Failed to read file.</p></body></html>");
            return;
        }
        
        response.setStatus(200);
        response.setHeader("Content-Type", getContentType(clean_path));
        
        //add Content-Disposition header
        size_t last_slash = clean_path.find_last_of('/');
        std::string filename = (last_slash != std::string::npos) ? 
                              clean_path.substr(last_slash + 1) : clean_path;
        
        if (force_download) {
            //force download
            response.setHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        } else {
            //try to display inline (browser decides based on content-type)
            response.setHeader("Content-Disposition", "inline; filename=\"" + filename + "\"");
        }
        
        response.setBody(content);
        
        std::cout << "SERVED UPLOADED FILE " << file_path 
                  << " (" << content.length() << " bytes)" << std::endl;
        return;
    }
    
    //default to index.html if path is /
    if (path == "/") {
        path = "/index.html";
    }
    
    //regular file serving from www/
    std::string file_path = www_root + path;
    
    std::cout << "LOOKING FOR FILE " << file_path << std::endl;
    
    if (!fileExists(file_path)) {
        std::cout << "FILE NOT FOUND " << file_path << std::endl;
        response.setStatus(404);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
                        "<p>The requested resource " + path + " was not found.</p>"
                        "</body></html>");
        return;
    }
    
    bool success;
    std::string content = readFile(file_path, success);
    
    if (!success) {
        std::cout << "FAILED TO READ FILE " << file_path << std::endl;
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><body><h1>500 Internal Server Error</h1>"
                        "<p>Failed to read file.</p></body></html>");
        return;
    }
    
    response.setStatus(200);
    response.setHeader("Content-Type", getContentType(path));
    response.setBody(content);
    
    std::cout << "SERVED FILE: " << file_path 
              << " (" << content.length() << " bytes)" << std::endl;
}

void Server::handlePOST(const HttpRequest& request, HttpResponse& response) {
    std::string path = HttpRequest::urlDecode(request.getPath());
    
    std::cout << "POST request to: " << path << std::endl;
    
    //route to appropriate handler
    if (path == "/login") {
        handleLogin(request, response);
        return;
    } else if (path == "/upload") { 
        handleUpload(request, response);
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
                              "<h1>Form Submitted Successfully!</h1>"
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

void Server::handleUpload(const HttpRequest& request, HttpResponse& response) {
    std::cout << "PROCESSING FILE UPLOAD..." << std::endl;
    
    //parse multipart form data
    std::vector<UploadedFile> files;
    std::map<std::string, std::string> form_fields = 
        const_cast<HttpRequest&>(request).parseMultipartFormData(files);
    
    std::string description = form_fields["description"];
    
    if (files.empty()) {
        std::cout << "NO FILES UPLOADED" << std::endl;
        response.setStatus(400);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><head><title>Error</title></head><body>"
                        "<h1>400 Bad Request</h1>"
                        "<p>No file was uploaded.</p>"
                        "<p><a href='/upload.html'>Try Again</a></p>"
                        "</body></html>");
        return;
    }
    
    //FOR EACH uploaded file
    std::vector<std::string> saved_files;
    
    for (const auto& file : files) {
        //create safe filename (prevent path traversal)
        std::string safe_filename = file.filename;
        
        //remove any path components
        size_t last_slash = safe_filename.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            safe_filename = safe_filename.substr(last_slash + 1);
        }
        
        //build full path
        std::string file_path = uploads_root + "/" + safe_filename;
        
        std::cout << "SAVING TO: " << file_path << std::endl;
        
        //write file
        std::ofstream out_file(file_path, std::ios::binary);
        if (out_file.is_open()) {
            out_file.write(file.content.c_str(), file.content.length());
            out_file.close();
            
            saved_files.push_back(safe_filename);
            std::cout << "SAVED: " << safe_filename 
                      << " (" << file.content.length() << " bytes)" << std::endl;
        } else {
            std::cout << "FAILED TO SAVE: " << safe_filename << std::endl;
        }
    }
    
    //success response
    if (!saved_files.empty()) {
        response.setStatus(200);
        response.setHeader("Content-Type", "text/html");
        
        std::string html = "<!DOCTYPE html><html><head><title>Upload Success</title>"
                          "<style>"
                          "body { font-family: Arial; max-width: 800px; margin: 50px auto; padding: 20px; }"
                          "h1 { color: #28a745; }"
                          ".file-list { background: #f8f9fa; padding: 20px; border-radius: 5px; }"
                          ".file-item { background: white; margin: 10px 0; padding: 15px; border-radius: 3px; }"
                          "a { color: #007bff; text-decoration: none; }"
                          "</style>"
                          "</head><body>"
                          "<h1>Upload Successful!</h1>";
        
        if (!description.empty()) {
            html += "<p><strong>Description:</strong> " + description + "</p>";
        }
        
        html += "<div class='file-list'><h2>Uploaded Files:</h2>";
        
        for (const auto& filename : saved_files) {
            std::string encoded_filename = HttpRequest::urlEncode(filename);
            html += "<div class='file-item'>"
                   "<strong>" + filename + "</strong><br>"
                   "<a href='/uploads/" + encoded_filename + "?download=1'>Download</a> | "
                   "<a href='/uploads/" + encoded_filename + "' target='_blank'>View</a>"
                   "</div>";
        }
        
        html += "</div>"
               "<p style='margin-top: 20px;'>"
               "<a href='/upload.html'>Upload Another</a> | "
               "<a href='/files'>View All Files</a> | "
               "<a href='/'>Home</a>"
               "</p></body></html>";
        
        response.setBody(html);
    } else {
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><body><h1>500 Error</h1>"
                        "<p>Failed to save file.</p></body></html>");
    }
}

void Server::handleFilesList(const HttpRequest& request, HttpResponse& response) {
    std::cout << "LISTING UPLOADED FILES..." << std::endl;
    
    DIR* dir = opendir(uploads_root.c_str());
    if (!dir) {
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><body><h1>Error</h1>"
                        "<p>Could not open uploads directory.</p></body></html>");
        return;
    }
    
    // build file list
    std::vector<std::string> files;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // skip . and .. and hidden files
        if (filename[0] == '.') {
            continue;
        }
        
        // skip submissions.txt
        if (filename == "submissions.txt") {
            continue;
        }
        
        files.push_back(filename);
    }
    
    closedir(dir);
    
    //sort files alphabetically
    std::sort(files.begin(), files.end());
    
    response.setStatus(200);
    response.setHeader("Content-Type", "text/html");
    
    std::string html = "<!DOCTYPE html><html><head><title>Uploaded Files</title>"
                      "<style>"
                      "body { font-family: Arial; max-width: 1000px; margin: 50px auto; padding: 20px; }"
                      "h1 { color: #333; border-bottom: 3px solid #007bff; padding-bottom: 10px; }"
                      ".header-actions { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }"
                      ".delete-all-btn { background: #dc3545; color: white; padding: 12px 25px; text-decoration: none; border-radius: 5px; font-weight: bold; }"
                      ".delete-all-btn:hover { background: #c82333; }"
                      ".files-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap: 20px; margin: 30px 0; }"
                      ".file-card { background: white; border: 1px solid #ddd; border-radius: 8px; padding: 20px; text-align: center; transition: transform 0.2s; }"
                      ".file-card:hover { transform: translateY(-5px); box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
                      ".file-icon { font-size: 48px; margin-bottom: 10px; }"
                      ".file-name { font-weight: bold; margin: 10px 0; word-break: break-word; }"
                      ".file-actions { margin-top: 15px; }"
                      ".file-actions a { margin: 0 5px; padding: 8px 15px; background: #007bff; color: white; text-decoration: none; border-radius: 4px; display: inline-block; font-size: 14px; }"
                      ".file-actions a:hover { background: #0056b3; }"
                      ".empty-state { text-align: center; padding: 60px 20px; color: #999; }"
                      ".upload-btn { background: #28a745; color: white; padding: 15px 30px; text-decoration: none; border-radius: 5px; display: inline-block; font-size: 18px; margin: 20px 0; }"
                      ".upload-btn:hover { background: #218838; }"
                      "</style>"
                      "<script>"
                      "function confirmDeleteAll() {"
                      "  if (confirm('Are you sure you want to delete ALL uploaded files? This cannot be undone!')) {"
                      "    window.location.href = '/delete-all';"
                      "  }"
                      "}"
                      "</script>"
                      "</head><body>"
                      "<h1>Uploaded Files</h1>"
                      "<div class='header-actions'>"
                      "<p>Total files: " + std::to_string(files.size()) + "</p>";
    
    if (!files.empty()) {
        html += "<a href='javascript:void(0)' onclick='confirmDeleteAll()' class='delete-all-btn'>Delete All Files</a>";
    }
    
    html += "</div>";
    
    if (files.empty()) {
        html += "<div class='empty-state'>"
               "<h2>No files uploaded yet</h2>"
               "<p>Upload your first file to get started!</p>"
               "</div>";
    } else {
        html += "<div class='files-grid'>";
        
        for (const auto& filename : files) {
            // Get file extension for icon (using text labels instead of emojis)
            std::string icon = "[FILE]";
            std::string ext = filename.substr(filename.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif") {
                icon = "[IMG]";
            } else if (ext == "pdf") {
                icon = "[PDF]";
            } else if (ext == "txt") {
                icon = "[TXT]";
            } else if (ext == "zip" || ext == "rar") {
                icon = "[ZIP]";
            }
            
            std::string encoded_filename = HttpRequest::urlEncode(filename);
            html += "<div class='file-card'>"
                   "<div class='file-icon'>" + icon + "</div>"
                   "<div class='file-name'>" + filename + "</div>"
                   "<div class='file-actions'>"
                   "<a href='/uploads/" + encoded_filename + "?download=1' download>Download</a>"
                   "<a href='/uploads/" + encoded_filename + "' target='_blank'>View</a>"
                   "</div>"
                   "</div>";
        }
        
        html += "</div>";
    }
    
    html += "<div style='text-align: center;'>"
           "<a href='/upload.html' class='upload-btn'>Upload New File</a><br>"
           "<a href='/'>Back to Home</a>"
           "</div>"
           "</body></html>";
    
    response.setBody(html);
}

void Server::handleDeleteAll(const HttpRequest& request, HttpResponse& response) {
    std::cout << "DELETING ALL UPLOADED FILES..." << std::endl;
    
    DIR* dir = opendir(uploads_root.c_str());
    if (!dir) {
        response.setStatus(500);
        response.setHeader("Content-Type", "text/html");
        response.setBody("<!DOCTYPE html><html><body><h1>Error</h1>"
                        "<p>Could not open uploads directory.</p></body></html>");
        return;
    }
    
    //delete all files (except submissions.txt and .gitkeep)
    int deleted_count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        if (filename[0] == '.') {
            continue;
        }
        
        if (filename == "submissions.txt") {
            continue;
        }
        
        std::string file_path = uploads_root + "/" + filename;
        if (std::remove(file_path.c_str()) == 0) {
            std::cout << " DELETED FILE " << filename << std::endl;
            deleted_count++;
        } else {
            std::cout << "  FAILED TO DELETE: " << filename << std::endl;
        }
    }
    
    closedir(dir);
    
    std::cout << "DELETED " << deleted_count << " files" << std::endl;
    
    response.setStatus(200);
    response.setHeader("Content-Type", "text/html");
    
    std::string html = "<!DOCTYPE html><html><head><title>Files Deleted</title>"
                      "<style>"
                      "body { font-family: Arial; max-width: 600px; margin: 100px auto; text-align: center; }"
                      "h1 { color: #dc3545; }"
                      ".message { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; "
                      "padding: 20px; border-radius: 5px; margin: 20px 0; }"
                      "a { display: inline-block; margin: 10px; padding: 10px 20px; "
                      "background: #007bff; color: white; text-decoration: none; border-radius: 4px; }"
                      "a:hover { background: #0056b3; }"
                      "</style>"
                      "</head><body>"
                      "<h1>Files Deleted</h1>"
                      "<div class='message'>"
                      "<p><strong>" + std::to_string(deleted_count) + " files</strong> have been deleted.</p>"
                      "</div>"
                      "<a href='/files'>Back to Files</a>"
                      "<a href='/'>Home</a>"
                      "</body></html>";
    
    response.setBody(html);
}