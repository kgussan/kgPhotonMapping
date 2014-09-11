//詳しくは同名ヘッダを参照
//注意：WIN32はwindows で定義されるマクロ。なお、64bit windows でも使うことができる。

#include "kgTime.h"
#include <stdio.h>
namespace nsKg{

uint64_t GetElapsedMSec(void)
{
	static int firsttime=1;
	static uint64_t started_time=0;
	uint64_t current_time;//in msec
	uint64_t elapsed;
#ifdef WIN32
	static time_t ltime;
	static struct _timeb tstruct;
	if (firsttime==1) {
		time(&ltime);
		_ftime_s(&tstruct);
		started_time=ltime*1000+tstruct.millitm;
		firsttime=0;
	}
	time(&ltime);
	_ftime_s(&tstruct);
	current_time=ltime*1000+tstruct.millitm;
#else //linux
	struct timeval now;
	if (firsttime) {
		gettimeofday(&now, NULL);
		started_time = now.tv_sec*1000+now.tv_usec/1000;
		firsttime = 0;
	}
	gettimeofday(&now, NULL);
	current_time=now.tv_sec*1000+now.tv_usec/1000;
#endif
	elapsed=current_time-started_time;
	started_time=current_time;
	return elapsed;
}

uint64_t GetMilliSecCount(void)
{
	double current_time;//in msec
#ifdef WIN32
	//time_t ltime;
	//time(&ltime);
	//current_time=ltime*1000+tstruct.millitm/1000;
	struct _timeb ts;
	_ftime_s(&ts);
	current_time=(double)(ts.time*1000+ts.millitm);
#else //linux
	struct timeval now;
	gettimeofday(&now, NULL);
	current_time=now.tv_sec*1000+now.tv_usec/1000;
#endif
	return (uint64_t)current_time;
}


uint64_t GetElapsedMSecForFps(void)
{
	static int firsttime=1;
	static uint64_t started_time=0;
	uint64_t current_time;//in msec
	uint64_t elapsed;
#ifdef WIN32
	static time_t ltime;
	static struct _timeb tstruct;
	if (firsttime==1) {
		time(&ltime);
		_ftime_s(&tstruct);
		started_time=ltime*1000+tstruct.millitm;
		firsttime=0;
	}
	time(&ltime);
	_ftime_s(&tstruct);
	current_time=ltime*1000+tstruct.millitm;
#else //linux
	struct timeval now;
	if (firsttime) {
		gettimeofday(&now, NULL);
		started_time = now.tv_sec*1000+now.tv_usec/1000;
		firsttime = 0;
	}
	gettimeofday(&now, NULL);
	current_time=now.tv_sec*1000+now.tv_usec/1000;
#endif
	elapsed=current_time-started_time;
	started_time=current_time;
	return elapsed;
}

float GetFps(void)
{
	const int NUM=5;
	static int firstTime=1;
	static int index=0;
	static uint64_t data[NUM];
	float ave;

	if (firstTime==1) {
		for (int i=0;i<NUM;i++) {
			data[i]=0;
		}
		firstTime=0;
	}
	data[index]=GetElapsedMSecForFps();
	ave=0.0f;
	for (int i=0;i<NUM;i++) {
		ave+=1000.0f/(float)data[i];
	}
	ave/=(float)NUM;
	index++;
	index%=NUM;
	return ave;
}

//time
//世界協定時刻 (UTC: Coordinated Universal Time) の 1970 年 1 月 1 日の 00:00:00 から経過した時間
uint32_t GetTimeCount(void)
{
#ifdef WIN32
	struct _timeb ts;
	_ftime_s(&ts);
	//current_time=ts.time*1000+ts.millitm;
	return (uint32_t)ts.time;
#else //linux
	struct timeval now;
	gettimeofday(&now, NULL);
	//current_time=now.tv_sec*1000+now.tv_usec/1000;
	//return now.
	return 0;
#endif
}


}//nsKg
