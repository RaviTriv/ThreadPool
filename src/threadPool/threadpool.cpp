#include "./threadPool.h"

std::unique_ptr<ThreadPool::localQueue> thread_local ThreadPool::localWorkQueue{};

ThreadGuard::~ThreadGuard()
{
  for (size_t i = 0; i < threads.size(); i++)
  {
    if (threads[i].joinable())
    {
      threads[i].join();
    }
  }
}

void ThreadSafeDequeue::push(dataType data)
{
  std::lock_guard<std::mutex> lock(mtx);
  dataQueue.push_front(std::move(data));
}

bool ThreadSafeDequeue::empty() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return dataQueue.empty();
}

bool ThreadSafeDequeue::tryPop(dataType &value)
{
  std::lock_guard<std::mutex> lock(mtx);
  if (dataQueue.empty())
  {
    return false;
  }
  value = std::move(dataQueue.back());
  dataQueue.pop_front();
  return true;
}

bool ThreadSafeDequeue::trySteal(dataType &value)
{
  std::lock_guard<std::mutex> lock(mtx);
  if (dataQueue.empty())
  {
    return false;
  }
  value = std::move(dataQueue.back());
  dataQueue.pop_back();
  return true;
}

void ThreadPool::workerThread()
{
  localWorkQueue.reset(new localQueue);
  while (!done)
  {
    FunctionWrapper task;
    if (queue.tryPop(task))
    {
      task();
    }
    else
    {
      std::this_thread::yield();
    }
  }
};

ThreadPool::ThreadPool() : done(false), guard(threads)
{
  unsigned const thread_count = std::thread::hardware_concurrency();
  try
  {
    for (unsigned i = 0; i < thread_count; ++i)
    {
      threads.push_back(
          std::thread(&ThreadPool::workerThread, this));
    }
  }
  catch (...)
  {
    done = true;
    throw;
  }
}

ThreadPool::~ThreadPool()
{
  done = true;
}

void ThreadPool::runPendingTask()
{
  FunctionWrapper task;
  if (localWorkQueue && !localWorkQueue->empty())
  {
    task = std::move(localWorkQueue->front());
    localWorkQueue->pop();
    task();
  }
  else if (queue.tryPop(task))
  {
    task();
  }
  else
  {
    std::this_thread::yield();
  }
}
