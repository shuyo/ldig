#pragma once
/**
	@file
	@brief definition of abstruct exception class
	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/
#include <string>
#include <cybozu/itoa.hpp>
#include <errno.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <windows.h>
#else
	#include <string.h> // for strerror_r
#endif

namespace cybozu {

const bool DontThrow = true;

namespace exception {

/* get max 32 characters to avoid buffer overrun */
inline std::string makeString(const char *p, size_t size = -1)
{
	const size_t maxSize = 32;
	size_t pos = 0;
	if (p) {
		for (pos = 0; pos < std::min(size, maxSize); pos++) {
			if (p[pos] == 0) break;
		}
	}
	return std::string(p, pos);
}
/**
	convert errno to string
	@param err [in] errno
	@note for both windows and linux
*/
inline std::string ConvertErrorNoToString(int err)
{
	char errBuf[256];
#ifdef _WIN32
	strerror_s(errBuf, sizeof(errBuf), err);
	return errBuf;
#else
	return ::strerror_r(err, errBuf, sizeof(errBuf));
#endif
}

} // cybozu::exception

class Exception : public std::exception {
	std::string str_;
public:
	explicit Exception(const char *name)
		: str_(name)
	{
	}
	~Exception() throw () {}
	const char *what() const throw () { return str_.c_str(); }
	const std::string& toString() const { return str_; }
	/**
		append string into str_
	*/
	Exception& operator<<(const std::string& str)
	{
		str_ += ':';
		str_ += str;
		return *this;
	}
	/**
		append C-string into str_
	*/
	Exception& operator<<(const char *str) { return operator<<(cybozu::exception::makeString(str)); }
	/**
		append char into str_
	*/
	Exception& operator<<(char c) { str_ += ':'; str_ += c; return *this; }
	/**
		append integer into str_
	*/
	template<class P>
	Exception& operator<<(P t) { return operator<<(cybozu::itoa(t)); }
};

class ErrorNo {
#ifdef _WIN32
	typedef unsigned int NativeErrorNo;
#else
	typedef int NativeErrorNo;
#endif
	NativeErrorNo err_;
public:
	explicit ErrorNo(NativeErrorNo err)
		: err_(err)
	{
	}
	ErrorNo()
		: err_(getLatestNativeErrorNo())
	{
	}
	NativeErrorNo getLatestNativeErrorNo() const
	{
#ifdef _WIN32
		return ::GetLastError();
#else
		return errno;
#endif
	}
	/**
		convert NativeErrNo to string(maybe UTF8)
		@param err [in] errno
		@note Linux   : same as ConvertErrorNoToString
			  Windows : for Win32 API(use en-us)
	*/
	std::string toString() const
	{
#ifdef _WIN32
		const int msgSize = 256;
		wchar_t msg[msgSize];
		int size = FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0,
			err_,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			msg,
			msgSize,
			NULL
		);
		if (size <= 0) return "";
		// remove last "\r\n"
		if (size > 2 && msg[size - 2] == '\r') {
			msg[size - 2] = 0;
			size -= 2;
		}
		std::string ret;
		ret.resize(size);
		// assume ascii only
		for (int i = 0; i < size; i++) {
			ret[i] = (char)msg[i];
		}
		return ret;
#else
		return exception::ConvertErrorNoToString(err_);
#endif
	}
};

} // cybozu
