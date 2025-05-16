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
  void push(T node)
  {
    std::lock_guard<std::mutex> lock(mtx);
    dataQueue.push(std::move(node));
    dataCond.notify_one();
  }
  void waitAndPop(T &value)
  {
    std::unique_lock<std::mutex> lock(mtx);
    dataCond.wait(lock, [this]
                  { return !dataQueue.empty(); });
    value = std::move(dataQueue.front());
    dataQueue.pop();
  }
  bool tryPop(T &value)
  {
    std::lock_guard<std::mutex> lock(mtx);
    if (dataQueue.empty())
    {
      return false;
    }
    value = std::move(dataQueue.front());
    dataQueue.pop();
    return true;
  }
  bool empty() const
  {
    std::lock_guard<std::mutex> lock(mtx);
    return dataQueue.empty();
  }
  size_t size() const
  {
    std::lock_guard<std::mutex> lock(mtx);
    return dataQueue.size();
  }
};
