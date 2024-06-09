#include "timer.hpp"
#include <iostream>

EasyTimer::EasyTimer()
    : start(std::chrono::high_resolution_clock::now()), message(""),
      total_time(nullptr) {}

EasyTimer::EasyTimer(std::string message)
    : start(std::chrono::high_resolution_clock::now()), message(message),
      total_time(nullptr) {}

EasyTimer::EasyTimer(float &total_time)
    : start(std::chrono::high_resolution_clock::now()), message(""),
      total_time(&total_time) {}

EasyTimer::EasyTimer(std::string message, float &total_time)
    : start(std::chrono::high_resolution_clock::now()), message(message),
      total_time(&total_time) {}

EasyTimer::~EasyTimer() {
  end = std::chrono::high_resolution_clock::now();
  duration = end - start;
  float ms = duration.count() * 1000.0f;
  if (total_time != nullptr) {
    *total_time += ms;
  }
  if (message == "") {
    return;
  }
  std::cout << message << " Duration: " << ms << " ms" << std::endl;
}

Timer::Timer() {}

void Timer::start() { start_time = std::chrono::high_resolution_clock::now(); }

void Timer::stop() {
  end_time = std::chrono::high_resolution_clock::now();
  duration = end_time - start_time;
}

float Timer::getDuration() {
  float ms = duration.count() * 1000.0f;
  return ms;
}