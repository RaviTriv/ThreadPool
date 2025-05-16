#include "./queue/queue.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <functional>
#include <vector>
#include <future>
#include <type_traits>

class ThreadGuard
{
  std::vector<std::thread> &threads;

public:
  explicit ThreadGuard(std::vector<std::thread> &threads_) : threads(threads_)
  {
  }
  ~ThreadGuard()
  {
    for (size_t i = 0; i < threads.size(); i++)
    {
      if (threads[i].joinable())
      {
        threads[i].join();
      }
    }
  };
};

class FunctionWrapper
{
  struct implBase
  {
    virtual void call() = 0;
    virtual ~implBase() {}
  };
  std::unique_ptr<implBase> impl;
  template <typename F>
  struct implType : implBase
  {
    F f;
    implType(F &&f_) : f(std::move(f_)) {}
    void call() { f(); }
  };

public:
  template <typename F>
  FunctionWrapper(F &&f) : impl(new implType<F>(std::move(f)))
  {
  }
  void operator()() { impl->call(); }
  FunctionWrapper() = default;
  FunctionWrapper(FunctionWrapper &&other) : impl(std::move(other.impl))
  {
  }
  FunctionWrapper &operator=(FunctionWrapper &&other)
  {
    impl = std::move(other.impl);
    return *this;
  }
  FunctionWrapper(const FunctionWrapper &) = delete;
  FunctionWrapper(FunctionWrapper &) = delete;
  FunctionWrapper &operator=(const FunctionWrapper &) = delete;
};

class ThreadPool
{
  std::atomic_bool done;
  ThreadSafeQueue<FunctionWrapper> queue;
  std::vector<std::thread> threads;
  ThreadGuard guard;
  void workerThread()
  {
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
  std::future<typename std::invoke_result<FunctionType>::type>
  submit(FunctionType f)
  {
    typedef typename std::invoke_result<FunctionType>::type
        result_type;
    std::packaged_task<result_type()> task(std::move(f));
    std::future<result_type> res(task.get_future());
    queue.push(std::move(task));
    return res;
  }
};
