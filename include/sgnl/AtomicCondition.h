// Author: Thomas Trapp - https://thomastrapp.com/
// License: MIT

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>


namespace sgnl {


template<typename ValueType>
class AtomicCondition
{
public:
  explicit AtomicCondition(ValueType val)
  : value_(val)
  , mutex_()
  , condvar_()
  {
  }

  ValueType get() const noexcept
  {
    return this->value_.load();
  }

  void set(ValueType val) noexcept
  {
    {
      // This ensures that wait_for is either
      // 1. not running, or
      // 2. in a waiting state
      // to avoid a data race between value_.load and cond_var.wait_for.
      std::unique_lock<std::mutex> lock(this->mutex_);
      this->value_.store(val);
    }

    this->condvar_.notify_all();
  }

  template<class Rep, class Period>
  void wait_for(const std::chrono::duration<Rep, Period>& time,
                ValueType val) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);

    while( this->value_.load() != val )
      if( this->condvar_.wait_for(lock, time) == std::cv_status::timeout )
        return;
  }

  template<class Rep, class Period, class Predicate>
  void wait_for(const std::chrono::duration<Rep, Period>& time,
                Predicate pred) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);

    while( !pred() )
      if( this->condvar_.wait_for(lock, time) == std::cv_status::timeout )
        return;
  }

private:
  std::atomic<ValueType> value_;
  mutable std::mutex mutex_;
  mutable std::condition_variable condvar_;
};


} // namespace sgnl

