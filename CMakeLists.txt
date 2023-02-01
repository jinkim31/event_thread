cmake_minimum_required(VERSION 3.5)
project(event_thread LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(event_thread STATIC
    event_thread/eventThread.cpp
    event_thread/sharedResource.cpp
)


find_package(Threads REQUIRED)
target_link_libraries(event_thread PRIVATE Threads::Threads)
target_include_directories(event_thread PUBLIC ${PROJECT_SOURCE_DIR}/event_thread)

add_executable(test
    testcode/main.cpp
        testcode/MainThread.cpp testcode/MainThread.h testcode/BaseThread.cpp testcode/BaseThread.h testcode/DerivedThread.cpp testcode/DerivedThread.h)

target_link_libraries(test PRIVATE event_thread)
