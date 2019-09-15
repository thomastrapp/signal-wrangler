#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE

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

int looping_worker(sgnl::AtomicCondition<bool>& exit_condition)
{
  int i = 0;
  while( exit_condition.get() == false )
  {
    exit_condition.wait_for(std::chrono::milliseconds(2), true);
    ++i;
  }

  return i;
}


} // namespace


TEST_CASE("condition-get-set")
{
  sgnl::AtomicCondition<int> condition(23);
  REQUIRE( condition.get() == 23 );
  condition.set(42);
  REQUIRE( condition.get() == 42 );
}

TEST_CASE("constructor-thread-blocks-signals")
{
  sgnl::AtomicCondition<bool> condition(false);

  sgnl::SignalHandler signal_handler({{SIGTERM, true}, {SIGINT, true}}, condition);

  std::promise<pthread_t> signal_handler_thread_id;
  std::future<int> ft_sig_handler =
    std::async(std::launch::async, [&]() {
      signal_handler_thread_id.set_value(pthread_self());
      return signal_handler();
    });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  // send terminate signal to main thread ..
  REQUIRE( pthread_kill(pthread_self(), SIGTERM) == 0 );

  // .. but we're still running
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  // send interrupt signal to signal handler thread
  REQUIRE(
      pthread_kill(
        signal_handler_thread_id.get_future().get(),
        SIGINT) == 0 );

  CHECK( ft_sig_handler.get() == SIGINT );
  CHECK( condition.get() == true );

  CHECK( signal_handler() == SIGTERM );
}

TEST_CASE("exit-condition")
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
    for(int i = 0; i < 50; ++i)
      futures.push_back(
          std::async(
            std::launch::async,
            &worker,
            std::ref(exit_condition)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

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

TEST_CASE("exit-condition-looping")
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

    std::vector<std::future<int>> futures;
    for(int i = 0; i < 10; ++i)
      futures.push_back(
          std::async(
            std::launch::async,
            &looping_worker,
            std::ref(exit_condition)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    REQUIRE(
        pthread_kill(
          signal_handler_thread_id.get_future().get(),
          test_signal) == 0 );

    for(auto& future : futures)
      // After 100 milliseconds, each worker thread should
      // have looped at least 10 times.
      // This might break if system is under heavy load
      // or really slow.
      REQUIRE(future.get() > 10);

    int signal = ft_sig_handler.get();
    REQUIRE( signal == test_signal );
  }
}

