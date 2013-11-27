/**
	@file
	@brief language detection with infinite-gram

	Copyright (C) 2013 Nakatani Shuyo / Cybozu Labs, Inc., all rights reserved.
*/

//#define CYBOZU_USE_STACKTRACE 
//#define CYBOZU_STACKTRACE_RESOLVE_SYMBOL

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>

#include "type.hpp"
#include "corpus.hpp"
#include "da.hpp"
#include "ldig.hpp"
#include "util.hpp"

//ifstream.exception(std::ifstream::failbit | std::ifstream::badbit);

#ifdef _WIN32
#include <Psapi.h>

size_t worksize() {
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return info.WorkingSetSize;
}
#else
size_t worksize() {
	return 0;
}
#endif


enum MODE {
	initialization,
	learning,
	detection,
	shrink,
	maxsubst,
	dump,
	varidation
};

const size_t BUFSIZE = 4096;

void saveModel(const cybozu::ldig::Model &model, const std::string &modelpath) {
	std::ofstream ofs(modelpath, std::ios::binary);
	if (!ofs.is_open()) {
		throw std::exception("cannot open the model path");
	}
	model.save(ofs);
	ofs.close();
}

void loadModel(cybozu::ldig::Model &model, const std::string &modelpath) {
	std::ifstream ifs(modelpath, std::ios::binary);
	if (!ifs.is_open()) {
		throw std::exception("cannot open the model path");
	}
	model.load(ifs);
	ifs.close();
}

void ldig_dump(const std::string &modelpath, const std::string &outputpath) {
	cybozu::ldig::Model model;
	loadModel(model, modelpath);

	std::ofstream ofs;
	ofs.open(outputpath, std::ios::binary);
	if (!ofs.is_open()) {
		throw std::exception("cannot open output file");
	}
	//ofs.precision(3);
	//ofs << std::fixed;

	for (auto i=model.features.cbegin(), ie=model.features.cend();i!=ie;++i) {
		ofs << i->str(model.features.text()) << std::endl;
	}
	ofs.close();
}

void ldig_shrink(const std::string &modelpath) {
	cybozu::ldig::Model model;
	loadModel(model, modelpath);

	const size_t K = model.K;
	const size_t org_M = model.M;
	const size_t org_text_len = model.features.text().length();
	std::cout << "labels : " << K << std::endl;

	std::vector<size_t> summary = model.shrink();
	const size_t new_M = model.M;

	std::cout << "features : " << org_M << " => " << new_M << std::endl;
	std::cout << "feature text length : " << org_text_len << " => " << model.features.text().length() << std::endl;
	std::cout << "nonzero params";
	size_t nonzeros = 0;
	for (size_t k=0;k<=K;++k) {
		std::cout << " " << k << ":" << summary[k];
		nonzeros += k * summary[k];
	}
	std::cout << std::endl; 
	std::cout << "# of nonzeros : " << nonzeros << std::endl;
	std::cout << "density : " << (LdigFloat)nonzeros / (org_M * K) << " => " << (LdigFloat)nonzeros / (new_M * K) << std::endl;

	saveModel(model, modelpath);
}

size_t extract_label(std::string &label, const std::string &line, const std::unordered_map<std::string, size_t> &labelmap) {
	size_t i = line.find("\t");
	size_t label_k = -1;
	if (i!=std::string::npos) {
		label = line.substr(0, i);
		auto lb = labelmap.find(label);
		if (lb == labelmap.end()) {
			size_t j = line.find("\t", i+1);
			if (j!=std::string::npos) {
				label = line.substr(i+1, j-i-1);
				lb = labelmap.find(label);
			}
		}
		if (lb != labelmap.end()) {
			label_k = lb->second;
		} else {
			label = "";
		}
	}
	return label_k;
}

void ldig_detect(const std::string &modelpath, const std::string &outputpath, const std::vector<std::string> &files, LdigFloat margin) {
	cybozu::ldig::Model model;
	loadModel(model, modelpath);
	const size_t K = model.K;
	std::cout << "labels : " << K << std::endl;
	std::cout << "features : " << model.M << std::endl;

	std::ofstream ofs;
	if (outputpath.length() > 0) {
		ofs.open(outputpath, std::ios::binary);
		if (!ofs.is_open()) {
			throw std::exception("cannot open output file");
		}
		ofs.precision(3);
		ofs << std::fixed;
	}

	char buf[BUFSIZE];

	std::map<size_t, std::map<size_t, size_t> > predicted;
	LdigFloat log_likelihood = 0;
	for (auto i=files.begin(), ie=files.end();i!=ie;++i) {
		std::cout << "loading... " << *i << std::endl;
		std::ifstream ifs(*i);
		if (!ifs.is_open()) {
			throw std::exception("cannot open a test file");
		}

		while (!ifs.eof()) {
			ifs.getline(buf, BUFSIZE);
			std::string line(buf);

			std::string label;
			size_t label_k = extract_label(label, line, model.labelmap);

			size_t i = line.rfind("\t");
			cybozu::String text;
			if (i!=std::string::npos) {
				cybozu::ldig::normalize(text, line.substr(i+1, line.length() - i - 1));
			} else {
				cybozu::ldig::normalize(text, line);
			}
			if (text.length()<=0) continue;
			text = "\x01" + text + "\x01";


			//size_t predict_k = model.predict(y, text);
			cybozu::ldig::Events events;
			model.trie.extract_features(events, text);
			if (margin > 0 && events.size() < 10) continue;

			std::vector<LdigFloat> y(K);
			size_t predict_k = model.predict(y, events);
			if (label_k != -1) {
				const size_t label_k = model.labelmap.at(label);
				predicted[label_k][(y[predict_k] >= 0.6)? predict_k : -1] += 1;
				if (y[label_k] > 0) log_likelihood -= log(y[label_k]);
			}

			const std::string &predict_label = model.label(predict_k);
			LdigFloat score = y[predict_k];
			if (margin > 0) {
				LdigFloat top = 0, second = 0;
				for (auto i=y.begin(), ie=y.end();i!=ie;++i) {
					if (*i>top) {
						second = top;
						top = *i;
					} else if (*i>second) {
						second = *i;
					}
				}
				score = top - second;
				if (score > margin) continue;
			}
			ofs << score << "\t" << predict_label << "\t" << line << std::endl;
		}
		ifs.close();
	}

	size_t cor = 0, sum = 0;
	for (auto k=predicted.begin(), ke=predicted.end();k!=ke;++k) {
		size_t s = 0;
		std::ostringstream buf;
		for (auto j=k->second.begin(), je=k->second.end();j!=je;++j) {
			s += j->second;
			buf << " " << model.label(j->first) << ":" << j->second;
		}
		size_t c = 0;
		auto l = k->second.find(k->first);
		if (l!=k->second.end()) c = l->second;
		std::cout << model.label(k->first) << " " << c << " / " << s << " = " << (LdigFloat)(c) / s << " (" << buf.str() << " )" << std::endl;
		cor += c;
		sum += s;
	}
	if (sum>0) {
		std::cout << "total : " << cor << " / " << sum << " = " << (LdigFloat)cor/sum << ", neg log likelihood " << log_likelihood << std::endl;
	}

}

void ldig_init(const std::string &modelpath, const std::vector<std::string> &files, size_t bound_feature_freq, LdigFloat eta, LdigFloat reg) {
	std::cout << worksize() << std::endl;

	cybozu::ldig::Corpus corpus;
	corpus.load(files);
	std::cout << worksize() << std::endl;

	const cybozu::String& fulltext = corpus.text();
	std::cout << "corpus : " << corpus.size() << std::endl;
	std::cout << " chars : " << fulltext.size() << std::endl;

	const size_t K = corpus.labels().size();
	std::cout << "labels : " << K;

	cybozu::ldig::Model model(K);
	for (auto i=corpus.texts().begin(), iend=corpus.texts().end();i!=iend;++i) {
		std::cout << " " << i->label;
		model.labellist.push_back(i->label);
	}
	std::cout << std::endl;
	model.generate_labelmap();

	model.generate_features(fulltext, bound_feature_freq);

	std::cout << "features : " << model.M << std::endl;
	//for(auto i=features.begin(), ie=features.end();i!=ie;++i) std::cout << i->str(fulltext) << "\t" << i->count << std::endl;

	std::cout << "darray : " << model.trie.size() << std::endl;
	std::cout << worksize() << std::endl;

	time_t t = time(0);
	for(size_t n=0;n<10;++n) {
		model.learn(corpus, eta, (n<5)?0:reg);
		if (n>3) {
			const size_t pre_M = model.M;
			model.shrink();
			std::cout << "fetures : " << pre_M << " => " << model.M << std::endl;
		}
		std::vector<size_t> correct(K);
		LdigFloat lh = model.likelihood(correct, corpus);
		size_t c = 0, s = 0;
		for (size_t k=0;k<K;++k) {
			auto &x = corpus.texts()[k];
			std::cout << x.label << " " << correct[k] << " / " << x.vec.size() << " = " << (LdigFloat)(correct[k]) / x.vec.size() << std::endl;
			c += correct[k];
			s += x.vec.size();
		}
		std::cout << n << " : " << c << " / " << s << " = " << (LdigFloat)c/(LdigFloat)s << ", neg log likelihood " << lh << " (" << time(0) - t << "s)" << std::endl;
		eta *= 0.8;
		std::cout << worksize() << std::endl;
	}

	saveModel(model, modelpath);
}


void ldig_varidation(const std::vector<std::string> &files, const std::string &outputpath, const size_t cvn, const size_t cvt, size_t bound_feature_freq) {
	cybozu::ldig::CorpusFactory validator(cvn, cvt);
	std::cout << worksize() << std::endl;
	validator.load(files);
	std::cout << worksize() << std::endl;
	while (validator.next()) {
		std::cout << "testing ..";
		for (auto i=validator.combination.begin(), ie=validator.combination.end();i!=ie;++i) {
			std::cout << " " << *i;
		}
		std::cout << std::endl;

		const cybozu::ldig::Corpus &corpus = validator.train;
		const size_t K = corpus.labels().size();
		cybozu::ldig::Model model(corpus, bound_feature_freq);

		LdigFloat eta = 0.1;
		for(size_t n=0;n<5;++n) {
			model.learn(corpus, eta);
			eta *= 0.8;
		}

		while (validator.nexttest()) {
			std::vector<LdigFloat> prob;
			size_t predict_k = model.predict(prob, validator.testtext());
			if (model.labellist[predict_k] == validator.testlabel()) ++validator.test().score;
		}
	}

	if (outputpath != "") {
		std::ofstream ofs;
		ofs.open(outputpath, std::ios::binary);
		if (!ofs.is_open()) {
			throw std::exception("cannot open output file");
		}
		ofs.imbue(std::locale("C"));

		validator.sortDataset();
		size_t T = validator.n_test_for_block();
		for (unsigned int score=0;score < T; ++score) {
			validator.output(ofs, score);
		}
	}
}

void maxsubstring(const std::string &input, const std::string &output) {
	std::ifstream ifs(input, std::ios::binary);
	cybozu::String str(std::istreambuf_iterator<char>(ifs.rdbuf()), std::istreambuf_iterator<char>());
	std::cerr << "    chars:" << str.size() << std::endl;

	cybozu::ldig::replace(str, "\n", 1);	// replace \n => \u0001
	cybozu::ldig::replace(str, "\t", 32);	// replace \t => ' '

	cybozu::ldig::Features result;
	result.extract(str, 2);
	std::cerr << "    nodes:" << result.nodesize() << std::endl;
	std::cerr << " maxsubst:" << result.size() << std::endl;

	std::ofstream ofs(output, std::ios::binary);
	for (auto i=result.begin(), ie=result.end();i!=ie;++i) {
		ofs << str.substr(i->begin, i->len) /* << "\t" << i->count */ << std::endl;	}
}




int main(int argc, char* argv[])
#ifndef _DEBUG
	try
#endif
{
	int bound_feature_freq = 5;
	LdigFloat eta = 0.1, reg = 0;
	size_t cvn = 5, cvt = 2;
	MODE mode = detection;
	LdigFloat margin = -1;
	std::string modelpath("ldig.model"), outputpath("");

	std::vector<std::string> files;
	for(int i=1;i<argc;++i) {
		std::string st(argv[i]);

		if (st == "--ff") {
			if (++i>=argc) goto ERROR_OPT_FF;
			bound_feature_freq = atoi(argv[i]);
		} else if (st == "-e") {
			if (++i>=argc) goto ERROR_OPT_E;
			eta = atof(argv[i]);
		} else if (st == "-r") {
			if (++i>=argc) goto ERROR_OPT_R;
			reg = atof(argv[i]);
		} else if (st == "-m") {
			modelpath = argv[++i];
		} else if (st == "-o") {
			outputpath = argv[++i];
		} else if (st == "--init") {
			mode = initialization;
		} else if (st == "--learning") {
			mode = learning;
		} else if (st == "--detection") {
			mode = detection;
		} else if (st == "--shrink") {
			mode = shrink;
		} else if (st == "--maxsubst") {
			mode = maxsubst;
		} else if (st == "--dump") {
			mode = dump;
		} else if (st == "--cv") {
			mode = varidation;
		} else if (st == "--cvn") {
			if (++i>=argc) goto ERROR_OPT_CV;
			cvn = atoi(argv[i]);
		} else if (st == "--cvt") {
			if (++i>=argc) goto ERROR_OPT_CV;
			cvt = atoi(argv[i]);
		} else if (st == "--margin") {
			if (++i>=argc) goto ERROR_OPT_MARGIN;
			margin = atof(argv[i]);
		} else {
			files.push_back(st);
		}
	}

	switch (mode) {
		case initialization:
			ldig_init(modelpath, files, bound_feature_freq, eta, reg);
			break;

		case learning:
			std::cerr << "learning is not implemented yet" << std::endl;
			break;

		case detection:
			ldig_detect(modelpath, outputpath, files, margin);
			break;

		case shrink:
			ldig_shrink(modelpath);
			break;

		case varidation:
			ldig_varidation(files, outputpath, cvn, cvt, bound_feature_freq);
			break;

		case maxsubst:  // for debug
			maxsubstring(files[0], files[1]);
			break;

		case dump:
			ldig_dump(modelpath, outputpath);
			break;
	}

	return 0;



	/* error */

	char *p;
ERROR_OPT_E:
	p = "[ERROR] -e option needs positive real number";
	goto ERROR_EXIT;

ERROR_OPT_R:
	p = "[ERROR] -r option needs positive real number";
	goto ERROR_EXIT;

ERROR_OPT_FF:
	p = "[ERROR] --ff option needs non-negative integer";
	goto ERROR_EXIT;

ERROR_OPT_CV:
	p = "[ERROR] --cvn/cvt option needs positive integer";
	goto ERROR_EXIT;

ERROR_OPT_MARGIN:
	p = "[ERROR] --margin option needs positive integer";
	goto ERROR_EXIT;

ERROR_EXIT:
	std::cerr << p << std::endl;
	return 1;

#ifndef _DEBUG
} catch (std::exception ex) {
	printf("err = %s\n", ex.what());
#endif
}
