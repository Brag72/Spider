#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../common/config_parser.h"
#include "../common/database.h"
#include "../common/text_indexer.h"

class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();
    
    // Initialize search engine
    bool initialize(const ConfigParser& config);
    
    // Perform search query
    std::vector<SearchResult> search(const std::string& query, int limit = 10);
    
    // Get search statistics
    struct SearchStats {
        size_t total_documents;
        size_t total_words;
        size_t total_word_frequencies;
    };
    
    SearchStats getStats() const;
    
private:
    std::unique_ptr<Database> database_;
    std::unique_ptr<TextIndexer> text_indexer_;
    
    // Parse search query into words
    std::vector<std::string> parseQuery(const std::string& query);
    
    // Validate and clean search words
    std::vector<std::string> validateSearchWords(const std::vector<std::string>& words);
};