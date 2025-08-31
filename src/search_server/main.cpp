#include <iostream>
#include <signal.h>
#include "../common/config_parser.h"
#include "http_server.h"

// Global server instance for signal handling
HttpServer* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", stopping server..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    std::cout << "Search Engine Server v1.0" << std::endl;
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
    
    // Create and initialize server
    HttpServer server;
    g_server = &server;
    
    if (!server.initialize(config)) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }
    
    // Start server
    std::cout << "\nStarting search server..." << std::endl;
    server.start();
    
    std::cout << "Server finished." << std::endl;
    return 0;
}