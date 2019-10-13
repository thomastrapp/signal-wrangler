Signal handler for multithreaded C++ applications on Linux
==========================================================

Signal handler that uses [pthread_sigmask](http://man7.org/linux/man-pages/man3/pthread_sigmask.3.html) and [sigwait](http://man7.org/linux/man-pages/man3/sigwait.3.html).


## Dependencies

* C++17
* Clang or GCC
* linux
* pthread
* cmake (recommended, but optional)


## Example usage

```
{
  // Block signals
  sgnl::SignalHandler signal_handler({SIGINT, SIGINT});

  // Wait for a signal
  int signal_number = signal_handler.sigwait();

  // Or, pass a handler
  auto handler = [](int signum) {
    if( signum == SIGINT )
      // continue waiting for signals
      return false;
    if( signum == SIGTERM )
      // stop waiting for signals
      return true;
  };

  int last_signal = signal_handler.sigwait_handler(handler);
} // signals are unblocked again
```
See [example.cpp](example/example.cpp) for an example using threads.


## Build & Install

```
mkdir -p build/ && cd build/
cmake ..
# build and run tests
make sgnl-test && ./test/sgnl-test
# build and run example
make example && ./example
# install headers and CMake config
make install
```

