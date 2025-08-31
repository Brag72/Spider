#pragma once

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();
    
    bool loadConfig(const std::string& filename);
    
    // Database configuration
    std::string getDatabaseHost() const;
    int getDatabasePort() const;
    std::string getDatabaseName() const;
    std::string getDatabaseUser() const;
    std::string getDatabasePassword() const;
    
    // Spider configuration
    std::string getStartUrl() const;
    int getCrawlDepth() const;
    
    // Search server configuration
    int getServerPort() const;
    
    // Generic getter
    std::string getValue(const std::string& key) const;
    
private:
    std::map<std::string, std::string> config_;
    
    void trim(std::string& str);
    void parseLine(const std::string& line);
};