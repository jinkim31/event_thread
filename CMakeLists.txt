cmake_minimum_required(VERSION 3.5)
project(event_thread LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(event_thread STATIC
        event_thread/ethread.cpp
        event_thread/etimer.cpp
        event_thread/epromise.cpp
        event_thread/eutil.cpp
        )


find_package(Threads REQUIRED)
target_link_libraries(event_thread PRIVATE Threads::Threads)
target_include_directories(event_thread PUBLIC ${PROJECT_SOURCE_DIR}/event_thread)

add_executable(test_itc test/itc.cpp)
target_link_libraries(test_itc PRIVATE event_thread)

add_executable(test_timer test/timer.cpp)
target_link_libraries(test_timer PRIVATE event_thread)

add_executable(test_promise test/promise.cpp)
target_link_libraries(test_promise PRIVATE event_thread)