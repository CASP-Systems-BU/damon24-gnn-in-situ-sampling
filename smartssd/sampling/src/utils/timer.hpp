#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <string>

class EasyTimer {
public:
  EasyTimer();
  EasyTimer(std::string message);
  EasyTimer(float &total_time);
  EasyTimer(std::string message, float &total_time);
  ~EasyTimer();

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  std::chrono::duration<float> duration;
  std::string message;
  float *total_time;
};

class Timer {
public:
  Timer();
  void start();
  void stop();
  float getDuration();

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
  std::chrono::duration<float> duration;
};

#endif // TIMER_HPP