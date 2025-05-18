#pragma once
#include "./queue/queue.h"
#include <thread>
#include <mutex>
#include <iostream>
#include <functional>
#include <vector>
#include <future>
#include <type_traits>
#include <queue>

class ThreadGuard
{
  std::vector<std::thread> &threads;

public:
  explicit ThreadGuard(std::vector<std::thread> &threads_) : threads(threads_)
  {
  }
  ~ThreadGuard();
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

class ThreadSafeDequeue
{
private:
  mutable std::mutex mtx;
  typedef FunctionWrapper dataType;
  std::deque<dataType> dataQueue;

public:
  ThreadSafeDequeue() {};
  ThreadSafeDequeue(const ThreadSafeDequeue &other) = delete;
  ThreadSafeDequeue &operator=(const ThreadSafeDequeue &other) = delete;
  void push(dataType data);
  bool empty() const;
  bool tryPop(dataType &value);
  bool trySteal(dataType &value);
};

class ThreadPool
{
  std::atomic_bool done;
  ThreadSafeQueue<FunctionWrapper> queue;
  typedef std::queue<FunctionWrapper> localQueue;
  static thread_local std::unique_ptr<localQueue>
      localWorkQueue;
  std::vector<std::thread>
      threads;
  ThreadGuard guard;
  void workerThread();

public:
  ThreadPool();
  ~ThreadPool();
  template <typename FunctionType>
  std::future<typename std::invoke_result<FunctionType>::type>
  submit(FunctionType f)
  {
    typedef typename std::invoke_result<FunctionType>::type
        result_type;
    std::packaged_task<result_type()> task(std::move(f));
    std::future<result_type> res(task.get_future());
    if (localWorkQueue)
    {
      localWorkQueue->push(std::move(task));
    }
    else
    {
      queue.push(std::move(task));
    }
    return res;
  }
  void runPendingTask();
};
