#pragma once
/**
	@file
	@brief convert integer to string(ascii)

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <limits.h>
#include <string>
#include <cybozu/inttype.hpp>

namespace cybozu {

namespace itoa_local {

template<typename T, typename UT, int n>
void convertFromInt(std::string& out, T x, T minusMax, const char (&minusMaxStr)[n])
{
	if (x == minusMax) {
		out.assign(minusMaxStr, minusMaxStr + n - 1);
		return;
	}
	if (x == 0) {
		out.assign(1, '0');
		return;
	}
	UT absX = x < 0 ? -x : x;
	char buf[40];
	int p = 0;
	while (absX > 0) {
		buf[p++] = '0' + (int)(absX % 10);
		absX /= 10;
	}
	if (x < 0) {
		buf[p++] = '-';
	}
	out.resize(p);
	for (int i = 0; i < p; i++) {
		out[i] = buf[p - 1 - i];
	}
}

template<typename T>
void convertFromUint(std::string& out, T x)
{
	if (x == 0) {
		out.assign(1, '0');
		return;
	}
	char buf[40];
	int p = 0;
	while (x > 0) {
		buf[p++] = '0' + (int)(x % 10);
		x /= 10;
	}
	out.resize(p);
	for (int i = 0; i < p; i++) {
		out[i] = buf[p - 1 - i];
	}
}

/**
	convert to to zero padding hex
*/
template<typename T>
void convertFromUintToHexWithZero(std::string& out, T x, bool upCase)
{
	const size_t len = sizeof(T) * 2;
	out.resize(len);
	static const char *hexTbl[] = {
		"0123456789abcdef",
		"0123456789ABCDEF"
	};
	const char *tbl = hexTbl[upCase];
	for (size_t i = 0; i < len; i++) {
		out[len - i - 1] = tbl[x % 16];
		x /= 16;
	}
}

} // itoa_local

/**
	convert int to string
	@param out [out] string
	@param x [in] int
*/
inline void itoa(std::string& out, int x)
{
	itoa_local::convertFromInt<int, unsigned int>(out, x, INT_MIN, "-2147483648");
}

/**
	convert int64_t to string
	@param out [out] string
	@param x [in] int64_t
*/
inline void itoa(std::string& out, int64_t x)
{
	itoa_local::convertFromInt<int64_t, uint64_t>(out, x, LLONG_MIN, "-9223372036854775808");
}

/**
	convert unsigned int to string
	@param out [out] string
	@param x [in] unsigned int
*/
inline void itoa(std::string& out, unsigned int x)
{
	itoa_local::convertFromUint(out, x);
}

/**
	convert uint64_t to string
	@param out [out] string
	@param x [in] uint64_t
*/
inline void itoa(std::string& out, uint64_t x)
{
	itoa_local::convertFromUint(out, x);
}

/**
	convert integer to string
	@param x [in] int
*/
template<typename T>
inline std::string itoa(T x)
{
	std::string ret;
	itoa(ret, x);
	return ret;
}

inline void itohex(std::string& out, unsigned char x, bool upCase = true)
{
	itoa_local::convertFromUintToHexWithZero(out, x, upCase);
}

inline void itohex(std::string& out, unsigned short x, bool upCase = true)
{
	itoa_local::convertFromUintToHexWithZero(out, x, upCase);
}

inline void itohex(std::string& out, unsigned int x, bool upCase = true)
{
	itoa_local::convertFromUintToHexWithZero(out, x, upCase);
}

inline void itohex(std::string& out, uint64_t x, bool upCase = true)
{
	itoa_local::convertFromUintToHexWithZero(out, x, upCase);
}

template<typename T>
inline std::string itohex(T x, bool upCase = true)
{
	std::string out;
	itohex(out, x, upCase);
	return out;
}

/**
	convert integer to string with zero padding
	@param x [in] int
	@param len [in] minimum lengh of string
	@param c [in] padding character
	@note
	itoa(12, 4) == "0012"
	itoa(1234, 4) == "1234"
	itoa(12345, 4) == "12345"
	itoa(-12, 4) == "-012"
*/
template<typename T>
inline std::string itoaWithZero(T x, size_t len, char c = '0')
{
	std::string ret;
	itoa(ret, x);
	if (ret.size() < len) {
		std::string zero(len - ret.size(), c);
		if (x >= 0) {
			ret = zero + ret;
		} else {
			ret = "-" + zero + ret.substr(1);
		}
	}
	return ret;
}

} // cybozu
