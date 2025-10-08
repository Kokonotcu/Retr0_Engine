#pragma once
#include <chrono>
#include <iostream>
#include <vector>

namespace Time
{
	struct TimeTracker
	{
		TimeTracker() = delete;
		TimeTracker(float period);
		void CalculateTime();
		float GetPeriod();
		float GetTimePassed();
		bool Check();
		bool CheckOnce();
	private:
		float timePassed = 0.0f;
		float period = 0.0f;
	};

	std::shared_ptr<TimeTracker> RequestTracker(float period);
	void CalculateDeltaTime();
	const float GetDeltatime();
	const double GetTime();
	float FPS();
};
