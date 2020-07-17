// Author: Thomas Trapp - https://thomastrapp.com/
// License: MIT

#pragma once

#include <csignal>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>


namespace sgnl {


class SignalHandlerException : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


/// When constructed, SignalHandler blocks the given `signals` in the calling
/// thread.
///
/// Signals can be polled by calling `SignalHandler::sigwait()` or
/// `SignalHandler::sigwait_handler(handler)`.
/// `handler` is a callable that accepts a signal number as its first and only
/// argument. `handler` returns false if the waiting should be continued.
///
/// When destructed, SignalHandler unblocks the `signals`.
class SignalHandler
{
public:
  explicit SignalHandler(const std::initializer_list<int>& signals)
  : set_()
  {
    if( sigemptyset(&this->set_) != 0 )
      throw SignalHandlerException("sigemptyset error");

    for( int signum : signals )
      if( sigaddset(&this->set_, signum) != 0 )
        throw SignalHandlerException("sigaddset error");

    int s = pthread_sigmask(SIG_BLOCK, &this->set_, nullptr);
    if( s != 0 )
      throw SignalHandlerException(
          std::string("pthread_sigmask: ") + std::strerror(s));
  }

  ~SignalHandler() noexcept
  {
    pthread_sigmask(SIG_UNBLOCK, &this->set_, nullptr);
  }

  SignalHandler(const SignalHandler& other) = delete;
  SignalHandler(SignalHandler&& other) = delete;
  SignalHandler& operator=(const SignalHandler& other) = delete;
  SignalHandler& operator=(SignalHandler&& other) = delete;

  int sigwait() const
  {
    int signum = 0;
    int ret = ::sigwait(&this->set_, &signum);
    if( ret != 0 )
      throw SignalHandlerException(
          std::string("sigwait: ") + std::strerror(ret));
    return signum;
  }

  int sigwait_handler(std::function<bool (int)> handler) const
  {
    while( true )
    {
      int signum = this->sigwait();
      if( handler(signum) )
        return signum;
    }
  }

private:
  sigset_t set_;
};


} // namespace sgnl

