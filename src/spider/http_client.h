#pragma once

#include <string>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

struct HttpResponse {
    int status_code;
    std::string body;
    std::string content_type;
    bool success;
    std::string error_message;
    std::string redirect_location;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Fetch URL and return response
    HttpResponse get(const std::string& url);
    
    // Set timeout for requests (in seconds)
    void setTimeout(int timeout_seconds);
    
    // Set user agent string
    void setUserAgent(const std::string& user_agent);
    
private:
    int timeout_seconds_;
    std::string user_agent_;
    ssl::context ssl_ctx_;
    
    struct UrlParts {
        std::string scheme;
        std::string host;
        std::string port;
        std::string path;
        bool is_https;
    };
    
    UrlParts parseUrl(const std::string& url);
    HttpResponse performHttpRequest(const UrlParts& url_parts);
    HttpResponse performHttpsRequest(const UrlParts& url_parts);
    std::string resolveUrl(const std::string& baseUrl, const std::string& relativeUrl);
    
    template<typename Stream>
    HttpResponse executeRequest(Stream& stream, const UrlParts& url_parts);
};