#pragma once
#include <queue>
#include <mutex>

template <typename T>
class ThreadSafeQueue
{
private:
  mutable std::mutex mtx;
  std::queue<T> dataQueue;
  std::condition_variable dataCond;

public:
  ThreadSafeQueue() {};
  void push(T node);
  void waitAndPop(T &value);
  bool tryPop(T &value);
  bool empty() const;
  size_t size() const;
};
