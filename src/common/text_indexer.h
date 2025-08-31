#pragma once

#include <string>
#include <vector>
#include <map>
#include <boost/locale.hpp>

class TextIndexer {
public:
    TextIndexer();
    ~TextIndexer();
    
    // Process text and return word frequency map
    std::map<std::string, int> indexText(const std::string& text);
    
    // Tokenize text into words
    std::vector<std::string> tokenize(const std::string& text);
    
    // Clean and normalize word
    std::string normalizeWord(const std::string& word);
    
    // Check if word should be indexed (length constraints, etc.)
    bool shouldIndexWord(const std::string& word);
    
private:
    boost::locale::generator locale_gen_;
    std::locale locale_;
    
    void initializeLocale();
    std::string removePunctuation(const std::string& text);
};