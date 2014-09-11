//É}ÉNÉç
#ifndef __HEADER_KG_MACRO_H__
#define __HEADER_KG_MACRO_H__
#include <stdio.h>
#if (1)
#	define LogDebug(...)	printf("Debug %s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogDebug(...)
#endif 

//
#if (0)
#	define LogTrace(...)	printf("Trace %s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogTrace(...)
#endif 

#if (1)
#	define LogInfo(...)	printf("Info %s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogInfo(...)
#endif 

#if (1)
#	define LogWarn(...)	printf("Warn%s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogWarn(...)
#endif 

#if (1)
#	define LogError(...)	printf("Error %s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogError(...)
#endif 

#if (1)
#	define LogFatal(...)	printf("Fatal %s %d %s: ",__FILE__,__LINE__,__FUNCTION__);printf(__VA_ARGS__)
#else
#	define LogFatal(...)
#endif 

//#define NAMESPACE_KG_START namespace KG{
//#define NAMESPACE_KG_END }

#define KG_RETURN_SUCCESS 0
#define KG_RETURN_FAILURE -1

#ifndef DEG2RAD
#define DEG2RAD(deg)((deg)*M_PI/180.0f)
#endif

#ifndef RAD2DEG
#define RAD2DEG(rad) ((rad)*180.0f/M_PI)
#endif

#define GET_ARRAY_NUMBER(arr)  (sizeof(arr)/sizeof(arr[0]))
#endif // __HEADER_KG_MACRO_H__


