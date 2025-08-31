# Simple Makefile for Search Engine project
# This is a basic alternative to CMake for testing

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Isrc/common
LIBS = -lboost_system -lboost_filesystem -lboost_locale -lboost_thread -lpqxx -lpq -lssl -lcrypto -lpthread

# Source files
COMMON_SOURCES = src/common/config_parser.cpp src/common/database.cpp src/common/html_parser.cpp src/common/text_indexer.cpp
SPIDER_SOURCES = src/spider/main.cpp src/spider/spider.cpp src/spider/http_client.cpp src/spider/url_queue.cpp
SEARCH_SERVER_SOURCES = src/search_server/main.cpp src/search_server/http_server.cpp src/search_server/search_engine.cpp

# Object files
COMMON_OBJECTS = $(COMMON_SOURCES:.cpp=.o)
SPIDER_OBJECTS = $(SPIDER_SOURCES:.cpp=.o)
SEARCH_SERVER_OBJECTS = $(SEARCH_SERVER_SOURCES:.cpp=.o)

# Targets
all: spider search_server

spider: $(COMMON_OBJECTS) $(SPIDER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

search_server: $(COMMON_OBJECTS) $(SEARCH_SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(COMMON_OBJECTS) $(SPIDER_OBJECTS) $(SEARCH_SERVER_OBJECTS) spider search_server

.PHONY: all clean

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build both spider and search_server"
	@echo "  spider        - Build spider executable"
	@echo "  search_server - Build search_server executable"
	@echo "  clean         - Remove all object files and executables"
	@echo "  help          - Show this help message"