#include "text_indexer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

TextIndexer::TextIndexer() {
    initializeLocale();
}

TextIndexer::~TextIndexer() {
}

std::map<std::string, int> TextIndexer::indexText(const std::string& text) {
    std::map<std::string, int> wordFreq;
    
    std::vector<std::string> words = tokenize(text);
    
    for (const auto& word : words) {
        std::string normalized = normalizeWord(word);
        if (shouldIndexWord(normalized)) {
            wordFreq[normalized]++;
        }
    }
    
    return wordFreq;
}

std::vector<std::string> TextIndexer::tokenize(const std::string& text) {
    std::vector<std::string> words;
    
    // Remove punctuation and split by whitespace
    std::string cleanText = removePunctuation(text);
    
    std::istringstream iss(cleanText);
    std::string word;
    
    while (iss >> word) {
        words.push_back(word);
    }
    
    return words;
}

std::string TextIndexer::normalizeWord(const std::string& word) {
    try {
        // Use Boost.Locale for proper Unicode normalization
        std::string normalized = boost::locale::normalize(word, boost::locale::norm_nfd);
        normalized = boost::locale::fold_case(normalized);
        return normalized;
    }
    catch (const std::exception& e) {
        // Fallback to simple lowercase
        std::string result = word;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
}

bool TextIndexer::shouldIndexWord(const std::string& word) {
    // Check word length constraints (in characters, not bytes)
    if (word.empty() || word.length() > 64) { // Increased limit for Unicode
        return false;
    }

    // Check if word contains valid characters (Unicode letters)
    // Use Boost.Locale for proper Unicode character classification
    try {
        for (char c : word) {
            // Simple check - in real implementation, use Boost.Locale character classification
            if (!std::isalpha(static_cast<unsigned char>(c)) &&
                !(c & 0x80)) { // Allow high-bit characters (Unicode)
                return false;
            }
        }
    }
    catch (...) {
        // Fallback to simple check
        return word.length() >= 2; // Minimal length for non-Latin scripts
    }

    return true;
}

void TextIndexer::initializeLocale() {
    try {
        locale_gen_.locale_cache_enabled(true);
        locale_gen_.generate("en_US.UTF-8");
        locale_ = locale_gen_("en_US.UTF-8");

        // Set global locale
        std::locale::global(locale_);
        std::cout << "Initialized locale: " << std::use_facet<boost::locale::info>(locale_).name() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Locale initialization error: " << e.what() << std::endl;
        // Fallback to system locale
        try {
            locale_ = locale_gen_("");
        }
        catch (const std::exception& e2) {
            // Use C locale as last resort
            locale_ = std::locale::classic();
            std::cerr << "Warning: Could not initialize UTF-8 locale, using C locale" << std::endl;
        }
    }
}

std::string TextIndexer::removePunctuation(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    for (char c : text) {
        // Keep Unicode characters and alphanumeric
        if (std::isalnum(static_cast<unsigned char>(c)) ||
            std::isspace(static_cast<unsigned char>(c)) ||
            (c & 0x80)) { // Keep Unicode characters (high bit set)
            result += c;
        }
        else {
            result += ' '; // Replace punctuation with space
        }
    }

    return result;
}