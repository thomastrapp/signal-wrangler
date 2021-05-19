Signal handler for multi threaded C++ applications on Linux
===========================================================

Signal handler that uses [pthread_sigmask](http://man7.org/linux/man-pages/man3/pthread_sigmask.3.html) and [sigwait](http://man7.org/linux/man-pages/man3/sigwait.3.html).


## Dependencies

* C++17
* Clang or GCC
* linux
* pthread
* cmake (recommended, but optional)
* Catch2 for testing


## Example usage

```C++
{
  // Block signals
  sgnl::SignalHandler signal_handler({SIGINT, SIGTERM});

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

Using a condition variable to shutdown all threads:

```C++
#include <sgnl/AtomicCondition.h>
#include <sgnl/SignalHandler.h>

#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>


void Worker(const sgnl::AtomicCondition<bool>& exit_condition)
{
  auto predicate = [&exit_condition]() {
    return exit_condition.get();
  };
  while( true )
  {
    exit_condition.wait_for(std::chrono::minutes(1), predicate);
    if( exit_condition.get() )
      return;
    /* ... do work ... */
  }
}

int main()
{
  sgnl::AtomicCondition<bool> exit_condition(false);

  auto handler = [&exit_condition](int signum) {
    std::cout << "received signal " << signum << "\n";
    if( signum == SIGTERM || signum == SIGINT )
    {
      exit_condition.set(true);
      // wakeup all waiting threads
      exit_condition.notify_all();
      // stop polling for signals
      return true;
    }

    // continue waiting for signals
    return false;
  };

  // Block signals in this thread.
  // Threads spawned later will inherit the signal mask.
  sgnl::SignalHandler signal_handler({SIGINT, SIGTERM, SIGUSR1});

  std::future<int> ft_sig_handler =
    std::async(
        std::launch::async,
        &sgnl::SignalHandler::sigwait_handler,
        &signal_handler,
        std::ref(handler));

  std::vector<std::future<void>> futures;
  for(int i = 0; i < 10; ++i)
    futures.push_back(
        std::async(
          std::launch::async,
          Worker,
          std::ref(exit_condition)));

  // SIGUSR1
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  kill(0, SIGUSR1);

  // SIGTERM
  kill(0, SIGTERM);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for(auto& future : futures)
    future.wait();

  int last_signal = ft_sig_handler.get();
  std::cout << "exiting (received signal " << last_signal << ")\n";

  return EXIT_SUCCESS;
}
```


## Build & Install

```SH
mkdir -p build/ && cd build/
cmake ..
# build and run tests
make sgnl-test && ./test/sgnl-test
# build and run example
make example && ./example
# install headers and CMake config
make install
```

## Using signal-wrangler with CMake

The easiest way to add signal-wrangler to a CMake project is by using
[FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```CMAKE
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(my-example)

include(FetchContent)
FetchContent_Declare(
  signal-wrangler
  GIT_REPOSITORY https://github.com/thomastrapp/signal-wrangler
  GIT_TAG v0.4.0)
FetchContent_MakeAvailable(signal-wrangler)

add_executable(my-example "example/example.cpp")
target_link_libraries(my-example sgnl::sgnl)
```

Or, by installing signal-wrangler (`make install`) and using `find_package`:

```CMAKE
find_package(Sgnl REQUIRED)
target_link_libraries(my-project sgnl::sgnl ...)
```
