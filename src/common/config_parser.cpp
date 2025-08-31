#include "config_parser.h"
#include <algorithm>

ConfigParser::ConfigParser() {
}

ConfigParser::~ConfigParser() {
}

bool ConfigParser::loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        parseLine(line);
    }
    
    file.close();
    return true;
}

std::string ConfigParser::getDatabaseHost() const {
    return getValue("db_host");
}

int ConfigParser::getDatabasePort() const {
    try {
        return std::stoi(getValue("db_port"));
    } catch (const std::exception&) {
        return 5432; // Default PostgreSQL port
    }
}

std::string ConfigParser::getDatabaseName() const {
    return getValue("db_name");
}

std::string ConfigParser::getDatabaseUser() const {
    return getValue("db_user");
}

std::string ConfigParser::getDatabasePassword() const {
    return getValue("db_password");
}

std::string ConfigParser::getStartUrl() const {
    return getValue("start_url");
}

int ConfigParser::getCrawlDepth() const {
    try {
        return std::stoi(getValue("crawl_depth"));
    } catch (const std::exception&) {
        return 2; // Default crawl depth
    }
}

int ConfigParser::getServerPort() const {
    try {
        return std::stoi(getValue("server_port"));
    } catch (const std::exception&) {
        return 8080; // Default server port
    }
}

std::string ConfigParser::getValue(const std::string& key) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return "";
}

void ConfigParser::trim(std::string& str) {
    // Remove leading whitespace
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    
    // Remove trailing whitespace
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}

void ConfigParser::parseLine(const std::string& line) {
    size_t equalPos = line.find('=');
    if (equalPos == std::string::npos) {
        return; // Invalid line format
    }
    
    std::string key = line.substr(0, equalPos);
    std::string value = line.substr(equalPos + 1);
    
    trim(key);
    trim(value);
    
    config_[key] = value;
}