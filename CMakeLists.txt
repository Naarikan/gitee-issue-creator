cmake_minimum_required(VERSION 3.10)
project(gitee_issue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/ConfigSetup.cpp
    src/Hasher.cpp
    src/IssueCreator.cpp
)

include_directories(
    src
    external
)

add_executable(gitee-issue ${SOURCES})

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)

if (NOT SQLITE3_INCLUDE_DIR OR NOT SQLITE3_LIBRARY)
    message(FATAL_ERROR "SQLite3 not found")
endif()

include_directories(${SQLITE3_INCLUDE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
        message(STATUS "Using GCC < 9.1: Linking stdc++fs for std::filesystem support")
        target_link_libraries(gitee-issue stdc++fs)
    endif()
endif()

target_link_libraries(gitee-issue
    CURL::libcurl
    OpenSSL::Crypto
    ${SQLITE3_LIBRARY}
)


install(TARGETS gitee-issue
        RUNTIME DESTINATION bin)

# Install bash completion script
install(FILES completion/gitee-issue
        DESTINATION /etc/bash_completion.d
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

