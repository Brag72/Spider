#pragma once

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include "../common/config_parser.h"
#include "../common/database.h"
#include "../common/html_parser.h"
#include "../common/text_indexer.h"
#include "http_client.h"
#include "url_queue.h"

class Spider {
public:
    Spider();
    ~Spider();
    
    // Initialize spider with configuration
    bool initialize(const ConfigParser& config);
    
    // Start crawling from the configured start URL
    void startCrawling();
    
    // Stop crawling
    void stopCrawling();
    
    // Get crawling statistics
    struct CrawlStats {
        size_t pages_crawled;
        size_t pages_indexed;
        size_t urls_in_queue;
        size_t total_words_indexed;
        bool is_running;
    };
    
    CrawlStats getStats() const;
    
private:
    ConfigParser config_;
    std::unique_ptr<Database> database_;
    std::unique_ptr<HtmlParser> html_parser_;
    std::unique_ptr<TextIndexer> text_indexer_;
    std::unique_ptr<HttpClient> http_client_;
    std::unique_ptr<UrlQueue> url_queue_;
    
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_;
    std::atomic<size_t> pages_crawled_;
    std::atomic<size_t> pages_indexed_;
    std::atomic<size_t> total_words_indexed_;
    
    int max_depth_;
    int num_threads_;
    std::string start_url_;
    
    // Worker thread function
    void workerThread();
    
    // Process a single URL
    bool processUrl(const UrlQueueItem& item);
    
    // Index a page and store in database
    bool indexPage(const std::string& url, const std::string& title, 
                   const std::string& content);
    
    // Extract and queue new URLs from page content
    void extractAndQueueUrls(const std::string& html_content, 
                            const std::string& base_url, int current_depth);
    
    // Check if URL should be crawled
    bool shouldCrawlUrl(const std::string& url) const;
};