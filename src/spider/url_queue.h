#pragma once

#include <string>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct UrlQueueItem {
    std::string url;
    int depth;
    
    UrlQueueItem(const std::string& u, int d) : url(u), depth(d) {}
};

class UrlQueue {
public:
    UrlQueue();
    ~UrlQueue();
    
    // Add URL to queue if not already processed
    bool enqueue(const std::string& url, int depth);
    
    // Get next URL from queue (blocks if empty)
    bool dequeue(UrlQueueItem& item);
    
    // Check if queue is empty
    bool empty() const;
    
    // Get queue size
    size_t size() const;
    
    // Stop the queue (unblocks waiting threads)
    void stop();
    
    // Check if URL has been processed
    bool isProcessed(const std::string& url) const;
    
    // Mark URL as processed
    void markProcessed(const std::string& url);
    
    // Get statistics
    size_t getProcessedCount() const;
    size_t getPendingCount() const;
    
private:
    std::queue<UrlQueueItem> queue_;
    std::unordered_set<std::string> processed_urls_;
    std::unordered_set<std::string> queued_urls_;
    
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopped_;
    
    std::string normalizeUrl(const std::string& url) const;
};