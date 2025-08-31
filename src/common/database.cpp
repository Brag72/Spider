#include "database.h"
#include <iostream>
#include <sstream>

Database::Database() : connected_(false) {
}

Database::~Database() {
    disconnect();
}

bool Database::connect(const ConfigParser& config) {
    try {
        std::string connectionString = createConnectionString(config);
        conn_ = std::make_unique<pqxx::connection>(connectionString);
        
        if (conn_->is_open()) {
            connected_ = true;
            std::cout << "Successfully connected to database: " << conn_->dbname() << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        connected_ = false;
    }
    
    return false;
}

void Database::disconnect() {
    if (conn_ && conn_->is_open()) {
        conn_->close();
    }
    connected_ = false;
}

bool Database::createTables() {
    if (!connected_) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn_);
        
        // Create documents table
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS documents (
                id SERIAL PRIMARY KEY,
                url VARCHAR(2048) UNIQUE NOT NULL,
                title TEXT,
                content TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )");
        
        // Create words table
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS words (
                id SERIAL PRIMARY KEY,
                word VARCHAR(100) UNIQUE NOT NULL
            )
        )");
        
        // Create word frequencies table (many-to-many relationship)
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS word_frequencies (
                document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE,
                word_id INTEGER REFERENCES words(id) ON DELETE CASCADE,
                frequency INTEGER NOT NULL DEFAULT 1,
                PRIMARY KEY (document_id, word_id)
            )
        )");
        
        // Create indexes for better search performance
        txn.exec("CREATE INDEX IF NOT EXISTS idx_words_word ON words(word)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_word_frequencies_word_id ON word_frequencies(word_id)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_word_frequencies_document_id ON word_frequencies(document_id)");
        
        txn.commit();
        std::cout << "Database tables created successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating tables: " << e.what() << std::endl;
        return false;
    }
}

int Database::insertDocument(const std::string& url, const std::string& title, const std::string& content) {
    if (!connected_) {
        return -1;
    }
    
    try {
        pqxx::work txn(*conn_);
        
        // Check if document already exists
        pqxx::result r = txn.exec_params(
            "SELECT id FROM documents WHERE url = $1",
            url
        );
        
        if (!r.empty()) {
            // Document exists, return existing ID
            return r[0][0].as<int>();
        }
        
        // Insert new document
        pqxx::result insert_result = txn.exec_params(
            "INSERT INTO documents (url, title, content) VALUES ($1, $2, $3) RETURNING id",
            url, title, content
        );
        
        txn.commit();
        
        if (!insert_result.empty()) {
            return insert_result[0][0].as<int>();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error inserting document: " << e.what() << std::endl;
    }
    
    return -1;
}

bool Database::documentExists(const std::string& url) {
    if (!connected_) {
        return false;
    }
    
    try {
        pqxx::nontransaction ntxn(*conn_);
        pqxx::result r = ntxn.exec_params(
            "SELECT 1 FROM documents WHERE url = $1",
            url
        );
        return !r.empty();
    } catch (const std::exception& e) {
        std::cerr << "Error checking document existence: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Document> Database::getAllDocuments() {
    std::vector<Document> documents;
    
    if (!connected_) {
        return documents;
    }
    
    try {
        pqxx::nontransaction ntxn(*conn_);
        pqxx::result r = ntxn.exec("SELECT id, url, title, content, created_at FROM documents");
        
        for (const auto& row : r) {
            Document doc;
            doc.id = row[0].as<int>();
            doc.url = row[1].as<std::string>();
            doc.title = row[2].as<std::string>();
            doc.content = row[3].as<std::string>();
            doc.created_at = row[4].as<std::string>();
            documents.push_back(doc);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting documents: " << e.what() << std::endl;
    }
    
    return documents;
}

int Database::getOrCreateWord(const std::string& word) {
    if (!connected_) {
        return -1;
    }
    
    try {
        pqxx::work txn(*conn_);
        
        // Check if word exists
        pqxx::result r = txn.exec_params(
            "SELECT id FROM words WHERE word = $1",
            word
        );
        
        if (!r.empty()) {
            return r[0][0].as<int>();
        }
        
        // Insert new word
        pqxx::result insert_result = txn.exec_params(
            "INSERT INTO words (word) VALUES ($1) RETURNING id",
            word
        );
        
        txn.commit();
        
        if (!insert_result.empty()) {
            return insert_result[0][0].as<int>();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting/creating word: " << e.what() << std::endl;
    }
    
    return -1;
}

bool Database::insertWordFrequency(int document_id, int word_id, int frequency) {
    if (!connected_) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn_);
        
        // Use ON CONFLICT to update frequency if word already exists for document
        txn.exec_params(R"(
            INSERT INTO word_frequencies (document_id, word_id, frequency) 
            VALUES ($1, $2, $3)
            ON CONFLICT (document_id, word_id) 
            DO UPDATE SET frequency = word_frequencies.frequency + $3
        )", document_id, word_id, frequency);
        
        txn.commit();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error inserting word frequency: " << e.what() << std::endl;
        return false;
    }
}

std::vector<SearchResult> Database::searchDocuments(const std::vector<std::string>& words, int limit) {
    std::vector<SearchResult> results;
    
    if (!connected_ || words.empty()) {
        return results;
    }
    
    try {
        pqxx::nontransaction ntxn(*conn_);
        
        // Build the SQL query for searching documents containing ALL words
        std::ostringstream query;
        query << R"(
            SELECT d.id, d.url, d.title, SUM(wf.frequency) as relevance_score
            FROM documents d
            JOIN word_frequencies wf ON d.id = wf.document_id
            JOIN words w ON wf.word_id = w.id
            WHERE w.word IN (
        )";
        
        // Add placeholders for words
        for (size_t i = 0; i < words.size(); ++i) {
            if (i > 0) query << ", ";
            query << "$" << (i + 1);
        }
        
        query << R"(
            )
            GROUP BY d.id, d.url, d.title
            HAVING COUNT(DISTINCT w.id) = $)" << (words.size() + 1) << R"(
            ORDER BY relevance_score DESC
            LIMIT $)" << (words.size() + 2);
        
        // Execute query based on number of words (simplified approach)
        pqxx::result r;
        if (words.size() == 1) {
            r = ntxn.exec_params(query.str(), words[0], words.size(), limit);
        } else if (words.size() == 2) {
            r = ntxn.exec_params(query.str(), words[0], words[1], words.size(), limit);
        } else if (words.size() == 3) {
            r = ntxn.exec_params(query.str(), words[0], words[1], words[2], words.size(), limit);
        } else if (words.size() == 4) {
            r = ntxn.exec_params(query.str(), words[0], words[1], words[2], words[3], words.size(), limit);
        } else {
            // Too many words, truncate to 4
            r = ntxn.exec_params(query.str(), words[0], words[1], words[2], words[3], 4, limit);
        }
        
        for (const auto& row : r) {
            SearchResult result;
            result.document_id = row[0].as<int>();
            result.url = row[1].as<std::string>();
            result.title = row[2].as<std::string>();
            result.relevance_score = row[3].as<int>();
            results.push_back(result);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error searching documents: " << e.what() << std::endl;
    }
    
    return results;
}

bool Database::isConnected() const {
    return connected_ && conn_ && conn_->is_open();
}

std::string Database::createConnectionString(const ConfigParser& config) {
    std::ostringstream ss;
    ss << "host=" << config.getDatabaseHost()
       << " port=" << config.getDatabasePort()
       << " dbname=" << config.getDatabaseName()
       << " user=" << config.getDatabaseUser()
       << " password=" << config.getDatabasePassword()
       << " client_encoding='UTF-8'"; // Force UTF-8 encoding
    
    return ss.str();
}