# event_thread

### Async multithreading framework

## Run demo
```console
cmake .
make
./test
```
expected result:
```text
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)
first thread received: "hello from second thread" (tid:5208)
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)
second thread received: "hello from first thread" (tid:5209)
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)
second thread received: "hello from first thread" (tid:5209)
first thread received: "hello from second thread" (tid:5208)

...
```
