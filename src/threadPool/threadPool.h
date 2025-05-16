#include "./queue/queue.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <functional>
#include <vector>

class ThreadGuard
{
  std::vector<std::thread> &threads;

public:
  explicit ThreadGuard(std::vector<std::thread> &threads_) : threads(threads_)
  {
  }
  ~ThreadGuard()
  {
    for (int i = 0; i < threads.size(); i++)
    {
      if (threads[i].joinable())
      {
        threads[i].join();
      }
    }
  };
};

class ThreadPool
{
  std::atomic_bool done;
  ThreadSafeQueue<std::function<void()>> queue;
  std::vector<std::thread> threads;
  ThreadGuard guard;
  void workerThread()
  {
    while (!done)
    {
      std::function<void()> task;
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

public:
  ThreadPool() : done(false), guard(threads)
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
  ~ThreadPool()
  {
    done = true;
  }
  template <typename FunctionType>
  void submit(FunctionType f)
  {
    queue.push(std::function<void()>(f));
  }
};
