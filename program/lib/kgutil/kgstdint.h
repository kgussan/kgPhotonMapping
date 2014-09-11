//stdintíuÇ´ä∑Ç¶ÅB
#ifndef __KGSTDINT_H__
#define __KGSTDINT_H__

#ifndef _MSC_VER
// except windows
//#error "_MSC_VER is not defined. Is this Visual Studio compiler ?"
#include <stdint.h>
#else

//windows env
#include <limits.h>

#if (_MSC_VER < 1300)
typedef signed char			int8_t;
typedef signed short		int16_t;
typedef signed int			int32_t;
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
#else
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#endif

typedef signed __int64		int64_t;
typedef unsigned __int64	uint64_t;

#ifndef INT8_MIN
#define INT8_MIN     ((int8_t)_I8_MIN)
#endif
#ifndef INT8_MAX
#define INT8_MAX     _I8_MAX
#endif
#ifndef INT16_MIN
#define INT16_MIN    ((int16_t)_I16_MIN)
#endif
#ifndef INT16_MAX
#define INT16_MAX    _I16_MAX
#endif
#ifndef INT32_MIN
#define INT32_MIN    ((int32_t)_I32_MIN)
#endif
#ifndef INT32_MAX
#define INT32_MAX    _I32_MAX
#endif
#ifndef INT64_MIN
#define INT64_MIN    ((int64_t)_I64_MIN)
#endif
#ifndef INT64_MAX
#define INT64_MAX    _I64_MAX
#endif
#ifndef UINT8_MAX
#define UINT8_MAX    _UI8_MAX
#endif
#ifndef UINT16_MAX
#define UINT16_MAX   _UI16_MAX
#endif
#ifndef UINT32_MAX
#define UINT32_MAX   _UI32_MAX
#endif
#ifndef UINT64_MAX
#define UINT64_MAX   _UI64_MAX
#endif

#ifdef _WIN64
#	ifndef INTPTR_MIN
#	define INTPTR_MIN   INT64_MIN
#	endif
#	ifndef INTPTR_MAX
#	define INTPTR_MAX   INT64_MAX
#	endif
#	ifndef UINTPTR_MAX
#	define UINTPTR_MAX  UINT64_MAX
#	endif
#else
#	ifndef INTPTR_MIN
#	define INTPTR_MIN   INT32_MIN
#	endif
#	ifndef INTPTR_MAX
#	define INTPTR_MAX   INT32_MAX
#	endif
#	ifndef UINTPTR_MAX
#	define UINTPTR_MAX  UINT32_MAX
#	endif
#endif

#endif

#endif // __KGSTDINT_H__

