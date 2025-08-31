#pragma once

#include <string>
#include <vector>
#include <regex>

class HtmlParser {
public:
    HtmlParser();
    ~HtmlParser();
    
    // Extract text content from HTML
    std::string extractText(const std::string& html);
    
    // Extract title from HTML
    std::string extractTitle(const std::string& html);
    
    // Extract all links from HTML
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl = "");
    
private:
    std::string removeHtmlTags(const std::string& html);
    std::string resolveUrl(const std::string& baseUrl, const std::string& relativeUrl);
};