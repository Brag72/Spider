#include "url_queue.h"
#include <algorithm>

UrlQueue::UrlQueue() : stopped_(false) {
}

UrlQueue::~UrlQueue() {
    stop();
}

bool UrlQueue::enqueue(const std::string& url, int depth) {
    std::string normalized_url = normalizeUrl(url);
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if URL is already processed or queued
    if (processed_urls_.count(normalized_url) > 0 || 
        queued_urls_.count(normalized_url) > 0) {
        return false;
    }
    
    // Add to queue and mark as queued
    queue_.emplace(normalized_url, depth);
    queued_urls_.insert(normalized_url);
    
    condition_.notify_one();
    return true;
}

bool UrlQueue::dequeue(UrlQueueItem& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until queue has items or is stopped
    condition_.wait(lock, [this] { return !queue_.empty() || stopped_; });
    
    if (stopped_ && queue_.empty()) {
        return false;
    }
    
    if (!queue_.empty()) {
        item = queue_.front();
        queue_.pop();
        
        // Remove from queued set (it will be added to processed when marked)
        queued_urls_.erase(item.url);
        
        return true;
    }
    
    return false;
}

bool UrlQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t UrlQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void UrlQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    condition_.notify_all();
}

bool UrlQueue::isProcessed(const std::string& url) const {
    std::string normalized_url = normalizeUrl(url);
    std::lock_guard<std::mutex> lock(mutex_);
    return processed_urls_.count(normalized_url) > 0;
}

void UrlQueue::markProcessed(const std::string& url) {
    std::string normalized_url = normalizeUrl(url);
    std::lock_guard<std::mutex> lock(mutex_);
    processed_urls_.insert(normalized_url);
}

size_t UrlQueue::getProcessedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return processed_urls_.size();
}

size_t UrlQueue::getPendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::string UrlQueue::normalizeUrl(const std::string& url) const {
    std::string normalized = url;
    
    // Remove trailing slash
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Convert to lowercase (for the scheme and host parts)
    // In a real implementation, you'd want more sophisticated URL normalization
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}