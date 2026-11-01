#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <fstream>
#include <sstream>

namespace OSManager {

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::string content_type = "application/json";
    std::string body = "{}";

    std::string to_http_string() const {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
        ss << "Content-Type: " << content_type << "\r\n";
        ss << "Content-Length: " << body.size() << "\r\n";
        ss << "Access-Control-Allow-Origin: *\r\n";
        ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        ss << "Access-Control-Allow-Headers: Content-Type\r\n";
        ss << "Connection: close\r\n\r\n";
        ss << body;
        return ss.str();
    }
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
private:
    int port;
    SOCKET listen_socket = INVALID_SOCKET;
    bool running = false;
    std::map<std::string, RouteHandler> get_routes;
    std::map<std::string, RouteHandler> post_routes;
    std::string static_dir;

    HttpRequest parse_request(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;

        // Request line: METHOD PATH VERSION
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream line_stream(line);
            line_stream >> req.method >> req.path;
        }

        // Headers
        size_t content_length = 0;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break; // End of headers

            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string header_name = line.substr(0, colon);
                std::string header_val = line.substr(colon + 1);
                // trim spaces
                while (!header_val.empty() && header_val.front() == ' ') header_val.erase(header_val.begin());
                req.headers[header_name] = header_val;

                if (header_name == "Content-Length" || header_name == "content-length") {
                    content_length = std::stoul(header_val);
                }
            }
        }

        // Body
        size_t header_end_pos = raw.find("\r\n\r\n");
        if (header_end_pos != std::string::npos) {
            req.body = raw.substr(header_end_pos + 4);
            if (req.body.size() > content_length && content_length > 0) {
                req.body = req.body.substr(0, content_length);
            }
        }

        return req;
    }

    void handle_client(SOCKET client_socket) {
        std::vector<char> buffer(65536, 0);
        int bytes_received = recv(client_socket, buffer.data(), static_cast<int>(buffer.size()) - 1, 0);
        if (bytes_received <= 0) {
            closesocket(client_socket);
            return;
        }

        std::string raw_request(buffer.data(), bytes_received);

        // Check if more body bytes need to be read based on Content-Length
        HttpRequest req = parse_request(raw_request);
        if (req.headers.find("Content-Length") != req.headers.end()) {
            size_t expected_len = std::stoul(req.headers["Content-Length"]);
            while (req.body.size() < expected_len) {
                int more = recv(client_socket, buffer.data(), static_cast<int>(buffer.size()) - 1, 0);
                if (more <= 0) break;
                req.body.append(buffer.data(), more);
            }
        }

        HttpResponse res;

        // Handle CORS preflight
        if (req.method == "OPTIONS") {
            res.status_code = 204;
            res.status_text = "No Content";
            res.body = "";
        } else if (req.method == "GET" || req.method == "HEAD") {
            auto it = get_routes.find(req.path);
            if (it != get_routes.end()) {
                res = it->second(req);
            } else {
                // Try serving static file
                res = serve_static_file(req.path);
            }
            if (req.method == "HEAD") {
                res.body = "";
            }
        } else if (req.method == "POST") {
            auto it = post_routes.find(req.path);
            if (it != post_routes.end()) {
                res = it->second(req);
            } else {
                res.status_code = 404;
                res.status_text = "Not Found";
                res.body = "{\"error\": \"Endpoint not found: " + req.path + "\"}";
            }
        } else {
            res.status_code = 405;
            res.status_text = "Method Not Allowed";
            res.body = "{\"error\": \"Method not allowed\"}";
        }

        std::string response_str = res.to_http_string();
        send(client_socket, response_str.c_str(), static_cast<int>(response_str.size()), 0);
        closesocket(client_socket);
    }

    HttpResponse serve_static_file(const std::string& path) {
        HttpResponse res;
        std::string file_path;

        if (path == "/" || path == "/index.html") {
            file_path = static_dir + "/index.html";
            res.content_type = "text/html; charset=utf-8";
        } else if (path == "/style.css" || path == "/frontend/style.css") {
            file_path = static_dir + "/style.css";
            res.content_type = "text/css; charset=utf-8";
        } else if (path == "/app.js" || path == "/frontend/app.js") {
            file_path = static_dir + "/app.js";
            res.content_type = "application/javascript; charset=utf-8";
        } else {
            // Check relative
            std::string clean_path = path;
            if (!clean_path.empty() && clean_path[0] == '/') clean_path = clean_path.substr(1);
            file_path = static_dir + "/" + clean_path;
            if (file_path.find(".html") != std::string::npos) res.content_type = "text/html; charset=utf-8";
            else if (file_path.find(".css") != std::string::npos) res.content_type = "text/css; charset=utf-8";
            else if (file_path.find(".js") != std::string::npos) res.content_type = "application/javascript; charset=utf-8";
            else res.content_type = "text/plain; charset=utf-8";
        }

        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            // Also try looking in current directory if static_dir not found
            std::string alt_path = "frontend" + (path == "/" ? std::string("/index.html") : path);
            file.open(alt_path, std::ios::binary);
        }

        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            res.status_code = 200;
            res.status_text = "OK";
            res.body = ss.str();
        } else {
            res.status_code = 404;
            res.status_text = "Not Found";
            res.content_type = "application/json";
            res.body = "{\"error\": \"Resource not found: " + path + "\"}";
        }
        return res;
    }

public:
    HttpServer(int p, const std::string& static_path = "frontend")
        : port(p), static_dir(static_path) {}

    ~HttpServer() {
        stop();
    }

    void get(const std::string& path, RouteHandler handler) {
        get_routes[path] = handler;
    }

    void post(const std::string& path, RouteHandler handler) {
        post_routes[path] = handler;
    }

    bool start() {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "[Error] WSAStartup failed.\n";
            return false;
        }
#endif

        listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket == INVALID_SOCKET) {
            std::cerr << "[Error] Socket creation failed.\n";
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        // Allow address reuse
#ifdef _WIN32
        BOOL opt = TRUE;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
        int opt = 1;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "[Error] Bind failed on port " << port << ".\n";
            closesocket(listen_socket);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[Error] Listen failed.\n";
            closesocket(listen_socket);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        running = true;
        std::cout << "========================================================\n";
        std::cout << "  OS Resource Manager Backend (Native HTTP REST API)\n";
        std::cout << "========================================================\n";
        std::cout << "  Status:  Running on http://localhost:" << port << "\n";
        std::cout << "  Static:  Serving UI from " << static_dir << "/\n";
        std::cout << "  API:     REST Endpoints ready under /api/...\n";
        std::cout << "  Press Ctrl+C to terminate server.\n";
        std::cout << "========================================================\n" << std::flush;

        while (running) {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET) {
                if (!running) break;
                continue;
            }

            // Handle client connection in a separate thread
            std::thread(&HttpServer::handle_client, this, client_socket).detach();
        }

        return true;
    }

    void stop() {
        running = false;
        if (listen_socket != INVALID_SOCKET) {
            closesocket(listen_socket);
            listen_socket = INVALID_SOCKET;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

} // namespace OSManager

#endif // HTTP_SERVER_H
