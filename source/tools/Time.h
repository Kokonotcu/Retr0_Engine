#pragma once
#include <chrono>
#include <iostream>
#include <vector>

namespace Time
{
	struct TimeTracker
	{
		TimeTracker() = delete;
		TimeTracker(double period);
		void CalculateTime();
		double GetPeriod();
		double GetTimePassed();
		bool Check();
		bool CheckOnce();
	private:
		double timePassed = 0.0f;
		double period = 0.0f;
	};

	std::shared_ptr<TimeTracker> RequestTracker(float period);
	void CalculateDeltaTime();
	double GetDeltatime();
	double GetTime();
	double FPS();
};
