#include "http_server.h"
#include "search_engine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <map>

HttpServer::HttpServer() : running_(false), port_(8080) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::initialize(const ConfigParser& config) {
    config_ = config;
    port_ = config_.getServerPort();
    
    // Initialize search engine
    search_engine_ = std::make_unique<SearchEngine>();
    if (!search_engine_->initialize(config_)) {
        std::cerr << "Failed to initialize search engine" << std::endl;
        return false;
    }
    
    // Initialize networking
    ioc_ = std::make_unique<net::io_context>();
    
    std::cout << "HTTP server initialized on port " << port_ << std::endl;
    return true;
}

void HttpServer::start() {
    if (running_) {
        std::cout << "Server is already running" << std::endl;
        return;
    }
    
    try {
        // Create acceptor
        tcp::endpoint endpoint{tcp::v4(), static_cast<unsigned short>(port_)};
        acceptor_ = std::make_unique<tcp::acceptor>(*ioc_, endpoint);
        
        running_ = true;
        
        std::cout << "Search server started on http://localhost:" << port_ << std::endl;
        
        // Start accepting connections
        doAccept();
        
        // Run the I/O service
        ioc_->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        running_ = false;
    }
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (ioc_) {
        ioc_->stop();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void HttpServer::doAccept() {
    if (!running_) {
        return;
    }
    
    acceptor_->async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                // Handle the session in a separate thread
                std::thread(&HttpServer::handleSession, this, std::move(socket)).detach();
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            
            // Continue accepting new connections
            doAccept();
        });
}

void HttpServer::handleSession(tcp::socket socket) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        
        // Read the request
        http::read(socket, buffer, req);
        
        // Process the request
        auto response = handleRequest(std::move(req));
        
        // Send the response
        http::write(socket, response);
        
        // Gracefully close the socket
        socket.shutdown(tcp::socket::shutdown_send);
        
    } catch (const std::exception& e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

template<class Body, class Allocator>
http::response<http::string_body> HttpServer::handleRequest(
    http::request<Body, http::basic_fields<Allocator>>&& req) {
    
    // Handle different HTTP methods
    if (req.method() == http::verb::get) {
        return handleGet(std::string(req.target()));
    } else if (req.method() == http::verb::post) {
        return handlePost(req.body());
    } else {
        // Method not allowed
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::server, "SearchEngine/1.0");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Method not allowed";
        res.prepare_payload();
        return res;
    }
}

http::response<http::string_body> HttpServer::handleGet(const std::string& target) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::server, "SearchEngine/1.0");
    res.set(http::field::content_type, "text/html; charset=utf-8");
    
    // Serve search form
    res.body() = generateSearchForm();
    res.prepare_payload();
    
    return res;
}

http::response<http::string_body> HttpServer::handlePost(const std::string& body) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::server, "SearchEngine/1.0");
    res.set(http::field::content_type, "text/html; charset=utf-8");
    
    try {
        // Parse form data
        auto form_data = parseFormData(body);
        std::string query = form_data["query"];
        
        if (query.empty()) {
            res.body() = generateErrorPage("Empty search query");
        } else {
            // Perform search
            std::vector<SearchResult> results = search_engine_->search(query, 10);
            res.body() = generateSearchResults(query, results);
        }
        
    } catch (const std::exception& e) {
        res.body() = generateErrorPage("Internal server error: " + std::string(e.what()));
    }
    
    res.prepare_payload();
    return res;
}

std::string HttpServer::loadTemplate(const std::string& template_name) {
    std::ifstream file("templates/" + template_name);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string HttpServer::generateSearchForm() {
    std::stringstream html;
    
    html << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Search Engine</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .search-container {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 {
            color: #333;
            margin-bottom: 30px;
        }
        .search-form {
            margin: 20px 0;
        }
        input[type="text"] {
            width: 400px;
            padding: 12px;
            font-size: 16px;
            border: 2px solid #ddd;
            border-radius: 5px;
            margin-right: 10px;
        }
        input[type="submit"] {
            padding: 12px 24px;
            font-size: 16px;
            background-color: #4285f4;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background-color: #3367d6;
        }
        .info {
            margin-top: 20px;
            color: #666;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="search-container">
        <h1>Search Engine</h1>
        <form class="search-form" method="post" action="/">
            <input type="text" name="query" placeholder="Enter your search query..." maxlength="100" required>
            <input type="submit" value="Search">
        </form>
        <div class="info">
            <p>Enter up to 4 words to search for documents.</p>
            <p>Search is case-insensitive and matches whole words.</p>
        </div>
    </div>
</body>
</html>
    )";
    
    return html.str();
}

std::string HttpServer::generateSearchResults(const std::string& query, 
                                             const std::vector<SearchResult>& results) {
    std::stringstream html;
    
    html << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Search Results - )" << query << R"(</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 20px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .search-header {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        .search-form {
            text-align: center;
        }
        input[type="text"] {
            width: 400px;
            padding: 10px;
            font-size: 16px;
            border: 2px solid #ddd;
            border-radius: 5px;
            margin-right: 10px;
        }
        input[type="submit"] {
            padding: 10px 20px;
            font-size: 16px;
            background-color: #4285f4;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        .results-info {
            margin: 20px 0;
            color: #666;
        }
        .result {
            background: white;
            padding: 20px;
            margin-bottom: 15px;
            border-radius: 5px;
            box-shadow: 0 1px 5px rgba(0,0,0,0.1);
        }
        .result-title {
            font-size: 18px;
            color: #1a0dab;
            text-decoration: none;
            font-weight: normal;
        }
        .result-title:hover {
            text-decoration: underline;
        }
        .result-url {
            color: #006621;
            font-size: 14px;
            margin: 5px 0;
        }
        .result-score {
            color: #666;
            font-size: 12px;
        }
        .no-results {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="search-header">
        <form class="search-form" method="post" action="/">
            <input type="text" name="query" value=")" << query << R"(" maxlength="100" required>
            <input type="submit" value="Search">
        </form>
    </div>
    
    <div class="results-info">
        Search results for: <strong>)" << query << R"(</strong>
    </div>
    )";
    
    if (results.empty()) {
        html << R"(
    <div class="no-results">
        <h3>No results found</h3>
        <p>Try different keywords or check your spelling.</p>
    </div>
        )";
    } else {
        html << "<div class=\"results-info\">Found " << results.size() << " results</div>\n";
        
        for (const auto& result : results) {
            html << R"(
    <div class="result">
        <a href=")" << result.url << R"(" class="result-title" target="_blank">)"
             << (result.title.empty() ? result.url : result.title) << R"(</a>
        <div class="result-url">)" << result.url << R"(</div>
        <div class="result-score">Relevance score: )" << result.relevance_score << R"(</div>
    </div>
            )";
        }
    }
    
    html << R"(
</body>
</html>
    )";
    
    return html.str();
}

std::string HttpServer::generateErrorPage(const std::string& error_message) {
    std::stringstream html;
    
    html << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Search Error</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 100px auto;
            padding: 20px;
            text-align: center;
            background-color: #f5f5f5;
        }
        .error-container {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .error-title {
            color: #d32f2f;
            margin-bottom: 20px;
        }
        .error-message {
            color: #666;
            margin-bottom: 30px;
        }
        .back-link {
            color: #4285f4;
            text-decoration: none;
        }
        .back-link:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="error-container">
        <h1 class="error-title">Search Error</h1>
        <p class="error-message">)" << error_message << R"(</p>
        <a href="/" class="back-link">← Back to Search</a>
    </div>
</body>
</html>
    )";
    
    return html.str();
}

std::string HttpServer::urlDecode(const std::string& encoded) {
    std::string decoded;
    size_t len = encoded.length();
    
    for (size_t i = 0; i < len; ++i) {
        if (encoded[i] == '%' && i + 2 < len) {
            int hex_value;
            std::istringstream hex_stream(encoded.substr(i + 1, 2));
            if (hex_stream >> std::hex >> hex_value) {
                decoded += static_cast<char>(hex_value);
                i += 2;
            } else {
                decoded += encoded[i];
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    
    return decoded;
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string& form_data) {
    std::map<std::string, std::string> data;
    
    std::istringstream stream(form_data);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, eq_pos));
            std::string value = urlDecode(pair.substr(eq_pos + 1));
            data[key] = value;
        }
    }
    
    return data;
}