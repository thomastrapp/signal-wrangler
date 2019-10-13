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

