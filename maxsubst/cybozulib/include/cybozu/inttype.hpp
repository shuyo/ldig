#pragma once
/**
	@file
	@brief int type definition and macros

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/

#if defined(_MSC_VER) && (MSC_VER <= 1500)
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned int uint32_t;
	typedef int int32_t;
	typedef unsigned short uint16_t;
	typedef short int16_t;
	typedef unsigned char uint8_t;
	typedef signed char int8_t;
#else
	#include <stdint.h>
#endif

#ifdef _MSC_VER
	#ifndef CYBOZU_DEFINED_SSIZE_T
		#define CYBOZU_DEFINED_SSIZE_T
		#ifdef _WIN64
			typedef int64_t ssize_t;
		#else
			typedef int32_t ssize_t;
		#endif
	#endif
#else
	#include <unistd.h> // for ssize_t
#endif

// std::vector<int> v; CYBOZU_FOREACH(auto x, v) {...}
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	#define CYBOZU_FOREACH(type_x, xs) for each (type_x in xs)
#elif defined(__GNUC__)
	#define CYBOZU_FOREACH(type_x, xs) for (type_x : xs)
#endif

#define CYBOZU_NUM_OF_ARRAY(x) (sizeof(x) / sizeof(*x))

#ifdef _MSC_VER
	#define CYBOZU_SNPRINTF _snprintf_s
#else
	#define CYBOZU_SNPRINTF snprintf
#endif
