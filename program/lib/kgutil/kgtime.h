//時間・日付に関する関数
#ifndef __TIME_H__
#define __TIME_H__
#include <stdio.h>
#ifdef _MSC_VER
#	include <Windows.h>
#	include <sys/timeb.h>
#	include <time.h>
#else
#	include <sys/time.h>
#endif
#include "kgstdint.h"
namespace nsKg{
inline bool IsStdc(void)
{
#	ifdef WIN32
	return 0;
#	else
	return __STDC__;
#	endif 
}

//前回読んだ時間からの変化分を返す。精度1ms程度。
uint64_t GetElapsedMSec(void);

//fpsを返す。
//多数で呼ぶとおかしくなるのでその場合にはGetMilliSecCountで自己管理すること。
float GetFps(void);

//ミリ秒のカウントを返す。
//linuxはマイクロ秒のカウントを返す能力がある。
//windows もパフォーマンスカウンタを使うとマイクロ秒で返せるらしい。
//使用時には2回読んだ差を見る。秒数にするためには1000倍する。
uint64_t GetMilliSecCount(void);

//時間のカウントを取得
uint32_t GetTimeCount(void);

}

#endif // __TIME_H__
