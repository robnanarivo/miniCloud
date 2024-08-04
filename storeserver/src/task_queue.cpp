#include "task_queue.h"

#include <chrono>

void TaskQueue::enqueue(const int &value) {
  std::lock_guard<std::mutex> lock(mutex);
  queue.push(value);
  cv.notify_one();
}

bool TaskQueue::deque(int &value) {
  std::unique_lock<std::mutex> lock(mutex);
  if (cv.wait_for(lock, std::chrono::seconds(40),
                  [this] { return !queue.empty(); })) {
    value = queue.front();
    queue.pop();
    return true;
  } else {
    return false;
  }
}

void TaskQueue::clear() { queue = std::queue<int>(); }