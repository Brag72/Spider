#pragma once

#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>
#include "config_parser.h"

struct Document {
    int id;
    std::string url;
    std::string title;
    std::string content;
    std::string created_at;
};

struct Word {
    int id;
    std::string word;
};

struct WordFrequency {
    int document_id;
    int word_id;
    int frequency;
};

struct SearchResult {
    int document_id;
    std::string url;
    std::string title;
    int relevance_score;
};

class Database {
public:
    Database();
    ~Database();
    
    bool connect(const ConfigParser& config);
    void disconnect();
    
    // Schema management
    bool createTables();
    
    // Document operations
    int insertDocument(const std::string& url, const std::string& title, const std::string& content);
    bool documentExists(const std::string& url);
    std::vector<Document> getAllDocuments();
    
    // Word operations
    int getOrCreateWord(const std::string& word);
    bool insertWordFrequency(int document_id, int word_id, int frequency);
    
    // Search operations
    std::vector<SearchResult> searchDocuments(const std::vector<std::string>& words, int limit = 10);
    
    // Utility
    bool isConnected() const;
    
private:
    std::unique_ptr<pqxx::connection> conn_;
    bool connected_;
    
    std::string createConnectionString(const ConfigParser& config);
};