#ifndef _TIMER_H_
#define _TIMER_H_

#include <iostream>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/timeb.h>

using namespace std;
namespace POWEROPT {
	class Timer {
	public:
		//constructors
		Timer() {}
		//modifiers
		//Record the current time in milliseconds
		void start()
		{
			getrusage(RUSAGE_SELF, &usage);
			startTime = (double) (usage.ru_utime.tv_sec*1000000.0+usage.ru_utime.tv_usec
			                               +usage.ru_stime.tv_sec*1000000.0+usage.ru_stime.tv_usec);
//			 cout<<"usage.ru_utime.tv_usec:"<<usage.ru_utime.tv_usec<<endl;
//            cout<<fixed<<(double)10/3<<endl;
//			cout<<fixed<<"startTime"<<startTime<<endl;
		}
		//accessors
		double getElapsedTime()
		{
			getrusage(RUSAGE_SELF, &usage);
			double endTime = (double) (usage.ru_utime.tv_sec*1000000.0+usage.ru_utime.tv_usec
			                                        +usage.ru_stime.tv_sec*1000000.0+usage.ru_stime.tv_usec);
//			 cout<<"usage.ru_utime.tv_usec:"<<usage.ru_utime.tv_usec<<endl;
//			cout<<fixed<<"endTime"<<endTime<<endl;
			return (endTime-startTime)/(double) 1000000.0;
		}
	private:
		double startTime;
		struct rusage usage;
	};

}

#endif //_TIMER_H_
