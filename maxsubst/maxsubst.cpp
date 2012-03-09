/**
	@file
	@brief maximal substring extractor

	Copyright (C) 2012 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

#include <iostream>
#include <vector>
#include <fstream>
#include "esa.hxx"
#include "cybozu/string.hpp"

const int k = 0x10000;

void replace(cybozu::String& str, const cybozu::String& from, cybozu::Char to) {
    cybozu::String::size_type pos = 0;
    while (pos = str.find(from, pos), pos != cybozu::String::npos) {
		str[pos] = to;
        ++pos;
    }
}

int main(int argc, char* argv[]){

	std::ifstream ifs(argv[1], std::ios::binary);
	cybozu::String str(std::istreambuf_iterator<char>(ifs.rdbuf()), std::istreambuf_iterator<char>());
	//std::istreambuf_iterator<char> isit(std::cin);
	//cybozu::String str(isit, std::istreambuf_iterator<char>());

	replace(str, "\n", 1);	// replace \n => \u0001
	replace(str, "\t", 32);	// replace \t => ' '
	//replace(str, "\0", 32);	// replace \0 => ' '
	size_t origLen = str.size();
	std::cerr << "    chars:" << origLen << std::endl;

	std::vector<int> charvec;
	charvec.resize(origLen);
	std::copy(str.begin(), str.end(), charvec.begin());
	std::vector<int>::iterator icv = charvec.begin(), icvend=charvec.end();
	for (;icv!=icvend;++icv) {
		if (*icv == 0 || *icv >= k) *icv = 32;
	}

	std::vector<int> SA(origLen);
	std::vector<int> L (origLen);
	std::vector<int> R (origLen);
	std::vector<int> D (origLen);

	int nodeNum = 0;
	if (esaxx(charvec.begin(), SA.begin(), L.begin(), R.begin(), D.begin(), (int)origLen, k, nodeNum) == -1){
		return -1;
	}
	std::cerr << "    nodes:" << nodeNum << std::endl;

	std::vector<int> rank(origLen);
	int r = 0;
	for (size_t i = 0; i < origLen; i++) {
		if (i == 0 || charvec[(SA[i] + origLen - 1) % origLen] != charvec[(SA[i - 1] + origLen - 1) % origLen]) r++;
		rank[i] = r;
	}

	/*
	for (int i = 0; i < nodeNum; ++i){
	cout << i << "\t" << R[i] - L[i] << "\t"  << D[i] << "\t"  << L[i] << "\t"  << SA[L[i]] << "\t" << (rank[ R[i] - 1 ] - rank[ L[i] ])  << "\t" << "'" << str.substr(SA[L[i]], D[i]) << "'";
	//printSnipet(T, SA[L[i]], D[i], id2word);
	cout << std::endl;
	}
	*/

	std::ofstream ofs(argv[2], std::ios::binary);
	int maxsubst = 0;
	for (int i = 0; i < nodeNum; ++i){
		int c = rank[ R[i] - 1 ] - rank[ L[i] ];
		if (D[i] > 0 && c > 0) {
			ofs << str.substr(SA[L[i]], D[i]) << "\t" << c + 1 << std::endl;
			++maxsubst;
		}
	}
	std::cerr << " maxsubst:" << maxsubst << std::endl;

	return 0;
}
