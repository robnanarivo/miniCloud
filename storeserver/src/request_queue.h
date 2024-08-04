#pragma once

#include <condition_variable>
#include <map>
#include <mutex>

#include "request.h"

class RequestQueue {
 private:
  int next = 1;
  std::map<int, Request::Request> map;
  std::mutex mutex;
  std::condition_variable cv;

 public:
  void enqueue(const int &key, const Request::Request &value);
  bool deque(Request::Request &value, int &seq);
  void clear();
};