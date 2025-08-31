#include "search_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>

SearchEngine::SearchEngine() {
}

SearchEngine::~SearchEngine() {
}

bool SearchEngine::initialize(const ConfigParser& config) {
    // Initialize database connection
    database_ = std::make_unique<Database>();
    if (!database_->connect(config)) {
        std::cerr << "Search engine: Failed to connect to database" << std::endl;
        return false;
    }
    
    // Initialize text indexer for query processing
    text_indexer_ = std::make_unique<TextIndexer>();
    
    std::cout << "Search engine initialized successfully" << std::endl;
    return true;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int limit) {
    std::vector<SearchResult> results;
    
    if (query.empty()) {
        return results;
    }
    
    // Parse and validate query
    std::vector<std::string> query_words = parseQuery(query);
    query_words = validateSearchWords(query_words);
    
    if (query_words.empty()) {
        std::cout << "No valid search words found in query: " << query << std::endl;
        return results;
    }
    
    // Limit to 4 words as specified in requirements
    if (query_words.size() > 4) {
        query_words.resize(4);
    }
    
    std::cout << "Searching for words: ";
    for (const auto& word : query_words) {
        std::cout << "'" << word << "' ";
    }
    std::cout << std::endl;
    
    // Perform database search
    try {
        results = database_->searchDocuments(query_words, limit);
        std::cout << "Found " << results.size() << " results" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
    }
    
    return results;
}

SearchEngine::SearchStats SearchEngine::getStats() const {
    SearchStats stats = {0, 0, 0};
    
    if (!database_ || !database_->isConnected()) {
        return stats;
    }
    
    // In a real implementation, you might want to cache these stats
    // or have dedicated methods in the Database class to get them efficiently
    
    return stats;
}

std::vector<std::string> SearchEngine::parseQuery(const std::string& query) {
    // Use text indexer to tokenize the query
    return text_indexer_->tokenize(query);
}

std::vector<std::string> SearchEngine::validateSearchWords(const std::vector<std::string>& words) {
    std::vector<std::string> valid_words;
    
    for (const std::string& word : words) {
        std::string normalized = text_indexer_->normalizeWord(word);
        
        if (text_indexer_->shouldIndexWord(normalized)) {
            valid_words.push_back(normalized);
        }
    }
    
    return valid_words;
}