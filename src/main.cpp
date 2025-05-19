#include "./threadPool/threadPool.h"
#include <iostream>
#include <future>
#include <type_traits>
#include <list>
#include <chrono>

bool filter(int x)
{
  // some long running calc
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  return x % 2 == 0;
}

int main()
{
  const int SIZE = 1000;
  const int UPPER_BOUND = 100;
  ThreadPool pool;
  std::vector<int> arr(SIZE);
  std::vector<std::future<bool>> futures(SIZE);

  for (int i = 0; i < SIZE; i++)
  {
    arr[i] = rand() % UPPER_BOUND;
    futures[i] = pool.submit([x = arr[i]]()
                             { return filter(x); });
  }

  for (int i = 0; i < SIZE; i++)
  {
    std::cout << futures[i].get() << std::endl;
  }
  return 0;
}