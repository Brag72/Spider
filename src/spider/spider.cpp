#include "spider.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

Spider::Spider() 
    : running_(false)
    , pages_crawled_(0)
    , pages_indexed_(0)
    , total_words_indexed_(0)
    , max_depth_(2)
    , num_threads_(4) {
}

Spider::~Spider() {
    stopCrawling();
}

bool Spider::initialize(const ConfigParser& config) {
    config_ = config;
    
    // Initialize database
    database_ = std::make_unique<Database>();
    if (!database_->connect(config_)) {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }
    
    if (!database_->createTables()) {
        std::cerr << "Failed to create database tables" << std::endl;
        return false;
    }
    
    // Initialize other components
    html_parser_ = std::make_unique<HtmlParser>();
    text_indexer_ = std::make_unique<TextIndexer>();
    http_client_ = std::make_unique<HttpClient>();
    url_queue_ = std::make_unique<UrlQueue>();
    
    // Get configuration values
    start_url_ = config_.getStartUrl();
    max_depth_ = config_.getCrawlDepth();
    
    if (start_url_.empty()) {
        std::cerr << "Start URL not configured" << std::endl;
        return false;
    }
    
    std::cout << "Spider initialized successfully" << std::endl;
    std::cout << "Start URL: " << start_url_ << std::endl;
    std::cout << "Max depth: " << max_depth_ << std::endl;
    std::cout << "Worker threads: " << num_threads_ << std::endl;
    
    return true;
}

void Spider::startCrawling() {
    if (running_) {
        std::cout << "Spider is already running" << std::endl;
        return;
    }
    
    running_ = true;
    pages_crawled_ = 0;
    pages_indexed_ = 0;
    total_words_indexed_ = 0;
    
    // Add start URL to queue
    url_queue_->enqueue(start_url_, 0);
    
    // Start worker threads
    for (int i = 0; i < num_threads_; ++i) {
        worker_threads_.emplace_back(&Spider::workerThread, this);
    }
    
    std::cout << "Spider started crawling with " << num_threads_ << " threads" << std::endl;
    
    // Monitor progress
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        auto stats = getStats();
        std::cout << "Progress: " << stats.pages_crawled << " pages crawled, "
                  << stats.pages_indexed << " pages indexed, "
                  << stats.urls_in_queue << " URLs in queue, "
                  << stats.total_words_indexed << " total words indexed" << std::endl;
        
        // Stop if queue is empty and all threads are idle
        if (stats.urls_in_queue == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (url_queue_->empty()) {
                std::cout << "Queue is empty, stopping crawling" << std::endl;
                break;
            }
        }
    }
    
    stopCrawling();
}

void Spider::stopCrawling() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    url_queue_->stop();
    
    // Wait for all worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    
    auto stats = getStats();
    std::cout << "Crawling stopped. Final stats:" << std::endl;
    std::cout << "  Pages crawled: " << stats.pages_crawled << std::endl;
    std::cout << "  Pages indexed: " << stats.pages_indexed << std::endl;
    std::cout << "  Total words indexed: " << stats.total_words_indexed << std::endl;
}

Spider::CrawlStats Spider::getStats() const {
    CrawlStats stats;
    stats.pages_crawled = pages_crawled_.load();
    stats.pages_indexed = pages_indexed_.load();
    stats.urls_in_queue = url_queue_->getPendingCount();
    stats.total_words_indexed = total_words_indexed_.load();
    stats.is_running = running_.load();
    return stats;
}

void Spider::workerThread() {
    while (running_) {
        UrlQueueItem item("", 0);
        
        if (!url_queue_->dequeue(item)) {
            // Queue is stopped or empty
            break;
        }
        
        if (processUrl(item)) {
            pages_crawled_++;
        }
        
        // Small delay to avoid overwhelming servers
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Spider::processUrl(const UrlQueueItem& item) {
    std::cout << "Processing URL (depth " << item.depth << "): " << item.url << std::endl;
    
    // Check if already processed
    if (url_queue_->isProcessed(item.url)) {
        return false;
    }
    
    // Check depth limit
    if (item.depth > max_depth_) {
        url_queue_->markProcessed(item.url);
        return false;
    }
    
    // Check if URL should be crawled
    if (!shouldCrawlUrl(item.url)) {
        url_queue_->markProcessed(item.url);
        return false;
    }
    
    // Fetch the page
    HttpResponse response = http_client_->get(item.url);
    
    if (!response.success) {
        std::cerr << "Failed to fetch " << item.url << ": " << response.error_message << std::endl;
        url_queue_->markProcessed(item.url);
        return false;
    }
    
    // Check content type
    if (response.content_type.find("text/html") == std::string::npos) {
        std::cout << "Skipping non-HTML content: " << item.url << std::endl;
        url_queue_->markProcessed(item.url);
        return false;
    }
    
    // Extract title and text content
    std::string title = html_parser_->extractTitle(response.body);
    std::string content = html_parser_->extractText(response.body);
    
    // Index the page
    if (indexPage(item.url, title, content)) {
        pages_indexed_++;
    }
    
    // Extract and queue new URLs if we haven't reached max depth
    if (item.depth < max_depth_) {
        extractAndQueueUrls(response.body, item.url, item.depth);
    }
    
    url_queue_->markProcessed(item.url);
    return true;
}

bool Spider::indexPage(const std::string& url, const std::string& title, 
                      const std::string& content) {
    
    // Insert document into database
    int document_id = database_->insertDocument(url, title, content);
    if (document_id <= 0) {
        std::cerr << "Failed to insert document: " << url << std::endl;
        return false;
    }
    
    // Index the content
    std::map<std::string, int> word_frequencies = text_indexer_->indexText(content);
    
    // Store word frequencies in database
    size_t words_count = 0;
    for (const auto& pair : word_frequencies) {
        const std::string& word = pair.first;
        int frequency = pair.second;
        
        int word_id = database_->getOrCreateWord(word);
        if (word_id > 0) {
            if (database_->insertWordFrequency(document_id, word_id, frequency)) {
                words_count += frequency;
            }
        }
    }
    
    total_words_indexed_ += words_count;
    
    std::cout << "Indexed page: " << url << " (" << word_frequencies.size() 
              << " unique words, " << words_count << " total words)" << std::endl;
    
    return true;
}

void Spider::extractAndQueueUrls(const std::string& html_content, 
                                const std::string& base_url, int current_depth) {
    
    std::vector<std::string> links = html_parser_->extractLinks(html_content, base_url);
    
    int queued_count = 0;
    for (const std::string& link : links) {
        if (shouldCrawlUrl(link)) {
            if (url_queue_->enqueue(link, current_depth + 1)) {
                queued_count++;
            }
        }
    }
    
    if (queued_count > 0) {
        std::cout << "Queued " << queued_count << " new URLs from " << base_url << std::endl;
    }
}

bool Spider::shouldCrawlUrl(const std::string& url) const {
    // Basic URL filtering
    
    // Must be HTTP or HTTPS
    if (url.find("http://") != 0 && url.find("https://") != 0) {
        return false;
    }
    
    // Skip common non-content files
    std::vector<std::string> skip_extensions = {
        ".css", ".js", ".jpg", ".jpeg", ".png", ".gif", ".pdf", 
        ".zip", ".rar", ".exe", ".dmg", ".mp3", ".mp4", ".avi"
    };
    
    for (const std::string& ext : skip_extensions) {
        if (url.find(ext) != std::string::npos) {
            return false;
        }
    }
    
    // Skip URLs that are too long
    if (url.length() > 500) {
        return false;
    }
    
    return true;
}