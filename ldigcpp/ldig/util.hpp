#pragma once
/**
	@file
	@brief utility for ldig

	Copyright (C) 2013 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

#include "cybozu/string.hpp"
#include "esaxx/esa.hxx"

namespace cybozu {
namespace ldig {
	
inline void replace(cybozu::String& str, const cybozu::String& from, cybozu::Char to) {
	cybozu::String::size_type pos = 0;
	while (pos = str.find(from, pos), pos != cybozu::String::npos) {
		str[pos] = to;
		++pos;
	}
}

template <class T>
void printvec(const std::vector<T> &vec) {
	auto i=vec.begin(), iend=vec.end();
	std::cout << "( ";
	for(;i!=iend;++i) std::cout << *i << " ";
	std::cout << ")" << std::endl;
}

}}
