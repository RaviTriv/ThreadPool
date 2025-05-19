#include "./threadPool.h"

thread_local ThreadSafeDequeue *ThreadPool::localWorkQueue;
thread_local unsigned ThreadPool::idx;

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
  value = std::move(dataQueue.front());
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

void ThreadPool::workerThread(unsigned index)
{
  idx = index;
  localWorkQueue = queues[idx].get();
  while (!done)
  {
    ThreadPool::runPendingTask();
  }
};

bool ThreadPool::localQueuePopTask(taskType &task)
{
  return localWorkQueue && localWorkQueue->tryPop(task);
}

bool ThreadPool::poolQueuePopTask(taskType &task)
{
  return queue.tryPop(task);
}

bool ThreadPool::otherThreadQueuePopTask(taskType &task)
{
  for (unsigned i = 0; i < queues.size(); ++i)
  {
    unsigned const index = (idx + i + 1) % queues.size();
    if (queues[index]->trySteal(task))
    {
      return true;
    }
  }
  return false;
}

ThreadPool::ThreadPool() : done(false), guard(threads)
{
  unsigned const threadCount = std::thread::hardware_concurrency();
  try
  {
    for (unsigned i = 0; i < threadCount; ++i)
    {
      queues.push_back(std::unique_ptr<ThreadSafeDequeue>(
          new ThreadSafeDequeue));
    }
    for (unsigned i = 0; i < threadCount; ++i)
    {
      threads.push_back(std::thread(&ThreadPool::workerThread, this, i));
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
  taskType task;
  if (
      localQueuePopTask(task) ||
      poolQueuePopTask(task) ||
      otherThreadQueuePopTask(task))
  {
    task();
  }
  else
  {
    std::this_thread::yield();
  }
}
