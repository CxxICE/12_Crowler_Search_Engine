cmake_minimum_required(VERSION 3.20)

project(CppFinalProject)

set(BOOST_ROOT "C:\\boost_1_82_0")
set(Boost_USE_STATIC_LIBS ON)

# If you are running on Windows below 10, change this:
add_compile_definitions(_WIN32_WINNT=0x0A01 _SILENCE_CXX20_CISO646_REMOVED_WARNING)

# Boost:
find_package(Boost 1.82.0 REQUIRED COMPONENTS system thread locale coroutine context url)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

# PQXX
set(PostgreSQL_ADDITIONAL_VERSIONS "15")

#include directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/_sources")
set(PQXX_DIR "${CMAKE_SOURCE_DIR}/pqxx_headers")
set(SSL_DIR "${CMAKE_SOURCE_DIR}/openssl_headers")

#link directories
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
set(PQXX_LINK_DIR "${CMAKE_SOURCE_DIR}/pqxx_libs/Static_Debug")
set(SSL_LINK_DIR "${CMAKE_SOURCE_DIR}/openssl_libs/MDd")
set(PSQL_LINK_DIR "${CMAKE_SOURCE_DIR}/psql_libs")
else()
set(PQXX_LINK_DIR "${CMAKE_SOURCE_DIR}/pqxx_libs/Static_Release")
set(SSL_LINK_DIR "${CMAKE_SOURCE_DIR}/openssl_libs/MD")
set(PSQL_LINK_DIR "${CMAKE_SOURCE_DIR}/psql_libs")
endif()

file(GLOB DLL_COPY "${CMAKE_SOURCE_DIR}/psql_libs/*.dll")

add_subdirectory(_crowler_project)
add_subdirectory(_search_engine_project)
