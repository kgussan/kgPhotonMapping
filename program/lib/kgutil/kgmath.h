//数学ヘッダ、数学関数/機能
#ifndef __HEADER_MATH_H__
#define __HEADER_MATH_H__
#include <stdlib.h>
#include <stdio.h>

#ifdef _MSC_VER
#include "kgstdint.h"
#else
#include <stdint.h>
#endif

#define _USE_MATH_DEFINES	//MSおまじない。
#include <math.h>
#include "kgmacro.h"

namespace nsKg{

//	定数群
namespace KineticCoeddficient
{
	// kinetic coefficient
	const float MS_GLASS = 0.94f;	//< static friction
	const float MK_GLASS = 0.4f;	//< kinetic(dynamic) friction
	const float MS_IRON = 1.1f;
	const float MK_IRON = 0.15f;
	const float MS_RUBBER_AND_LAND = 0.55f;
	const float MK_RUBBER_AND_LAND = 0.4f;
	const float MS_STEEL = 0.78f;
	const float MK_STEEL = 0.42f;
	const float MS_WOOD = 0.38f;
	const float MK_WOOD = 0.2f;
	const float MS_ICE = 0.1f;
	const float MK_ICE = 0.03f;
	const float MS_OILSTEEL = 0.10f;
	const float MK_OILSTEEL = 0.08f;
}

// variable
const double PI = 3.14159265358979323846e0;

///	数学関数
// MACRO
#define DEG2RAD(deg) ((deg)*M_PI/180.0f)
#define RAD2DEG(rad) ((rad)*180.0f/M_PI)
inline float Deg2Rad(float deg)
{
	return (deg*(float)M_PI/180.0f);
}
inline float Rad2Deg(float rad)
{
	return (rad*180.0f/(float)M_PI);
}

#if defined(__cplusplus)
extern "C" {
#endif
struct Vector3_t {
	float x, y, z;
};
struct Vector4_t {
	float x, y, z, w;
};

inline bool IsPositive( float val )
{
	if ( val>0 ) {
		return true;
	}else{
		return false;
	}
}
inline bool IsNegative(float val)
{
	if ( val>0 ) {
		return false;
	}else{
		return true;
	}
	//return ~IsPositive(val);
}

inline float GetSign(float val)
{
	if (IsPositive(val)) {
		return +1.0f;
	}else{
		return -1.0f;
	}
}

inline uint32_t CountLeadingZero(uint32_t x)
{
#ifdef __PPU__
	uint32_t result;
    __asm__ volatile ("cntlzw %0, %1" : "=r"(result) : "r"(x));
	return result;
#else
	uint32_t y;
	uint32_t n = 32;
	y=x>>16;if (y!=0) {n=n-16;x=y;}
	y=x>> 8;if (y!=0) {n=n-8;x=y;}
	y=x>> 4;if (y!=0) {n=n-4;x=y;}
	y=x>> 2;if (y!=0) {n=n-2;x=y;}
	y=x>> 1;if (y!=0) {return n-2;}
	return n - x;
#endif
}

inline float GetEuclidDist2D(float x1,float y1,float x2,float y2)
{
	return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

inline float GetEuclidDist3D(float x1,float y1,float z1,float x2,float y2,float z2)
{
	return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1));
}

//	ベクトル演算
//法線
inline int32_t CalculateNormal(const float p1[3],const float p2[3],const float p3[3],float n[3])
{
	float v1[3];
	float v2[3];
	float cross[3];
	float length;
	int i;
	//v1=p1-p2を求める
	for (i=0;i<3;i++) {
		v1[i]=p1[i]-p2[i];
	}
	//v2=p3-p2を求める
	for (i=0;i<3;i++) {
		v2[i]=p3[i]-p2[i];
	}
	//外積v2×v1（=cross）を求める
	for (i=0;i<3;i++) {
		cross[i]=v2[(i+1)%3]*v1[(i+2)%3]-v2[(i+2)%3]*v1[(i+1)%3];
	}
	//外積v2×v1の長さ|v2×v1|（=length）を求める
	length=sqrtf(cross[0]*cross[0]+cross[1]*cross[1]+cross[2]*cross[2]);
	//長さ|v2×v1|が0のときは法線ベクトルは求められない
	if (length==0.0f) {
		return 0;
	}
	// 外積v2×v1を長さ|v2×v1|で割って法線ベクトルnを求める (正規化)
	for (i=0;i<3;i++) {
		n[i]=cross[i]/length;
	}
	return 1;
}

inline int32_t ScaleVec3( const float s[3], const float bv[3], float v[3] )
{
	v[0] = bv[0]*s[0];
	v[1] = bv[1]*s[1];
	v[2] = bv[2]*s[2];
	return 0;
}

inline int32_t RotXVec4( const float g_theta, const float bv[4], float v[4] )
{
	v[0] = ( + 1*bv[0] +      0      *bv[1] +      0      *bv[2] + 0*bv[3] )*bv[0];
	v[1] = (   0*bv[0] + cos( g_theta )*bv[1] - sin( g_theta )*bv[2] + 0*bv[3] )*bv[1];
	v[2] = (   0*bv[0] + sin( g_theta )*bv[1] + cos( g_theta )*bv[2] + 0*bv[3] )*bv[2];
	v[3] = (   0*bv[0] +      0      *bv[1] +      0      *bv[2] + 1*bv[3] )*bv[2];
	return 0;
}

inline int32_t RotXVec3( const float g_theta, const float bv[3], float v[3] )
{
	v[0] = ( + 1*bv[0] +            0*bv[1] +            0*bv[2] )*bv[0];
	v[1] = (   0*bv[0] + cos( g_theta )*bv[1] - sin( g_theta )*bv[2] )*bv[1];
	v[2] = (   0*bv[0] + sin( g_theta )*bv[1] + cos( g_theta )*bv[2] )*bv[2];
	return 0;
}
inline int32_t RotYVec4( const float g_theta, const float bv[4], float v[4] )
{
	v[0] = ( + cos( g_theta )*bv[0] + 0*bv[1] + sin( g_theta )*bv[2] + 0*bv[3] )*bv[0];
	v[1] = (              0*bv[0] + 1*bv[1] +            0*bv[2] + 0*bv[3] )*bv[1];
	v[2] = ( - sin( g_theta )*bv[0] + 0*bv[1] + cos( g_theta )*bv[2] + 0*bv[3] )*bv[2];
	v[3] = (   0*bv[0] +      0      *bv[1] +      0      *bv[2] + 1*bv[3] )*bv[2];
	return 0;
}

inline int32_t RotYVec3( const float g_theta, const float bv[3], float v[3] )
{
	v[0] = ( + cos( g_theta )*bv[0] + 0*bv[1] + sin( g_theta )*bv[2] )*bv[0];
	v[1] = (              0*bv[0] + 1*bv[1] +            0*bv[2] )*bv[1];
	v[2] = ( - sin( g_theta )*bv[0] + 0*bv[1] + cos( g_theta )*bv[2] )*bv[2];
	return 0;
}

inline int32_t RotZVec4( const float g_theta, const float bv[4], float v[4] )
{
	v[0] = ( + cos( g_theta )*bv[0] - sin( g_theta )*bv[1] + 0*bv[2] + 0*bv[3] )*bv[0];
	v[1] = ( + sin( g_theta )*bv[0] + cos( g_theta )*bv[1] + 0*bv[2] + 0*bv[3] )*bv[1];
	v[2] = (              0*bv[0] +            0*bv[1] + 1*bv[2] + 0*bv[3] )*bv[2];
	v[3] = (   0*bv[0] +      0      *bv[1] +      0      *bv[2] + 1*bv[3] )*bv[2];
	return 0;
}

inline int32_t RotZVec3( const float g_theta, const float bv[3], float v[3] )
{
	v[0] = ( + cos( g_theta )*bv[0] - sin( g_theta )*bv[1] + 0*bv[2] )*bv[0];
	v[1] = ( + sin( g_theta )*bv[0] + cos( g_theta )*bv[1] + 0*bv[2] )*bv[1];
	v[2] = (              0*bv[0] +            0*bv[1] + 1*bv[2] )*bv[2];
	return 0;
}



inline int32_t RotVec3( const float g_theta[3], const float bv[3], float v[3] )
{
	RotXVec3( g_theta[0], bv, v );
	RotYVec3( g_theta[1], bv, v );
	RotZVec3( g_theta[2], bv, v );
	return 0;
}
inline int32_t RotVec4( const float g_theta[3], const float bv[4], float v[4] )
{
	RotXVec4( g_theta[0], bv, v );
	RotYVec4( g_theta[1], bv, v );
	RotZVec4( g_theta[2], bv, v );
	return 0;
}



#if defined(__cplusplus)
}
#endif

// Compute the floor of the log base 2 of a unsigned integer
inline uint32_t FLOOR_LOG2(const uint32_t n)
{
   uint32_t i;
   for (i = 1; (n >> i) > 0; i++)
   {
       // empty
   }
   return i-1;
}

// helper function to swap the two 16bit values of a 32bit integer
// (needed for ucode constant downloads in big endian systems)
inline uint32_t swapU32_16(const uint32_t v)
{
    return (v>>16) | (v<<16);
}

inline uint32_t swapU32_8(const uint32_t v)
{
    return (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24));
}

inline float swapF32_16(const float f)
{
    union SwapF32_16
    {
        uint32_t ui;
        float f;
    } v;
    v.f = f;
    v.ui = swapU32_16(v.ui);
    return v.f;
}

inline uint16_t swapU16_8(const uint16_t v)
{
    return (v>>8) | (v<<8);
}

inline int _strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2)
	{
		if (*s1 == '\0') return 0;
		s1++;
		s2++;
	}
	return (unsigned char)*s1 - (unsigned char)*s2;
}

inline uint32_t castF32_U32(const float f)
{
	union CastF32_U32
	{
		uint32_t ui;
		float f;
	}v;
	v.f = f;
	return v.ui;
}

//uint32_t のための切り上げ関数
inline uint32_t RoundUp( uint32_t value, uint32_t multiple )
{
	if ( value%multiple>0 ) {
		return value + multiple - ( value%multiple );
	} else {
		return value;
	}
}

//uint32_t のための切り捨て関数
inline uint32_t RoundDown( uint32_t value, uint32_t multiple )
{
	if ( value%multiple>0 ) {
		return value - ( value%multiple );
	} else {
		return value;
	}
	//return (value+(multiple-1)/multiple - 1)*multiple;
	//((x+3)/4)*4
}

inline float Clamp(float val,float minimum,float maximum)
{
	float ret;
	if (val<minimum) {
		ret=minimum;
	}else if (val>maximum) {
		ret=maximum;
	}else{
		ret=val;
	}
	return ret;
}

inline double Saturate(double x)
{
	if (x<0.0f) {
		return 0.0f;
	}else if (x>1.0f) {
		return 1.0f;
	}else{
		return x;
	}
}
inline float Saturatef(float x)
{
	if (x<0.0f) {
		return 0.0f;
	}else if (x>1.0f) {
		return 1.0f;
	}else{
		return x;
	}
}

inline int32_t Clamp(int32_t val,int32_t minimum,int32_t maximum)
{
	uint32_t ret;
	if (val<minimum) {
		ret=minimum;
	}else if (val>maximum) {
		ret=maximum;
	}else{
		ret=val;
	}
	return ret;
}

inline float rand0to1(void)
{
	return rand() / (float)RAND_MAX;
}
inline float randMinus1to1(void)
{
	return 2.0f*(rand0to1())-1.0f;
}

inline float Min(float a, float b)
{
	if (a<b) return a;
	else return b;
}

inline float Max(float a, float b)
{
	if (a>b) return a;
	else return b;
}

template<class T> inline T Max(const T &a, const T &b)
{
	return ( a > b ) ? ( a ) : ( b );
}
template<class T> inline T Min(const T &a, const T &b)
{
	return ( a < b ) ? ( a ) : ( b );
}


} //nsKg

#endif // __HEADER__
