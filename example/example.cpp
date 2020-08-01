#include <sgnl/AtomicCondition.h>
#include <sgnl/SignalHandler.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>


void Worker(const sgnl::AtomicCondition<bool>& exit_condition)
{
  while( !exit_condition.get() )
  {
    /* ... do work ... */

    exit_condition.wait_for_value(true, std::chrono::minutes(1));
  }
}

int main()
{
  sgnl::AtomicCondition<bool> exit_condition(false);

  auto my_handler = [&exit_condition](int signum) {
    std::cout << "received signal " << signum << "\n";
    if( signum == SIGTERM || signum == SIGINT )
    {
      // set atomic<bool> and wakeup all waiting threads
      exit_condition.set_and_notify_all(true);

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
    // Spawn a thread that handles the
    // signals {SIGINT, SIGTERM, SIGUSR1}
    signal_handler.async_sigwait_handler(my_handler);

  std::vector<std::future<void>> futures;
  for(int i = 0; i < 10; ++i)
    futures.push_back(
        std::async(
          std::launch::async,
          Worker,
          std::ref(exit_condition)));

  // SIGUSR1
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  kill(getpid(), SIGUSR1);

  // SIGTERM
  kill(getpid(), SIGTERM);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for(auto& future : futures)
    future.wait();

  int last_signal = ft_sig_handler.get();
  std::cout << "exiting (received signal " << last_signal << ")\n";

  return EXIT_SUCCESS;
}

