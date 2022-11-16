cmake_minimum_required(VERSION 3.5)

project(multithread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

add_library(event_thread STATIC
    event_thread/eventThread.cpp
    event_thread/sharedResource.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(event_thread PRIVATE Threads::Threads)

add_executable(test
    testcode/main.cpp
    testcode/firstThread.cpp
    testcode/secondThread.cpp
)

target_link_libraries(test PRIVATE event_thread)
