cmake_minimum_required(VERSION 3.14)
include(FetchContent)
project(alex)

set(CMAKE_CXX_STANDARD 17)

# Install boost
add_subdirectory(boost-cmake)

# Define the macro ‘DEBUG' in the debug mode
if(CMAKE_BUILD_TYPE STREQUAL Debug)        
    ADD_DEFINITIONS(-DDEBUG)               
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "/O2 /arch:AVX2 /W1 /EHsc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "-O3 -xHost")
else()
    # clang and gcc
    set(CMAKE_CXX_FLAGS "-O3 -march=native -Wall -Wextra")
endif()

include_directories(src/core)

add_executable(example src/examples/main.cpp)
target_link_libraries(example PUBLIC Boost::serialization)
target_link_libraries(example PUBLIC Boost::iostreams)

add_executable(boost_demo src/examples/boost_demo.cpp)
target_link_libraries(boost_demo PUBLIC Boost::serialization)
target_link_libraries(boost_demo PUBLIC Boost::iostreams)

add_executable(alex_serde_demo src/examples/alex_serde_demo.cpp)
target_link_libraries(alex_serde_demo PUBLIC Boost::serialization)
target_link_libraries(alex_serde_demo PUBLIC Boost::iostreams)

add_executable(benchmark src/benchmark/main.cpp)
target_link_libraries(benchmark PUBLIC Boost::serialization)
target_link_libraries(benchmark PUBLIC Boost::iostreams)

add_executable(kv_build src/benchmark/kv_build.cpp)
target_link_libraries(kv_build PUBLIC Boost::serialization)
target_link_libraries(kv_build PUBLIC Boost::iostreams)

add_executable(kv_benchmark src/benchmark/kv_benchmark.cpp)
target_link_libraries(kv_benchmark PUBLIC Boost::serialization)
target_link_libraries(kv_benchmark PUBLIC Boost::iostreams)

set(DOCTEST_DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctest)
file(DOWNLOAD
    https://raw.githubusercontent.com/onqtam/doctest/2.4.6/doctest/doctest.h
    ${DOCTEST_DOWNLOAD_DIR}/doctest.h
    EXPECTED_HASH SHA1=40728d2bed7f074e80cb095844820114e7d16ce0
)

add_executable(test_alex test/unittest_main.cpp)
target_include_directories(test_alex PRIVATE ${DOCTEST_DOWNLOAD_DIR})
target_link_libraries(test_alex PUBLIC Boost::serialization)
target_link_libraries(test_alex PUBLIC Boost::iostreams)

enable_testing()
add_test(test_alex test_alex)