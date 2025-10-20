#include "Time.h"

namespace Time
{
	namespace
	{
		std::vector<std::shared_ptr<TimeTracker>> trackers;
		double deltaTime = 0.0;
		auto last = std::chrono::high_resolution_clock::now(); //std::chrono::steady_clock::now(); 
		double average[150] = { 0.0 }; int i = 0;
	}

	std::shared_ptr<TimeTracker> RequestTracker(float period)
	{
		auto tracker = std::make_shared<TimeTracker>(period);
		trackers.emplace_back(tracker);
		return tracker;
	}

	void CalculateDeltaTime()
	{
		auto old = last;
		last = std::chrono::high_resolution_clock::now(); //std::chrono::steady_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::duration<double>> (last - old).count() ;

		for (auto &t : trackers)
			t->CalculateTime();

		average[i] = (deltaTime);
		i = (i + 1) % 150;
	}
	double GetDeltatime()
	{
		return deltaTime;
	}
	double GetTime()
	{
		return std::chrono::duration_cast<std::chrono::duration<double>> (std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	double FPS()
	{
		double sum = 0.0;
		for (int x = 0; x < 150; x++)
		{
			sum += average[x];
		}
		return 1.0 / (sum / 150.0);
	}


	TimeTracker::TimeTracker(double _limit)
		:
		period(_limit)
	{
		timePassed = 0.0f;
	}
	void TimeTracker::CalculateTime()
	{
		timePassed += deltaTime;
	}
	double TimeTracker::GetPeriod()
	{
		return period;
	}
	double TimeTracker::GetTimePassed()
	{
		return timePassed;
	}
	bool TimeTracker::Check()
	{
		if (timePassed >= period)
		{
			timePassed -= period;
			return true;
		}
		return false;
	}
	bool TimeTracker::CheckOnce()
	{
		if (timePassed >= period)
		{
			timePassed = 0.0;
			return true;
		}
		return false;
	}
}
