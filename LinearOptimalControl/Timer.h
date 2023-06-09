#pragma once

#ifdef TIMING
#define TIME_FUNCTION() {}
#define TIME_SCOPE(name) {}
#define TIMER_START(name) {}
#define TIMER_STOP() {}
#else
#include <chrono>
#include <iostream>

#define TIME_FUNCTION() Timer timer(__func__);
#define TIME_SCOPE(name) Timer timer(name);
#define TIMER_START(name) ManualTimer::start(name);
#define TIMER_STOP() ManualTimer::stop();

struct Timer
{
	const char* name;
	const std::chrono::steady_clock::time_point start;

	Timer(const char* name)
		: name(name), start(std::chrono::high_resolution_clock::now())
	{}

	~Timer()
	{
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
		//std::cout << "\n[Timer] " << name << ": " << duration.count() << " " << (char)(230) << "s\n";
		std::cout << ((double)duration.count()) / 1000 << ((name == "CPLEX") ? "\n" : ", ");
	}
};

namespace ManualTimer
{
	Timer* active = nullptr;

	void stop()
	{
		delete active;
		active = nullptr;
	};

	void start(const char* name)
	{
		if (active)
			stop();

		active = new Timer(name);
	};
};

#endif // _DEBUG