#pragma once

/**
	@file
	@brief double array

	Copyright (C) 2013 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

#include <algorithm>
#include <queue>
#include <stack>
#include "cybozu/string.hpp"
#include "type.hpp"
#include "esaxx/esa.hxx"

namespace cybozu {
namespace ldig {

typedef int CHAR;
const int K = 0x10000;	// max CHAR + 1
const LdigChar charLF = 1;

typedef std::vector<Feature> FeatureVec;

class Features : public FeatureVec {
	LdigString fulltext;
	int nodeNum;
public:
	const LdigString &text() const { return fulltext; }
	int nodesize() const { return nodeNum; }

	void settext(const std::string &str) {
		fulltext = str;
	}
	void settext(const LdigString &str) {
		fulltext = str;
	}
	Features() : nodeNum(0) {}

	Features(const LdigString &str, size_t bound_feature_freq) : nodeNum(0) {
		extract(str, bound_feature_freq);
	}

	Features(const LdigString &str, const FeatureVec &features) :
		fulltext(str), nodeNum(0), FeatureVec(features)
	{}

	void extract(const LdigString &str, size_t bound_feature_freq)
	{
		fulltext = str;
		size_t len = fulltext.size();
		std::vector<int> SA(len), L(len), R(len), D(len), rank(len);
		auto icv = fulltext.begin(), icvend=fulltext.end();
		for (;icv!=icvend;++icv) {
			if (*icv == 0 || *icv >= K) *icv = 32;
		}

		if (esaxx(fulltext.begin(), SA.begin(), L.begin(), R.begin(), D.begin(), (int)len, K, nodeNum) == -1){
			nodeNum = -1;
			return;
		}

		int r = 0;
		for (size_t i = 0; i < len; i++) {
			if (i == 0 || fulltext[(SA[i] + len - 1) % len] != fulltext[(SA[i - 1] + len - 1) % len]) r++;
			rank[i] = r;
		}

		for (int i = 0; i < nodeNum; ++i){
			unsigned int c = rank[ R[i] - 1 ] - rank[ L[i] ];
			if (D[i] > 0 && c + 1 >= bound_feature_freq) {
				size_t begin = SA[L[i]], len = D[i];
				bool noLF = true, hasLetter = false;
				for (size_t j=0;j<len;++j) {
					auto c = fulltext[begin+j];
					if (c<=0x40) {
						if (c == charLF && j>0 && j<len-1) {
							noLF = false;
							break;
						}
					} else if (c<=0x5a || (c>=0x61 && c<=0x7a) || (c>=0xc0 && c<0x2000) || (c>=0x20a0 && c<0x20d0) || (c>=0x2c00 && c<0x3000) || (c>=0x3040)) {
						hasLetter = true;
					}
				}
				if (noLF && hasLetter) {
					push_back(Feature(begin, len, c + 1));
				}
			}
		}
		std::sort(begin(), end(), 
			[&](const Feature &a, const Feature &b)->bool {
			size_t i=a.begin, j=b.begin;
			while (i<a.begin+a.len && j<b.begin+b.len) {
				if (fulltext[i] > fulltext[j]) return false;
				if (fulltext[i] < fulltext[j]) return true;
				++i;
				++j;
			}
			if (a.len>b.len) return false;
			return true;
		});
	}

	Features(const Features &features) : nodeNum(0), FeatureVec(features) {
		typedef std::pair<size_t, Feature> IdxFtr;
		std::vector<IdxFtr> list;
		for(size_t i=0;i<features.size();++i) list.push_back(IdxFtr(i, features[i]));
		std::sort(list.begin(), list.end(), 
			[](const IdxFtr &a, const IdxFtr &b)->bool {return a.second.len > b.second.len;});

		for(auto i=list.begin(), ie=list.end();i!=ie;++i) {
			LdigString st(i->second.str(features.text()));
			size_t j = fulltext.find(st);
			if (j==fulltext.npos) {
				j = fulltext.length();
				fulltext.append(st);
			}
			at(i->first).begin = j;
		}
	}

	void shrink() {
		typedef std::pair<size_t, Feature> IdxFtr;
		std::vector<IdxFtr> list;
		for(size_t i=0;i<size();++i) list.push_back(IdxFtr(i, at(i)));
		std::sort(list.begin(), list.end(), 
			[](const IdxFtr &a, const IdxFtr &b)->bool {
				if (a.second.begin == b.second.begin) return a.second.len > b.second.len;
				return a.second.begin < b.second.begin;
		});

		LdigString orgstr;
		orgstr.swap(fulltext);

		size_t pre_begin = 0, cur_begin = 0, pre_end = 0;
		for(auto i=list.begin(), ie=list.end();i!=ie;++i) {
			if (i->second.begin > pre_begin) {
				if (i->second.begin > pre_end) {
					cur_begin += pre_end - pre_begin;
				} else {
					cur_begin += i->second.begin - pre_begin;
				}
			}
			at(i->first).begin = cur_begin;
			pre_begin = i->second.begin;
			if (pre_end < pre_begin + i->second.len) {
				pre_end = pre_begin + i->second.len;
				size_t cur_end = pre_begin + (fulltext.size() - cur_begin);
				if (pre_end > cur_end)
					fulltext.append(orgstr.begin() + cur_end, orgstr.begin() + pre_end);
			}
		}
	}

	inline LdigString str(size_t index) const {
		return at(index).str(fulltext);
	}
};


class Pos {
public:
	unsigned int index, left, right, depth;
	Pos(unsigned int i, unsigned int l, unsigned int r, unsigned int d) : index(i), left(l), right(r), depth(d) {}
};

class Branch {
public:
	unsigned int index;
	LdigChar chr;
	Branch(unsigned int i, LdigChar c) : index(i), chr(c) {}
};

class DoubleArray {
	size_t N;
public:
	std::vector<int> base, check, value;

	size_t size() const { return N; }

	void extend_array(size_t max_cand) {
		size_t oldN = N;
		while (N <= max_cand) N *= 2;
		if (N <= oldN) return;
		base.resize(N);
		check.resize(N);
		value.resize(N, -1);
		for (size_t n=oldN;n<N;++n) {
			base[n] = (int)n - 1;
			check[n] = - (int)n - 1;
		}
	}

	void construct(const Features &features) {
		N = 1;
		base.clear();
		check.clear();
		value.clear();
		base.push_back(-1);
		check.push_back(-1);
		value.push_back(-1);

		std::queue<Pos> queue;
		queue.push(Pos(0, 0, (unsigned int)features.size(), 0));
		int max_index = 0;
		const LdigString &text = features.text();
		while(!queue.empty()) {
			const Pos &pos = queue.front();
			unsigned int index = pos.index, left = pos.left, right = pos.right, depth = pos.depth;
			queue.pop();
			if (depth >= features[left].len) {
				value[index] = left++;
				if (left >= right) continue;
			}

			// get branches of current node
			std::stack<Branch> stack;
			std::vector<Branch> result;
			stack.push(Branch(right, -1));
			unsigned int cur_index = left;
			LdigChar cur_chr = text[features[left].begin + depth];
			while (!stack.empty()) {
				while (cur_chr == stack.top().chr) {
					cur_index = stack.top().index;
					cur_chr =  stack.top().chr;
					stack.pop();
				}
				unsigned int mid = (cur_index + stack.top().index) / 2;
				if (cur_index == mid) {
					result.push_back(Branch(cur_index + 1, cur_chr));
					cur_index = stack.top().index;
					cur_chr =  stack.top().chr;
					stack.pop();
				} else {
					LdigChar c2 = text[features[mid].begin + depth];
					if (cur_chr != c2) {
						stack.push(Branch(mid, c2));
					} else {
						cur_index = mid;
					}
				}
			}

			// search empty index for current node
			LdigChar v0 = result[0].chr;
			int j = - check[0] - (int)v0;
			while (true) {
				auto i = result.begin(), ie = result.end();
				for (;i!=ie;++i) {
					size_t k = j + i->chr;
					if (k < N && check[k] >= 0) break;
				}
				if (i==ie) break;
				j = - check[j + v0] - v0;
			}
			int tail_index = j + result.back().chr;
			if (max_index < tail_index) {
				max_index = tail_index;
				extend_array(tail_index + 2);
			}

			// insert current node into DA
			base[index] = j;
			depth++;
			for (auto i = result.begin(), ie = result.end();i!=ie;++i) {
				int child = j + i->chr;
				check[base[child]] = check[child];
				base[-check[child]] = base[child];
				check[child] = index;
				queue.push(Pos(child, left, i->index, depth));
				left = i->index;
			}
		}
		shrink(max_index);
	}

	void shrink(size_t max_index) {
		N = max_index + 1;
		check.resize(N);
		base.resize(N);
		value.resize(N);
	}

	DoubleArray() {}
	DoubleArray(const Features &features) {
		construct(features);
	}

	int get(const LdigString &key) const {
		size_t cur = 0;
		for(auto i=key.begin(), ie=key.end();i!=ie;++i) {
			size_t next = base[cur] + *i;
			if (next >= N || check[next] != cur) return -1;
			cur = next;
		}
		int v = value[cur];
		if (v>=0) return v;
		return -1;
	}

	void extract_features(Events &events, LdigString::const_iterator begin, LdigString::const_iterator end) const {
		for (auto i1=begin;i1!=end;++i1) {
			size_t pointer = 0;
			for(auto i2=i1;i2!=end;++i2) {
				size_t next = base[pointer] + *i2;
				if (next >= N || check[next] != pointer) break;
				int id = value[next];
				if (id >= 0) {
					events[id]++;
				}
				pointer = next;
			}
		}
	}

	void extract_features(Events &events, const LdigString &key) const {
		extract_features(events, key.begin(), key.end());
	}

	void extract_features(Events &events, const LdigString &fulltext, const TextPos &pos) const {
		LdigString::const_iterator begin = fulltext.begin() + pos.begin;
		extract_features(events, begin, begin + pos.len);
	}
};

}}
