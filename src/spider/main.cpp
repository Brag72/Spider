#include <iostream>
#include <signal.h>
#include "../common/config_parser.h"
#include "spider.h"

// Global spider instance for signal handling
Spider* g_spider = nullptr;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", stopping spider..." << std::endl;
    if (g_spider) {
        g_spider->stopCrawling();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    std::cout << "Search Engine Spider v1.0" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    std::string config_file = "config/config.ini";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    std::cout << "Using config file: " << config_file << std::endl;
    
    // Load configuration
    ConfigParser config;
    if (!config.loadConfig(config_file)) {
        std::cerr << "Failed to load configuration file: " << config_file << std::endl;
        return 1;
    }
    
    // Create and initialize spider
    Spider spider;
    g_spider = &spider;
    
    if (!spider.initialize(config)) {
        std::cerr << "Failed to initialize spider" << std::endl;
        return 1;
    }
    
    // Start crawling
    std::cout << "\nStarting web crawling..." << std::endl;
    spider.startCrawling();
    
    std::cout << "Spider finished successfully." << std::endl;
    return 0;
}