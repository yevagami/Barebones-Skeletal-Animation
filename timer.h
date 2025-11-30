#pragma once
#include <chrono>
#include <queue>

struct TimerLogEntry{
public:
	const char* prefix;
	float result;
};

class TIMER {
private:
	static int count;
	static const char* prefix;
	static std::chrono::time_point<std::chrono::steady_clock> startTime;
	static std::chrono::time_point<std::chrono::steady_clock> endTime;

public:
	static const int MAX_ENTRIES = 4;
	static TimerLogEntry logs[MAX_ENTRIES];

	static void StartTiming(const char* p) {
		prefix = p;
		startTime = std::chrono::high_resolution_clock().now();
	}

	static void StopTiming() {
		endTime = std::chrono::high_resolution_clock().now();
		int64_t start = std::chrono::time_point_cast<std::chrono::microseconds>(startTime).time_since_epoch().count();
		int64_t end = std::chrono::time_point_cast<std::chrono::microseconds>(endTime).time_since_epoch().count();
		float result = (float)(end - start) / 1000.0f;

		TimerLogEntry newEntry = {prefix, result};
		AddEntry(newEntry);
	}

	static void AddEntry(TimerLogEntry newEntry) {
		if (count >= MAX_ENTRIES) {
			count = 1;
		}
		logs[count - 1] = newEntry;
		count++;
	}

};
const char* TIMER::prefix;
int TIMER::count = 1;
TimerLogEntry TIMER::logs[MAX_ENTRIES];
std::chrono::time_point<std::chrono::steady_clock> TIMER::startTime;
std::chrono::time_point<std::chrono::steady_clock> TIMER::endTime;