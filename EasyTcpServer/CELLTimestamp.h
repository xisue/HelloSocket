#ifndef _CELLTIMESTAMP_H_
#define _CELLTIMESTAMP_H_


#include <chrono>
using namespace std::chrono;
class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		update();
	}
	~CELLTimestamp(){}

	void update()
	{
		_begin = high_resolution_clock::now();
	}

	//获取当前秒
	double getElapsedSecond()
	{
		return this->getElapsedTimeInMicroSec() * 0.000001;
	}

	//获取毫秒
	double getElapsedTimeInMillisec()
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}

	//获取微妙
	long long getElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock>_begin;
};

#endif 