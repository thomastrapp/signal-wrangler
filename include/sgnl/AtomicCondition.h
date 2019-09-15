// Author: Thomas Trapp - https://thomastrapp.com/
// License: MIT

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>


namespace sgnl {


template<typename ValueType>
class AtomicCondition
{
public:
  explicit AtomicCondition(ValueType val)
  : value_(val)
  , condvar_mutex_()
  , condvar_()
  {
    // requirement of std::signal
    if( !this->value_.is_lock_free() )
      throw std::runtime_error("atomic<ValueType> is not lock-free");
  }

  ValueType get() const
  {
    return this->value_.load();
  }

  void set(ValueType val)
  {
    this->value_.store(val);
  }

  void wait_for(const std::chrono::milliseconds& time, ValueType val) const
  {
    std::unique_lock<std::mutex> lock(this->condvar_mutex_);

    // This while-loop takes care of "spurious wakeups"
    while( this->value_.load() != val )
      if( this->condvar_.wait_for(lock, time) == std::cv_status::timeout )
        return;
  }

  void notify_all() noexcept
  {
    this->condvar_.notify_all();
  }

private:
  std::atomic<ValueType> value_;
  mutable std::mutex condvar_mutex_;
  mutable std::condition_variable condvar_;
};


} // namespace sgnl

