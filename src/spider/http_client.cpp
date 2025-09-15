#include "http_client.h"
#include <iostream>
#include <regex>
#include <type_traits>

HttpClient::HttpClient() 
    : timeout_seconds_(30)
    , user_agent_("SearchEngine-Spider/1.0")
    , ssl_ctx_(ssl::context::tlsv12_client) {
    
    // Configure SSL context
    ssl_ctx_.set_default_verify_paths();
    ssl_ctx_.set_verify_mode(ssl::verify_none); // For simplicity, don't verify certificates
}

HttpClient::~HttpClient() {
}

HttpResponse HttpClient::get(const std::string& url) {
    HttpResponse response;
    response.success = false;

    std::string currentUrl = url;
    const int MAX_REDIRECTS = 5;

    for (int i = 0; i < MAX_REDIRECTS; ++i)
    {
        try
        {
            UrlParts Parts = parseUrl(currentUrl);
            if (Parts.scheme.empty() || Parts.host.empty())
            {
                response.error_message = "Invalid URL format: " + currentUrl;
                response.success = false;

                return response;
            }

            if (Parts.is_https)
            {
                response = performHttpsRequest(Parts);
            }
            else
            {
                response = performHttpRequest(Parts);
            }

            if (response.success || response.status_code < 300 || response.status_code >= 400)
            {
                break;
            }

            if (!response.redirect_location.empty())
            {
                std::string prevUrl = currentUrl;
                currentUrl = resolveUrl(prevUrl, response.redirect_location);
            }
            else
            {
                response.error_message = "Redirect response with no location header.";
                response.success = false;
                break;
            }
        }
        catch (const std::exception& e)
        {
            response.error_message = std::string("HTTP request failed: ") + e.what();
            response.success = false;
            return response;
        } 
    }

    if (response.status_code >= 300 && response.status_code < 400)
    {
        response.error_message = "Too many redirects.";
        response.success = false;
    }

    return response;
}

void HttpClient::setTimeout(int timeout_seconds) {
    timeout_seconds_ = timeout_seconds;
}

void HttpClient::setUserAgent(const std::string& user_agent) {
    user_agent_ = user_agent;
}

HttpClient::UrlParts HttpClient::parseUrl(const std::string& url) {
    UrlParts parts;
    
    // Parse URL using regex
    std::regex url_regex(R"(^(https?):\/\/([^:\/]+)(?::(\d+))?(.*)$)");
    std::smatch match;
    
    if (std::regex_match(url, match, url_regex)) {
        parts.scheme = match[1].str();
        parts.host = match[2].str();
        parts.port = match[3].str();
        parts.path = match[4].str();
        
        if (parts.path.empty()) {
            parts.path = "/";
        }
        
        if (parts.port.empty()) {
            parts.port = (parts.scheme == "https") ? "443" : "80";
        }
        
        parts.is_https = (parts.scheme == "https");
    }
    
    return parts;
}

HttpResponse HttpClient::performHttpRequest(const UrlParts& url_parts) {
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    tcp::socket socket(ioc);
    
    // Resolve the host
    auto const results = resolver.resolve(url_parts.host, url_parts.port);
    
    // Connect to the server
    net::connect(socket, results.begin(), results.end());
    
    return executeRequest(socket, url_parts);
}

HttpResponse HttpClient::performHttpsRequest(const UrlParts& url_parts) {
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    ssl::stream<tcp::socket> stream(ioc, ssl_ctx_);
    
    // Set SNI hostname
    if (!SSL_set_tlsext_host_name(stream.native_handle(), url_parts.host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }
    
    // Resolve the host
    auto const results = resolver.resolve(url_parts.host, url_parts.port);
    
    // Connect to the server
    net::connect(stream.next_layer(), results.begin(), results.end());
    
    // Perform SSL handshake
    stream.handshake(ssl::stream_base::client);
    
    return executeRequest(stream, url_parts);
}

std::string HttpClient::resolveUrl(const std::string& baseUrl, const std::string& relativeUrl)
{
    if (relativeUrl.rfind("http://", 0) == 0 || relativeUrl.rfind("https://", 0) == 0)
    {
        return relativeUrl;
    }
    size_t protocolEnd = baseUrl.find("://");
    
    if (protocolEnd == std::string::npos)
    {
        return relativeUrl;
    }

    if (!relativeUrl.empty() && relativeUrl[0] == '/')
    {
        size_t domainEnd = baseUrl.find('/', protocolEnd + 3);

        if (domainEnd != std::string::npos)
        {
            return baseUrl.substr(0, domainEnd) + relativeUrl;
        }
        else
        {
            return baseUrl + relativeUrl;
        }
    }

    std::string base = baseUrl;
    size_t lastSlash = base.rfind('/');

    if (lastSlash > protocolEnd + 2)
    {
        base = base.substr(0, lastSlash + 1);
    }
    else
    {
        if (base.back() != '/')
        {
            base += '/';
        }
    }
    return base + relativeUrl;
}

template<typename Stream>
HttpResponse HttpClient::executeRequest(Stream& stream, const UrlParts& url_parts) {
    HttpResponse response;
    
    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, url_parts.path, 11};
    req.set(http::field::host, url_parts.host);
    req.set(http::field::user_agent, user_agent_);
    req.set(http::field::accept, "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.set(http::field::accept_language, "en-US,en;q=0.5");
    req.set(http::field::connection, "close");
    
    // Send the HTTP request
    http::write(stream, req);
    
    // Declare a container to hold the response
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    
    // Receive the HTTP response
    http::read(stream, buffer, res);
    
    // Extract response data
    response.status_code = static_cast<int>(res.result_int());
    response.body = beast::buffers_to_string(res.body().data());
    response.success = (response.status_code >= 200 && response.status_code < 300);
    
    // Extract content type
    auto content_type_it = res.find(http::field::content_type);
    if (content_type_it != res.end()) {
        response.content_type = content_type_it->value();
    }
    
    // Handle redirects (simple implementation)
    if (response.status_code >= 300 && response.status_code < 400) {
        auto location_it = res.find(http::field::location);
        if (location_it != res.end()) {
            response.redirect_location = location_it->value();
        }
    }
    
    // Gracefully close the stream
    beast::error_code ec;
    if constexpr (std::is_same_v<Stream, ssl::stream<tcp::socket>>) {
        stream.shutdown(ec);
    } else {
        stream.shutdown(tcp::socket::shutdown_both, ec);
    }
        
    return response;
}