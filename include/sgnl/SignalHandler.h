// Author: Thomas Trapp - https://thomastrapp.com/
// License: MIT

#pragma once

#include <sgnl/AtomicCondition.h>

#include <csignal>
#include <cstring>
#include <map>
#include <stdexcept>
#include <utility>


namespace sgnl {


class SignalHandlerException : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};


template<typename ValueType>
class SignalHandler
{
public:
  SignalHandler(std::map<int, ValueType> signal_map,
                AtomicCondition<ValueType>& condition)
  : signal_map_(std::move(signal_map))
  , set_()
  , condition_(condition)
  {
    if( sigemptyset(&this->set_) != 0 )
      throw SignalHandlerException("sigemptyset error");

    for( const auto& p : this->signal_map_ )
      if( sigaddset(&this->set_, p.first) != 0 )
        throw SignalHandlerException("sigaddset error");

    int s = pthread_sigmask(SIG_BLOCK, &this->set_, nullptr);
    if( s != 0 )
      throw SignalHandlerException(
          std::string("pthread_sigmask: ") + std::strerror(s));
  }

  int operator()()
  {
    while( true )
    {
      int signum = 0;
      int ret = sigwait(&this->set_, &signum);
      if( ret != 0 )
        throw SignalHandlerException(
            std::string("sigwait: ") + std::strerror(ret));

      if( auto it = this->signal_map_.find(signum);
          it != this->signal_map_.end() )
      {
        this->condition_.set(it->second);
        return it->first;
      }
    }
  }

private:
  std::map<int, ValueType> signal_map_;
  sigset_t set_;
  AtomicCondition<ValueType>& condition_;
};


} // namespace sgnl

