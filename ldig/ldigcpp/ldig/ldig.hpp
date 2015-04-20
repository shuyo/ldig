#pragma once

/**
	@file
	@brief language detection with infinite grams

	Copyright (C) 2013 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

#include <ctime>
#include <algorithm>
#include <math.h>
#include "type.hpp"
#include "da.hpp"
#include "corpus.hpp"

namespace cybozu {
namespace ldig {

static const std::string nomatchedlabel = "**";

class Model {
public:
	size_t K, M;
	Features features;
	DoubleArray trie;
	std::unordered_map<std::string, size_t> labelmap;
	std::vector<std::string> labellist;
	std::vector<LdigFloat> params;

	Model() {
		std::srand((unsigned int)time(0));
	}

	Model(const size_t K_) : K(K_) {
		std::srand((unsigned int)time(0));
	}

	Model(const Corpus &corpus, const size_t bound_feature_freq) : K(corpus.labels().size()) {
		for (auto i=corpus.texts().begin(), iend=corpus.texts().end();i!=iend;++i) {
			labellist.push_back(i->label);
		}
		generate_labelmap();
		generate_features(corpus.text(), bound_feature_freq);
	}

	void generate_features(const LdigString &str, size_t bound_feature_freq) {
		features.extract(str, bound_feature_freq);
		post_features();
	}

	void post_features() {
		M = features.size();
		params.resize(K * M);
		trie.construct(features);
	}

	void generate_labellist() {
		labellist.resize(K);
		for (auto i=labelmap.begin(),ie=labelmap.end();i!=ie;++i) {
			labellist[i->second] = i->first;
		}
	}

	void generate_labelmap() {
		for(size_t k=0;k<K;++k) {
			labelmap[labellist[k]] = k;
		}
	}

	const std::string& label(const size_t k) const {
		if (k>=K) return nomatchedlabel;
		return labellist[k];
	};

	size_t label(const std::string &st) const {
		return labelmap.at(st);
	}

	/*
		@brief  parameter update for unit test only ( same with inner code of learn method )
	*/
	void update(const Events &events, const size_t label_k, const LdigFloat eta) {
		std::vector<LdigFloat> y(K);
		predict(y, events);
		y[label_k] -= 1;

		for (auto k=y.begin(), ke=y.end(); k!=ke; ++k) *k *= eta;
		for (auto i=events.begin(), ie=events.end(); i!=ie; ++i) {
			auto j = params.begin() + i->first * K;
			for (auto k=y.begin(), ke=y.end(); k!=ke; ++k, ++j) {
				*j -= i->second * *k;
			}
		}
	}

	void learn(const Corpus &corpus, LdigFloat eta, const LdigFloat reg = 0) {
		const size_t D = corpus.maxsize();
		const size_t N = D * K;
		std::vector<size_t> index(N);
		for (size_t i=0;i<D*K;++i) index[i] = i;
		std::random_shuffle(index.begin(), index.end());

		// for regularization
		const bool withreg = reg > 0;
		std::vector<LdigFloat> penalties;
		LdigFloat uk = 0, alpha = 0;
		if (withreg) {
			penalties.resize(M * K);
			alpha = pow(0.9, -1.0 / N);
		}
		const size_t N_WHOLE_REG = 100;
		const size_t WHOLE_REG_INT = (N / N_WHOLE_REG) + 1;

		std::vector<LdigFloat>::iterator prm, pnl, pe=params.end();
		auto L1regularize = [&prm, &pnl, &uk](){
			LdigFloat w = *prm;
			if (w > 0) {
				LdigFloat w1 = w - uk - *pnl;
				if (w1 > 0) {
					*prm = w1;	*pnl += w1 - w;
				} else {
					*prm = 0;	*pnl -= w;
				}
			} else if (w < 0) {
				LdigFloat w1 = w + uk - *pnl;
				if (w1 < 0) {
					*prm = w1;	*pnl += w1 - w;
				} else {
					*prm = 0;	*pnl -= w;
				}
			}
		};


		const LdigString &fulltext = corpus.text();
		size_t ni = 0;
		std::vector<LdigFloat> y(K);
		for (auto n=index.begin(), ne=index.end(); n!=ne; ++n, ++ni) {
			size_t label_k = *n / D;
			const std::vector<TextPos> &v = corpus.texts()[label_k].vec;

			size_t r = *n % D;
			if (r / v.size() == D / v.size()) {
				r = std::rand() % v.size();
			} else {
				r %= v.size();
			}

			Events events;
			trie.extract_features(events, fulltext, v[r]);
			y.clear();
			predict(y, events);
			y[label_k] -= 1;	// t_n - y_n

			if (withreg) {
				// with L1 regularization
				eta *= alpha;
				uk += reg * eta / N;
				for (auto k=y.begin(), ke=y.end(); k!=ke; ++k) *k *= eta;

				if ((N-ni) % WHOLE_REG_INT == 1) {
					for (auto i=events.begin(), ie=events.end(); i!=ie; ++i) {
						prm = params.begin() + i->first * K;
						for (auto k=y.begin(), ke=y.end(); k!=ke; ++k, ++prm) {
							*prm -= *k * i->second;
						}
					}
				
					for (prm=params.begin(), pnl=penalties.begin(); prm!=pe; ++prm, ++pnl) {
						L1regularize();
					}
				} else {
					for (auto i=events.begin(), ie=events.end(); i!=ie; ++i) {
						prm = params.begin() + i->first * K;
						pnl = penalties.begin() + i->first * K;
						for (auto k=y.begin(), ke=y.end(); k!=ke; ++k, ++prm, ++pnl) {
							*prm -= *k * i->second;
							L1regularize();
						}
					}
				}

			} else if (y[label_k] < -LdigAlmostZero) {

				// same with update(events, label_k, eta);
				for (auto k=y.begin(), ke=y.end(); k!=ke; ++k) *k *= eta;
				for (auto i=events.begin(), ie=events.end(); i!=ie; ++i) {
					auto j = params.begin() + i->first * K;
					for (auto k=y.begin(), ke=y.end(); k!=ke; ++k, ++j) {
						*j -= *k * i->second;
					}
				}
			}
		}
	}

	/*
		@brief return negative log likelihood
	*/
	LdigFloat likelihood(std::vector<size_t> &correct, const Corpus &corpus) {
		if (correct.size() != corpus.texts().size()) correct.resize(corpus.texts().size());

		const LdigString &fulltext = corpus.text();
		LdigFloat log_likelihood = 0;
		const std::vector<TextVec> &texts = corpus.texts();
		auto cor = correct.begin();
		std::vector<LdigFloat> y(K);
		for (auto k=texts.begin(), ke=texts.end();k!=ke;++k,++cor) {
			const std::vector<TextPos> &v = k->vec;
			const size_t label_k = labelmap.at(k->label);
			for (auto t=v.begin(), te=v.end(); t!=te; ++t) {
				Events events;
				trie.extract_features(events, fulltext, *t);

				y.clear();
				size_t predict_k = predict(y, events);
				if (predict_k == label_k) *cor += 1;
				if (y[label_k] > 0) log_likelihood -= log(y[label_k]);
			}
		}
		return log_likelihood; //exp(log_likelihood / corpus.size());
	}

	size_t predict(std::vector<LdigFloat> &result, const Events &events) const {
		if (result.size() != K) result.resize(K);

		for (auto i=events.begin(), ie=events.end(); i!=ie; ++i) {
			auto j = params.begin() + i->first * K;
			for (auto k=result.begin(), ke=result.end(); k!=ke; ++k, ++j) {
				*k += i->second * *j;
			}
		}

		LdigFloat max = LdigFloatMin;
		size_t maxindex = 0;
		for (auto k=result.begin(), ke=result.end(); k!=ke; ++k) {
			if (max < *k) {
				max = *k;
				maxindex = k - result.begin();
			}
		}

		LdigFloat sum = 0;
		for (auto k=result.begin(), ke=result.end(); k!=ke; ++k) {
			*k = exp(*k - max);
			sum += *k;
		}

		for (auto k=result.begin(), ke=result.end(); k!=ke; ++k) {
			*k /= sum;
		}

		return maxindex;
	}

	size_t predict(std::vector<LdigFloat> &result, const LdigString &fulltext, const TextPos &pos) const {
		Events events;
		trie.extract_features(events, fulltext, pos);
		return predict(result, events);
	}

	size_t predict(std::vector<LdigFloat> &result, const LdigString &text) const {
		Events events;
		trie.extract_features(events, text);
		return predict(result, events);
	}


	void save(std::ostream &ofs) const {
		unsigned char size_size_t = sizeof(size_t);
		ofs.write((const char*)(&size_size_t), sizeof(size_size_t));

		const unsigned int K_ = (unsigned int)K, M_ = (unsigned int)M;
		ofs.write((const char*)(&K_), sizeof(K_));
		ofs.write((const char*)(&M_), sizeof(M_));

		for (auto i=labellist.begin(), ie=labellist.end();i!=ie;++i) {
			unsigned char c = (unsigned char)i->size();
			ofs.write((const char*)(&c), sizeof(c));
			ofs.write(i->c_str(), i->size());
		}

		std::string text;
		features.text().toUtf8(text);
		const size_t textsize = text.size();
		ofs.write((const char*)(&textsize), sizeof(textsize));
		ofs.write(text.c_str(), textsize);

		ofs.write((const char*)&features[0], sizeof(Feature) * M);
		ofs.write((const char*)&params[0], sizeof(LdigFloat) * K * M);
	}

	void load(std::istream &ifs) {
		int size_size_t = ifs.get();
		if (size_size_t != sizeof(size_t)) {
			if (size_size_t==4) throw Exception("cannot load a model builded on 32 bit platform");
			throw Exception("cannot load a model builded on 64 bit platform");
		}

		unsigned int K_, M_;
		ifs.read((char *)&K_, sizeof(K_));
		ifs.read((char *)&M_, sizeof(M_));
		K = K_;
		M = M_;

		labellist.resize(K);
		for (size_t k=0;k<K;++k) {
			std::string &s = labellist[k];
			int c = ifs.get();
			s.resize(c);
			ifs.read(&s[0], c);
		}
		generate_labelmap();

		size_t textsize;
		ifs.read((char *)&textsize, sizeof(textsize));
		std::string text;
		text.resize(textsize);
		ifs.read(&text[0], textsize);

		features.settext(text);		// to cybozu::String
		features.resize(M);
		params.resize(K * M);
		ifs.read((char *)&features[0], sizeof(Feature) * M);
		ifs.read((char *)&params[0], sizeof(LdigFloat) * K * M);

		post_features();
	}

	std::vector<size_t> shrink() {
		Features new_features;
		new_features.settext(features.text());

		std::vector<LdigFloat> new_params;

		std::vector<size_t> summary(K+1);
		auto p=params.begin();
		for(auto f=features.begin(), fe=features.end(); f!=fe; ++f, p+=K) {
			size_t nonzeros = 0;
			for (size_t k=0;k<K;++k) {
				if (abs(*(p+k)) > LdigAlmostZero) ++nonzeros;
			}
			++summary[nonzeros];
			if (nonzeros > 0) {
				new_features.push_back(*f);
				//for (size_t k=0;k<K;++k) new_params.push_back(*(p+k));
				new_params.insert(new_params.end(), p, p+K);
			}
		}
		new_features.shrink();
		
		features = std::move(new_features);
		params = std::move(new_params);
		M = features.size();
		trie.construct(features);

		return summary;
	}

};

}}
