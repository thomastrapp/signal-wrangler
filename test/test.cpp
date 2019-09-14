#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <sgnl/SignalHandler.h>

#include <pthread.h>

#include <chrono>
#include <future>
#include <vector>


namespace {


bool worker(sgnl::AtomicCondition<bool>& exit_condition)
{
  exit_condition.wait_for(std::chrono::milliseconds(1000), true);
  return exit_condition.get();
}


} // namespace


TEST_CASE("main")
{
  std::vector<int> signals({SIGINT, SIGTERM, SIGUSR1, SIGUSR2});
  for( auto test_signal : signals )
  {
    sgnl::AtomicCondition exit_condition(false);

    sgnl::SignalHandler signal_handler({{test_signal, true}}, exit_condition);
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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(
        pthread_kill(
          signal_handler_thread_id.get_future().get(),
          test_signal) == 0 );

    for(auto& future : futures)
      REQUIRE(future.get() == true);

    int signal = ft_sig_handler.get();
    REQUIRE( signal == test_signal );
  }
}

