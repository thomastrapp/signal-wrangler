Signal handler for multithreaded C++ applications on Linux
==========================================================


## Build & Install

```
mkdir -p build/ && cd build/
cmake ..
make
make test
make install
```


Example usage:
```
bool worker(sgnl::AtomicCondition<bool>& exit_condition)
{
  while( true )
  {
    exit_condition.wait_for(std::chrono::milliseconds(1000), true);
    if( exit_condition.get() )
      return true;
  }
}

sgnl::AtomicCondition exit_condition(false);

sgnl::SignalHandler signal_handler(
  {{SIGINT, true}, {SIGTERM, true}},
  exit_condition);

std::promise<pthread_t> signal_handler_thread_id;
std::future<int> ft_sig_handler =
  std::async(std::launch::async, [&]() {
    signal_handler_thread_id.set_value(pthread_self());
    return signal_handler();
  });

std::vector<std::future<bool>> futures;
for(int i = 0; i < 10; ++i)
  futures.push_back(
      std::async(
        std::launch::async,
        &worker,
        std::ref(exit_condition)));

// simulate [ctrl]+[c], which sends SIGINT
std::this_thread::sleep_for(std::chrono::milliseconds(100));
pthread_kill(
  signal_handler_thread_id.get_future().get(),
  SIGINT);

for(auto& future : futures)
  future.get();

int signal = ft_sig_handler.get();
```

