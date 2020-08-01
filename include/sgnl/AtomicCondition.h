// Author: Thomas Trapp - https://thomastrapp.com/
// License: MIT

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <utility>


namespace sgnl {


/// Paires a `std::condition_variable` with a `std::atomic<ValueType>`,
/// protected by a mutex.
/// The interface of `std::condition_variable` is preserved through "perfect
/// forwarding".
template<typename ValueType>
class AtomicCondition
{
public:
  explicit AtomicCondition(ValueType initial_value)
  : value_(initial_value)
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
    // This lock is required to avoid a data race between calls to
    // `AtomicCondition::get` when called in `AtomicCondition::wait_*` through
    // a predicate.
    std::unique_lock<std::mutex> lock(this->mutex_);
    this->value_.store(val);
  }

  void set_and_notify_one(ValueType val) noexcept
  {
    this->set(std::move(val));
    this->condvar_.notify_one();
  }

  void set_and_notify_all(ValueType val) noexcept
  {
    this->set(std::move(val));
    this->condvar_.notify_all();
  }

  auto native_handle() { return this->condvar_.native_handle(); }
  void notify_one() const noexcept { this->condvar_.notify_one(); }
  void notify_all() const noexcept { this->condvar_.notify_all(); }

  template<typename... Args>
  auto wait(Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait(lock, std::forward<Args>(args)...);
  }

  template<typename... Args>
  auto wait_value(ValueType value, Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait(
        lock,
        std::forward<Args>(args)...,
        [this, &value](){
          return this->value_.load() == value;
        });
  }

  template<typename... Args>
  auto wait_for(Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait_for(lock, std::forward<Args>(args)...);
  }

  template<typename... Args>
  auto wait_for_value(ValueType value, Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait_for(
        lock,
        std::forward<Args>(args)...,
        [this, &value](){
          return this->value_.load() == value;
        });
  }

  template<typename... Args>
  auto wait_until(Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait_until(lock, std::forward<Args>(args)...);
  }

  template<typename... Args>
  auto wait_until_value(ValueType value, Args&&... args) const
  {
    std::unique_lock<std::mutex> lock(this->mutex_);
    return this->condvar_.wait_until(
        lock,
        std::forward<Args>(args)...,
        [this, &value](){
          return this->value_.load() == value;
        });
  }

private:
  std::atomic<ValueType> value_;
  mutable std::mutex mutex_;
  mutable std::condition_variable condvar_;
};


template<typename ValueType>
AtomicCondition(ValueType) -> AtomicCondition<ValueType>;


} // namespace sgnl

