#include "html_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <boost/locale/encoding.hpp>

// HtmlParser implementation
HtmlParser::HtmlParser() {
}

HtmlParser::~HtmlParser() {
}

std::string HtmlParser::extractText(const std::string& html) {
    // First, try to detect encoding
    std::string encoding = "UTF-8"; // Default assumption

    // Check for meta charset tag
    std::regex charset_regex(R"(<meta[^>]*charset\s*=\s*["']?([^"'>\s]+)[^>]*>)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(html, match, charset_regex)) {
        encoding = match[1].str();
        // Convert to uppercase for consistency
        std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::toupper);
    }

    std::string text = removeHtmlTags(html);

    // If not UTF-8, convert to UTF-8 (simplified approach)
    if (encoding != "UTF-8" && encoding != "UTF8") {
        try {
            text = boost::locale::conv::to_utf<char>(text, encoding);
        }
        catch (...) {
            // Conversion failed, keep original text
        }
    }

    // Remove extra whitespace
    std::regex whitespace_regex(R"(\s+)");
    text = std::regex_replace(text, whitespace_regex, " ");

    // Trim leading and trailing whitespace
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
    text.erase(std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), text.end());

    return text;
}

std::string HtmlParser::extractTitle(const std::string& html) {
    std::regex title_regex(R"(<title[^>]*>(.*?)</title>)", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, title_regex)) {
        std::string title = match[1].str();
        return extractText(title); // Remove any remaining HTML tags from title
    }
    
    return "";
}

std::vector<std::string> HtmlParser::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::regex link_regex(R"(<a[^>]*href\s*=\s*["\']([^"\']*)["\'][^>]*>)", std::regex_constants::icase);
    
    std::sregex_iterator iter(html.begin(), html.end(), link_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string link = (*iter)[1].str();
        
        // Skip empty links, javascript, mailto, etc.
        if (link.empty() || link.find("javascript:") == 0 || 
            link.find("mailto:") == 0 || link.find("#") == 0) {
            continue;
        }
        
        // Resolve relative URLs
        if (!baseUrl.empty()) {
            link = resolveUrl(baseUrl, link);
        }
        
        links.push_back(link);
    }
    
    return links;
}

std::string HtmlParser::removeHtmlTags(const std::string& html) {
    std::regex tag_regex("<[^>]*>");
    return std::regex_replace(html, tag_regex, " ");
}

std::string HtmlParser::resolveUrl(const std::string& baseUrl, const std::string& relativeUrl) {
    // Simple URL resolution - in a real implementation, you'd want more robust URL parsing
    if (relativeUrl.find("http://") == 0 || relativeUrl.find("https://") == 0) {
        return relativeUrl; // Already absolute
    }
    
    if (relativeUrl[0] == '/') {
        // Absolute path - extract domain from base URL
        size_t protocolEnd = baseUrl.find("://");
        if (protocolEnd != std::string::npos) {
            size_t domainEnd = baseUrl.find('/', protocolEnd + 3);
            if (domainEnd != std::string::npos) {
                return baseUrl.substr(0, domainEnd) + relativeUrl;
            } else {
                return baseUrl + relativeUrl;
            }
        }
    }
    
    // Relative path - append to base URL
    std::string base = baseUrl;
    if (base.back() != '/') {
        base += '/';
    }
    return base + relativeUrl;
}