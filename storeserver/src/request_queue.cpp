#include "request_queue.h"

#include <chrono>

void RequestQueue::enqueue(const int &key, const Request::Request &value) {
  std::lock_guard<std::mutex> lock(mutex);
  map.emplace(key, value);
  cv.notify_one();
}

bool RequestQueue::deque(Request::Request &value, int &seq) {
  std::unique_lock<std::mutex> lock(mutex);
  if (cv.wait_for(lock, std::chrono::seconds(40),
                  [this] { return map.count(next) == 1; })) {
    seq = next;
    auto itr = map.find(next);
    value = std::move(itr->second);
    map.erase(itr);
    next++;
    return true;
  } else {
    return false;
  }
}

void RequestQueue::clear() {
  next = 1;
  map = std::map<int, Request::Request>();
}