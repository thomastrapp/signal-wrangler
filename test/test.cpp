#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE

#include <catch2/catch.hpp>

#include <sgnl/AtomicCondition.h>
#include <sgnl/SignalHandler.h>

#include <signal.h>
#include <sys/types.h>

#include <chrono>
#include <future>
#include <vector>


namespace {


bool Worker(const sgnl::AtomicCondition<bool>& exit_condition)
{
  exit_condition.wait_for(
      std::chrono::hours(1000),
      [&exit_condition](){ return exit_condition.get() == true; });
  return exit_condition.get();
}

int LoopingWorker(const sgnl::AtomicCondition<bool>& exit_condition)
{
  int i = 0;
  while( exit_condition.get() == false )
  {
    exit_condition.wait_for(
        std::chrono::milliseconds(2),
        [&exit_condition](){ return exit_condition.get() == true; });
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
  condition.set_and_notify_all(1);
  REQUIRE( condition.get() == 1 );
  condition.set_and_notify_one(2);
  REQUIRE( condition.get() == 2 );
}

TEST_CASE("wait")
{
  sgnl::AtomicCondition condition(false);
  std::future<bool> future =
    std::async(
        std::launch::async,
        [&condition](){ condition.wait(); return true; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  condition.notify_one();
  REQUIRE( future.get() == true );
}

TEST_CASE("wait-pred")
{
  sgnl::AtomicCondition condition(false);
  std::future<bool> future =
    std::async(
        std::launch::async,
        [&condition](){ condition.wait([](){ return true; }); return true; });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  condition.notify_one();
  REQUIRE( future.get() == true );
}

TEST_CASE("wait-for-predicate")
{
  sgnl::AtomicCondition condition(23);
  auto pred = [&condition](){ return condition.get() == 42; };
  std::future<void> future =
    std::async(
        std::launch::async,
        [&condition, &pred](){
          condition.wait_for(std::chrono::hours(1000), pred); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  condition.set(42);
  condition.notify_all();
  future.wait();

  REQUIRE( condition.get() == 42 );
}

TEST_CASE("wait-for-predicate-simple")
{
  sgnl::AtomicCondition condition(23);
  auto pred = [&condition](){ return condition.get() == 42; };
  std::future<void> future =
    std::async(
        std::launch::async,
        [&condition, &pred](){
          condition.wait_for(std::chrono::hours(1000), pred); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  condition.set_and_notify_all(42);
  future.wait();

  REQUIRE( condition.get() == 42 );
}

TEST_CASE("wait-until-predicate")
{
  sgnl::AtomicCondition condition(23);
  auto pred = [&condition](){ return condition.get() == 42; };
  std::future<void> future =
    std::async(
        std::launch::async,
        [&condition, &pred](){
          condition.wait_until(
              std::chrono::system_clock::now() + std::chrono::hours(1),
              pred); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  condition.set(42);
  condition.notify_all();
  future.wait();

  REQUIRE( condition.get() == 42 );
}

TEST_CASE("sigwait")
{
  sgnl::SignalHandler signal_handler({SIGUSR1});
  kill(0, SIGUSR1);
  REQUIRE( signal_handler.sigwait() == SIGUSR1 );
}

TEST_CASE("sigwait_handler")
{
  auto handler = [](int){
    return true;
  };
  sgnl::SignalHandler signal_handler({SIGUSR2});
  kill(0, SIGUSR2);
  REQUIRE( signal_handler.sigwait_handler(handler) == SIGUSR2 );
}

TEST_CASE("constructor-thread-blocks-signals")
{
  std::atomic last_signal(0);
  sgnl::SignalHandler signal_handler({SIGTERM, SIGINT});

  auto handler = [&last_signal](int signum) {
    last_signal.store(signum);
    return signum == SIGINT;
  };

  std::future<int> ft_sig_handler =
    std::async(
        std::launch::async,
        &sgnl::SignalHandler::sigwait_handler,
        &signal_handler,
        std::ref(handler));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();

  REQUIRE( kill(0, SIGTERM) == 0 );

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::this_thread::yield();
  CHECK( last_signal.load() == SIGTERM );

  REQUIRE( kill(0, SIGINT) == 0 );

  REQUIRE( ft_sig_handler.get() == SIGINT );
  REQUIRE( last_signal.load() == SIGINT );
}

TEST_CASE("sleeping-workers-with-exit-condition")
{
  sgnl::AtomicCondition exit_condition(false);
  std::initializer_list signals({SIGINT, SIGTERM, SIGUSR1, SIGUSR2});
  for( auto test_signal : signals )
  {
    exit_condition.set(false);
    auto handler = [&exit_condition, test_signal](int signum) {
      exit_condition.set(true);
      exit_condition.notify_all();
      return test_signal == signum;
    };

    sgnl::SignalHandler signal_handler({test_signal});
    std::future<int> ft_sig_handler =
      std::async(
          std::launch::async,
          &sgnl::SignalHandler::sigwait_handler,
          &signal_handler,
          std::ref(handler));

    std::vector<std::future<bool>> futures;
    for(int i = 0; i < 50; ++i)
      futures.push_back(
          std::async(
            std::launch::async,
            Worker,
            std::ref(exit_condition)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    REQUIRE( kill(0, test_signal) == 0 );

    for(auto& future : futures)
      REQUIRE(future.get() == true);

    REQUIRE( ft_sig_handler.get() == test_signal );
  }
}

TEST_CASE("looping-workers-with-exit-condition")
{
  std::atomic last_signal(0);
  sgnl::AtomicCondition exit_condition(false);
  std::initializer_list signals({SIGINT, SIGTERM, SIGUSR1, SIGUSR2});
  for( auto test_signal : signals )
  {
    exit_condition.set(false);
    auto handler = [&exit_condition, test_signal](int signum) {
      exit_condition.set(true);
      exit_condition.notify_all();
      return test_signal == signum;
    };

    sgnl::SignalHandler signal_handler({test_signal});
    std::future<int> ft_sig_handler =
      std::async(
          std::launch::async,
          &sgnl::SignalHandler::sigwait_handler,
          &signal_handler,
          std::ref(handler));

    std::vector<std::future<int>> futures;
    for(int i = 0; i < 10; ++i)
      futures.push_back(
          std::async(
            std::launch::async,
            LoopingWorker,
            std::ref(exit_condition)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    REQUIRE( kill(0, test_signal) == 0 );

    for(auto& future : futures)
      // After 100 milliseconds, each worker thread should
      // have looped at least 10 times.
      REQUIRE( future.get() > 10 );

    REQUIRE( ft_sig_handler.get() == test_signal );
  }
}

