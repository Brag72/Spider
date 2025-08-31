cmake_minimum_required(VERSION 3.30)
project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PQXX_INCLUDE_DIRS "C:/local/libpqxx-7.10.1/include")
set(PQXX_LIBRARIES "C:/local/vcpkg-master/packages/libpqxx_x64-windows/lib/pqxx.lib")
# Find required packages

set(BOOST_INCLUDE_LIBRARIES system filesystem locale thread)
set(BOOST_ENABLE_CMAKE ON)

add_definitions(-DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS)

include(FetchContent)
FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost
    GIT_PROGRESS TRUE
    GIT_TAG boost-1.88.0
)
FetchContent_MakeAvailable(Boost)


# Find PostgreSQL
set(PostgreSQL_ADDITIONAL_VERSIONS "17")
find_package(PostgreSQL REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PQXX_INCLUDE_DIRS})
include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories(src/common)

# Source files for common library
set(COMMON_SOURCES
    src/common/config_parser.cpp
    src/common/database.cpp
    src/common/html_parser.cpp
    src/common/text_indexer.cpp
)

# Create common library
add_library(common STATIC ${COMMON_SOURCES})
target_link_libraries(common 
    ${Boost_LIBRARIES}
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

# Spider executable
add_executable(spider 
    src/spider/main.cpp
    src/spider/spider.cpp
    src/spider/http_client.cpp
    src/spider/url_queue.cpp
)

target_link_libraries(spider 
    common
    ${Boost_LIBRARIES}
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

# Search server executable  
add_executable(search_server
    src/search_server/main.cpp
    src/search_server/http_server.cpp
    src/search_server/search_engine.cpp
)

target_link_libraries(search_server
    common
    ${Boost_LIBRARIES}
    ${PQXX_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)

# Compiler flags
target_compile_options(common PRIVATE ${PQXX_CFLAGS_OTHER})
target_compile_options(spider PRIVATE ${PQXX_CFLAGS_OTHER})