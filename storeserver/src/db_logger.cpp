#include "db_logger.h"

#include <ctime>
#include <iostream>
#include <thread>

static std::mutex cout_mtx;

void db_log(std::string msg) {
  cout_mtx.lock();
  std::cerr << std::time(nullptr) << " " << std::this_thread::get_id() << " "
            << msg;
  cout_mtx.unlock();
}