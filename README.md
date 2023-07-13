# EThread - an Asynchronous Multithreading Framework

# Features

## Thread & Worker Architecture

## Timer

## Thread-safe Event Based Inter-thread Communication


## High Level Asynchronous Processing
 EThread supports `EPromise` class that resemble promises from Javascript.
 `EPromise::then()` is used to chain multiple function to be executed sequentially from different objects across different threads.
 `EPromise::cat()` is used to capture any exceptions that are thrown during the execution of the chain.
For more information see [the promise example](test/promise.cpp).