#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

class TaskQueue {
 private:
  std::queue<int> queue;
  std::mutex mutex;
  std::condition_variable cv;

 public:
  void enqueue(const int &value);
  bool deque(int &value);
  void clear();
};