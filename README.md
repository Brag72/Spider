# Search Engine Project

A complete search engine implementation in C++ consisting of a web spider (crawler) and a search server, similar to Google, Yandex, or Yahoo.

## Overview

The search engine consists of two main components:

1. **Spider (Crawler)**: An HTTP client that crawls websites, parses HTML content, and builds search indexes based on word frequencies in documents.
2. **Search Server**: An HTTP server that accepts search queries and returns ranked results based on document relevance.

## Architecture

### Components

- **Spider Program** (`spider`): Multi-threaded web crawler
- **Search Server** (`search_server`): HTTP server for search queries
- **Common Libraries**: Shared functionality for configuration, database, HTML parsing, and text indexing

### Database Schema

The system uses PostgreSQL with the following tables:

```sql
-- Documents table
CREATE TABLE documents (
    id SERIAL PRIMARY KEY,
    url VARCHAR(2048) UNIQUE NOT NULL,
    title TEXT,
    content TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Words table
CREATE TABLE words (
    id SERIAL PRIMARY KEY,
    word VARCHAR(100) UNIQUE NOT NULL
);

-- Word frequencies table (many-to-many relationship)
CREATE TABLE word_frequencies (
    document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE,
    word_id INTEGER REFERENCES words(id) ON DELETE CASCADE,
    frequency INTEGER NOT NULL DEFAULT 1,
    PRIMARY KEY (document_id, word_id)
);
```

## Dependencies

### Required Libraries

- **Boost Libraries**:
  - Boost.Beast (HTTP client/server)
  - Boost.System
  - Boost.Filesystem
  - Boost.Locale (text processing)
  - Boost.Thread
- **libpqxx**: PostgreSQL C++ client library
- **PostgreSQL**: Database server
- **CMake**: Build system

### Installation on Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libboost-all-dev
sudo apt-get install libpqxx-dev postgresql-dev
sudo apt-get install postgresql postgresql-contrib
```

### Installation on Windows

1. Install vcpkg package manager
2. Install dependencies:
```cmd
vcpkg install boost[beast,system,filesystem,locale,thread]
vcpkg install libpqxx
```

## Building

1. Clone or extract the project
2. Create build directory:
```bash
mkdir build
cd build
```

3. Configure and build:
```bash
cmake ..
make
```

## Configuration

Edit `config/config.ini` to configure the system:

```ini
# Database configuration
db_host=localhost
db_port=5432
db_name=search_engine
db_user=postgres
db_password=password

# Spider configuration
start_url=https://example.com
crawl_depth=2

# Search server configuration
server_port=8080
```

### Configuration Parameters

- `db_host`: PostgreSQL server hostname
- `db_port`: PostgreSQL server port (default: 5432)
- `db_name`: Database name
- `db_user`: Database username
- `db_password`: Database password
- `start_url`: Starting URL for spider crawling
- `crawl_depth`: Maximum crawling depth (1 = start page only)
- `server_port`: HTTP server port for search interface

## Database Setup

1. Install and start PostgreSQL
2. Create database and user:
```sql
CREATE DATABASE search_engine;
CREATE USER search_user WITH PASSWORD 'password';
GRANT ALL PRIVILEGES ON DATABASE search_engine TO search_user;
```

3. Tables will be created automatically when spider runs for the first time.

## Usage

### Running the Spider

The spider crawls websites and builds the search index:

```bash
./spider [config_file]
```

Example:
```bash
./spider config/config.ini
```

The spider will:
- Start from the configured URL
- Follow links up to the specified depth
- Extract text content and index words
- Store document information and word frequencies in the database
- Use multiple threads for efficient crawling

### Running the Search Server

The search server provides a web interface for searching:

```bash
./search_server [config_file]
```

Example:
```bash
./search_server config/config.ini
```

Then open your browser and go to:
```
http://localhost:8080
```

### Search Interface

The web interface provides:
- **GET /**: Search form page
- **POST /**: Search results page

Search features:
- Up to 4 words per query
- Case-insensitive search
- Results ranked by word frequency relevance
- Maximum 10 results per page

## Features

### Spider Features

- **Multi-threaded crawling**: Configurable number of worker threads
- **Depth-limited crawling**: Configurable maximum depth
- **URL deduplication**: Prevents processing the same URL multiple times
- **HTML parsing**: Extracts text content and links from HTML pages
- **Text indexing**: Analyzes word frequencies in documents
- **Robust HTTP client**: Handles both HTTP and HTTPS URLs
- **Content filtering**: Skips non-HTML content and unwanted file types

### Search Features

- **Full-text search**: Searches across all indexed documents
- **Relevance ranking**: Ranks results by total word frequency
- **Multi-word queries**: Supports queries with multiple words
- **Word validation**: Filters out short words and non-alphabetic content
- **Clean web interface**: Simple, user-friendly search form and results

### Text Processing

- **HTML tag removal**: Cleans HTML markup from content
- **Punctuation removal**: Removes punctuation while preserving word boundaries
- **Case normalization**: Converts text to lowercase for consistent indexing
- **Word length filtering**: Only indexes words between 3-32 characters
- **Locale support**: Uses Boost.Locale for proper text processing

## Technical Details

### Spider Algorithm

1. Initialize with start URL in queue
2. Worker threads dequeue URLs
3. For each URL:
   - Check if already processed
   - Fetch HTTP content
   - Parse HTML and extract text
   - Index words and store frequencies
   - Extract links and add to queue (if within depth limit)
   - Mark URL as processed

### Search Algorithm

1. Parse search query into words
2. Normalize and validate words
3. Execute SQL query to find documents containing ALL words
4. Rank by sum of word frequencies across all query words
5. Return top 10 results ordered by relevance

### Performance Considerations

- **Indexes**: Database indexes on words and word frequencies for fast search
- **Connection pooling**: Reuses database connections
- **Memory management**: Efficient string handling and memory allocation
- **Thread safety**: All shared data structures are thread-safe

## Error Handling

The system includes comprehensive error handling:
- Database connection failures
- HTTP request failures
- Invalid URLs
- Missing configuration
- Malformed HTML content

## Limitations

Current limitations (can be extended):
- No JavaScript execution (static HTML only)
- Basic URL normalization
- No robots.txt respect
- No rate limiting per domain
- Simple relevance scoring (word frequency only)
- No support for stemming or synonyms

## Future Enhancements

Possible improvements:
- PageRank-style link analysis
- Stemming and lemmatization
- Synonym support
- Advanced relevance scoring
- Faceted search
- Search suggestions
- Admin interface
- Distributed crawling
- Real-time indexing

## License

This project is provided as-is for educational purposes.

## Troubleshooting

### Common Issues

1. **Database connection failed**:
   - Check PostgreSQL is running
   - Verify credentials in config file
   - Ensure database exists

2. **Boost libraries not found**:
   - Install development packages
   - Set CMAKE_PREFIX_PATH if needed

3. **libpqxx not found**:
   - Install libpqxx development package
   - Check pkg-config setup

4. **Port already in use**:
   - Change server_port in config
   - Kill existing processes using the port

### Debug Mode

For debugging, you can:
- Add verbose logging to source code
- Use database queries to inspect indexed content
- Monitor HTTP traffic with tools like Wireshark
- Use debugger with debug build (-g flag)

## Contact

For questions or issues, please refer to the source code documentation and comments.