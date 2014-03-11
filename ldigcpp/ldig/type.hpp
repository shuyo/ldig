#pragma once
/**
	@file
	@brief data type for ldig

	Copyright (C) 2013 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

#include <cfloat>
#include <unordered_map>
#include "cybozu/string.hpp"

typedef double LdigFloat;
const double LdigFloatMin = DBL_MIN;
const double LdigAlmostZero = 1e-7;

typedef cybozu::String LdigString;
typedef cybozu::Char LdigChar;

namespace cybozu {
namespace ldig {

typedef std::unordered_map<size_t, size_t> Events;

class Feature {
public:
	size_t begin, len;
	//unsigned int count;
	Feature() {}
	Feature(size_t b, size_t l, unsigned int c) : begin(b), len(l) {}
	inline LdigString str(const LdigString& text) const { return text.substr(begin, len); }
};

class TextPos {
public:
	size_t begin, len;
	TextPos(size_t b, size_t l) : begin(b), len(l) {}
};

class TextVec
{
public:
	std::string label;
	std::vector<TextPos> vec;
	TextVec(const std::string &l) : label(l) {}
};

class Exception : public std::exception {
private:
	std::string message_;
public:
	Exception(const std::string& message) : message_(message) {}
	virtual ~Exception() throw() {}

	virtual const char* what() const throw() {
		return message_.c_str();
	}
};

}}
