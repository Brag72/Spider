#pragma once

#include <string>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "../common/config_parser.h"
#include "../common/database.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class SearchEngine;

class HttpServer {
public:
    HttpServer();
    ~HttpServer();
    
    // Initialize server with configuration
    bool initialize(const ConfigParser& config);
    
    // Start the server
    void start();
    
    // Stop the server
    void stop();
    
private:
    ConfigParser config_;
    std::unique_ptr<SearchEngine> search_engine_;
    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<tcp::acceptor> acceptor_;
    bool running_;
    int port_;
    
    // Accept incoming connections
    void doAccept();
    
    // Handle a single HTTP session
    void handleSession(tcp::socket socket);
    
    // Process HTTP request and generate response
    template<class Body, class Allocator>
    http::response<http::string_body> handleRequest(
        http::request<Body, http::basic_fields<Allocator>>&& req);
    
    // Handle GET request (search form)
    http::response<http::string_body> handleGet(const std::string& target);
    
    // Handle POST request (search query)
    http::response<http::string_body> handlePost(const std::string& body);
    
    // Load HTML template
    std::string loadTemplate(const std::string& template_name);
    
    // Generate search form HTML
    std::string generateSearchForm();
    
    // Generate search results HTML
    std::string generateSearchResults(const std::string& query, 
                                     const std::vector<struct SearchResult>& results);
    
    // Generate error page HTML
    std::string generateErrorPage(const std::string& error_message);
    
    // URL decode
    std::string urlDecode(const std::string& encoded);
    
    // Parse form data
    std::map<std::string, std::string> parseFormData(const std::string& form_data);
};