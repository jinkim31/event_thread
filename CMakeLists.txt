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

add_executable(test_inter_thread_communication
        test/inter_thread_communication/main.cpp
        test/inter_thread_communication/app.cpp
        test/inter_thread_communication/worker.cpp)
target_link_libraries(test_inter_thread_communication PRIVATE event_thread)

add_executable(test_call_queued_move
        test/call_queued_move/main.cpp
        test/call_queued_move/app.cpp
        test/call_queued_move/worker.cpp)
target_link_libraries(test_call_queued_move PRIVATE event_thread)

add_executable(test_promise test/promise/main.cpp)
target_link_libraries(test_promise PRIVATE event_thread)

add_executable(test_etimer test/etimer/main.cpp)
target_link_libraries(test_etimer PRIVATE event_thread)
