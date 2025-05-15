#include "queue.h"

template <typename T>
void ThreadSafeQueue<T>::push(T node)
{
  std::lock_guard<std::mutex> lock(mtx);
  dataQueue.push(std::move(node));
  dataCond.notify_one();
}

template <typename T>
void ThreadSafeQueue<T>::waitAndPop(T &value)
{
  std::unique_lock<std::mutex> lock(mtx);
  dataCond.wait(lock, [this]
                { return !dataQueue.empty(); });
  value = std::move(dataQueue.front());
  dataQueue.pop();
}

template <typename T>
bool ThreadSafeQueue<T>::tryPop(T &value)
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

template <typename T>
bool ThreadSafeQueue<T>::empty() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return dataQueue.empty();
}

template <typename T>
size_t ThreadSafeQueue<T>::size() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return dataQueue.size();
}
