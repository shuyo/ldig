#include <iostream>
#include <math.h>
#include "cybozu/test.hpp"
#include "type.hpp"
#include "da.hpp"
#include "util.hpp"
#include "ldig.hpp"

/*
void printevents(const cybozu::ldig::Events &events, const cybozu::ldig::DoubleArray &da, const LdigString &fulltext) {
	for (auto e=events.begin(), ee=events.end();e!=ee;++e) {
		const cybozu::ldig::Feature &f = da.features().at(e->first);
		std::cout << "|" << e->first << ":" << fulltext.substr(f.begin, f.len);
	}
	std::cout << "|" << std::endl;
}
*/

CYBOZU_TEST_AUTO(test_corpus)
{
	cybozu::ldig::Corpus corpus;
	corpus.addText("en", "This is a pen.");
	corpus.addText("en", "It is rainy today.");
	corpus.postLoad();

	const cybozu::String &text = corpus.text();
	CYBOZU_TEST_EQUAL(text, "\x01this is a pen.\x01It is rainy today.\x01");
	const std::vector<cybozu::ldig::TextPos> &texts = corpus.texts()[0].vec;
	CYBOZU_TEST_EQUAL(texts.size(), 2);
	{
		const cybozu::ldig::TextPos &x = texts[0];
		CYBOZU_TEST_EQUAL( text.substr(x.begin, x.len), "\x01this is a pen.\x01");
	}
	{
		const cybozu::ldig::TextPos &x = texts[1];
		CYBOZU_TEST_EQUAL( text.substr(x.begin, x.len), "\x01It is rainy today.\x01");
	}
}

CYBOZU_TEST_AUTO(test_normalization)
{
	cybozu::ldig::Corpus corpus;
	corpus.addText("en", "@ ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_");
	corpus.addText("en", "`abcdefghijklmnopqrstuvwxyz{|}~");
	corpus.addText("en", CYBOZU_RE("ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"));
	corpus.addText("en", CYBOZU_RE("ĀāĂăĄąĆćĈĉĊċČčĎď"));
	corpus.addText("en", CYBOZU_RE("ĐđĒēĔĕĖėĘęĚěĜĝĞğ"));
	corpus.addText("en", CYBOZU_RE("ĠġĢģĤĥĦħĨĩĪīĬĭĮį"));
	corpus.addText("en", CYBOZU_RE("İıĲĳĴĵĶķĸĹĺĻļĽľĿ"));
	corpus.addText("en", CYBOZU_RE("ŀŁłŃńŅņŇňŉŊŋŌōŎŏ"));
	corpus.addText("en", CYBOZU_RE("ŐőŒœŔŕŖŗŘřŚśŜŝŞş"));
	corpus.addText("en", CYBOZU_RE("ŠšŢţŤťŦŧŨũŪūŬŭŮů"));
	corpus.addText("en", CYBOZU_RE("ŰűŲųŴŵŶŷŸŹźŻżŽžſ"));
	corpus.addText("en", CYBOZU_RE("ƠơƯưȘșȚț"));
	corpus.addText("en", CYBOZU_RE("ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩΪ"));
	corpus.addText("en", CYBOZU_RE("АБВГДЕЖЗИЙКЛМНОП"));
	corpus.addText("en", CYBOZU_RE("РСТУФХЦЧШЩЪЫЬЭЮЯ"));
	corpus.addText("en", CYBOZU_RE("ẠạẢảẤấẦầẨẩẪẫẬậẮắ"));
	corpus.addText("en", CYBOZU_RE("ẰằẲẳẴẵẶặẸẹẺẻẼẽẾế"));
	corpus.addText("en", CYBOZU_RE("ỀềỂểỄễỆệỈỉỊịỌọỎỏ"));
	corpus.addText("en", CYBOZU_RE("ỐốỒồỔổỖỗỘộỚớỜờỞở"));
	corpus.addText("en", CYBOZU_RE("ỠỡỢợỤụỦủỨứỪừỬửỮữ"));
	corpus.addText("en", CYBOZU_RE("ỰựỲỳỴỵỶỷỸỹ"));
	corpus.addText("en", CYBOZU_RE("A\u0300A\u0301A\u0303A\u0309A\u0323E\u0300E\u0301E\u0303E\u0309E\u0323I\u0300I\u0301I\u0303I\u0309I\u0323"));
	corpus.addText("en", CYBOZU_RE("O\u0300O\u0301O\u0303O\u0309O\u0323U\u0300U\u0301U\u0303U\u0309U\u0323Y\u0300Y\u0301Y\u0303Y\u0309Y\u0323"));
	corpus.addText("en", CYBOZU_RE("a\u0300a\u0301a\u0303a\u0309a\u0323e\u0300e\u0301e\u0303e\u0309e\u0323i\u0300i\u0301i\u0303i\u0309i\u0323"));
	corpus.addText("en", CYBOZU_RE("o\u0300o\u0301o\u0303o\u0309o\u0323u\u0300u\u0301u\u0303u\u0309u\u0323y\u0300y\u0301y\u0303y\u0309y\u0323"));
	corpus.addText("en", CYBOZU_RE("\xc2\u0300\xc2\u0301\xc2\u0303\xc2\u0309\xc2\u0323\xca\u0300\xca\u0301\xca\u0303\xca\u0309\xca\u0323\xd4\u0300\xd4\u0301\xd4\u0303\xd4\u0309\xd4\u0323"));
	corpus.addText("en", CYBOZU_RE("\xe2\u0300\xe2\u0301\xe2\u0303\xe2\u0309\xe2\u0323\xea\u0300\xea\u0301\xea\u0303\xea\u0309\xea\u0323\xf4\u0300\xf4\u0301\xf4\u0303\xf4\u0309\xf4\u0323"));
	corpus.addText("en", CYBOZU_RE("\u0102\u0300\u0102\u0301\u0102\u0303\u0102\u0309\u0102\u0323\u0103\u0300\u0103\u0301\u0103\u0303\u0103\u0309\u0103\u0323\u01a0\u0300\u01a0\u0301\u01a0\u0303\u01a0\u0309\u01a0\u0323"));
	corpus.addText("en", CYBOZU_RE("\u01a1\u0300\u01a1\u0301\u01a1\u0303\u01a1\u0309\u01a1\u0323\u01af\u0300\u01af\u0301\u01af\u0303\u01af\u0309\u01af\u0323\u01b0\u0300\u01b0\u0301\u01b0\u0303\u01b0\u0309\u01b0\u0323"));

	corpus.addText("en", "ahahahah");
	corpus.addText("en", "hahha");
	corpus.addText("en", "hahaa");
	corpus.addText("en", "ahahahahhahahhahahaaaa");
	corpus.addText("en", "jajjajajaja");
	corpus.addText("en", "Jejeje");
	corpus.addText("en", "Bin dia!!! :-)");
	corpus.postLoad();

	const cybozu::String &text = corpus.text();
	const std::vector<cybozu::ldig::TextPos> &texts = corpus.texts()[0].vec;
	auto checkstr = [&](size_t index, const cybozu::String &st) {
		const cybozu::ldig::TextPos &x = texts[index];
		CYBOZU_TEST_EQUAL( text.substr(x.begin, x.len), st);
	};

	checkstr(0, "\x01@ abcdefghIjklmnopqrstuvwxyz[\\]^_\x01");  // I isn't lowerized for Turkish
	checkstr(1, CYBOZU_RE("\x01`abcdefghijklmnopqrstuvwxyz{|}~\x01"));
	checkstr(2, CYBOZU_RE("\x01àáâãäåæçèéêëìíîïðñòóôõö×øùúûüýþß\x01"));
	checkstr(3, CYBOZU_RE("\x01āāăăąąććĉĉċċččďď\x01"));
	checkstr(4, CYBOZU_RE("\x01đđēēĕĕėėęęěěĝĝğğ\x01"));
	checkstr(5, CYBOZU_RE("\x01ġġģģĥĥħħĩĩīīĭĭįį\x01"));
	checkstr(6, CYBOZU_RE("\x01İıĳĳĵĵķķĸĺĺļļľľŀ\x01"));  // İ isn't lowerized for Turkish
	checkstr(7, CYBOZU_RE("\x01ŀłłńńņņňňŉŋŋōōŏŏ\x01"));
	checkstr(8, CYBOZU_RE("\x01őőœœŕŕŗŗřřśśŝŝşş\x01"));  // U+017f is nomalized to 's'
	checkstr(9, CYBOZU_RE("\x01ššţţťťŧŧũũūūŭŭůů\x01"));
	checkstr(10, CYBOZU_RE("\x01űűųųŵŵŷŷÿźźżżžžs\x01"));
	checkstr(11, CYBOZU_RE("\x01ơơưưşşţţ\x01"));  // for Romanian
	checkstr(12, CYBOZU_RE("\x01αβγδεζηθικλμνξοπρστυφχψωϊ\x01"));
	checkstr(13, CYBOZU_RE("\x01абвгдежзийклмноп\x01"));
	checkstr(14, CYBOZU_RE("\x01рстуфхцчшщъыьэюя\x01"));
	checkstr(15, CYBOZU_RE("\x01ạạảảấấầầẩẩẫẫậậắắ\x01"));
	checkstr(16, CYBOZU_RE("\x01ằằẳẳẵẵặặẹẹẻẻẽẽếế\x01"));
	checkstr(17, CYBOZU_RE("\x01ềềểểễễệệỉỉịịọọỏỏ\x01"));
	checkstr(18, CYBOZU_RE("\x01ốốồồổổỗỗộộớớờờởở\x01"));
	checkstr(19, CYBOZU_RE("\x01ỡỡợợụụủủứứừừửửữữ\x01"));
	checkstr(20, CYBOZU_RE("\x01ựựỳỳỵỵỷỷỹỹ\x01"));
	checkstr(21, CYBOZU_RE("\x01àáãảạèéẽẻẹìíĩỉị\x01"));
	checkstr(22, CYBOZU_RE("\x01òóõỏọùúũủụỳýỹỷỵ\x01"));
	checkstr(23, CYBOZU_RE("\x01àáãảạèéẽẻẹìíĩỉị\x01"));
	checkstr(24, CYBOZU_RE("\x01òóõỏọùúũủụỳýỹỷỵ\x01"));
	checkstr(25, CYBOZU_RE("\x01ầấẫẩậềếễểệồốỗổộ\x01"));
	checkstr(26, CYBOZU_RE("\x01ầấẫẩậềếễểệồốỗổộ\x01"));
	checkstr(27, CYBOZU_RE("\x01ằắẵẳặằắẵẳặờớỡởợ\x01"));
	checkstr(28, CYBOZU_RE("\x01ờớỡởợừứữửựừứữửự\x01"));

	checkstr(29, "\x01" "ahahah\x01");
	checkstr(30, CYBOZU_RE("\x01haha\x01"));
	checkstr(31, CYBOZU_RE("\x01haha\x01"));
	checkstr(32, "\x01" "ahaha\x01");
	checkstr(33, CYBOZU_RE("\x01jaja\x01"));
	checkstr(34, CYBOZU_RE("\x01jeje\x01"));
	checkstr(35, "\x01" "bin dia!! \x01");

	auto assert_normalize = [](const cybozu::String &in, const cybozu::String &out) {
		cybozu::String s;
		cybozu::ldig::normalize(s, in);
		CYBOZU_TEST_EQUAL(s, out);
	};
	cybozu::String s, s1(CYBOZU_RE("ѐёѓєѕіїјљњќџґ"));
	cybozu::ldig::normalize(s, CYBOZU_RE("ЀЁЃЄЅІЇЈЉЊЌЏҐ"));
	CYBOZU_TEST_EQUAL(s, s1);
	cybozu::ldig::normalize(s, s1);
	CYBOZU_TEST_EQUAL(s, s1);

	assert_normalize(CYBOZU_RE("ČŠŽčšž"), CYBOZU_RE("čšžčšž"));
}


CYBOZU_TEST_AUTO(test_model1)
{
	const size_t K = 2;
	cybozu::ldig::Model model(K);

	model.features.settext(LdigString("catdog"));
	model.features.push_back(cybozu::ldig::Feature(0,3,1));
	model.features.push_back(cybozu::ldig::Feature(3,3,1));
	model.post_features();

	CYBOZU_TEST_EQUAL(model.K, K);
	CYBOZU_TEST_EQUAL(model.M, 2);

	cybozu::ldig::Events events;
	events[0] = 1;
	std::vector<LdigFloat> prob(K);
	model.predict(prob, events);
	CYBOZU_TEST_EQUAL(prob.size(), K);
	CYBOZU_TEST_NEAR(prob[0], 0.5, 1e-7);
	CYBOZU_TEST_NEAR(prob[1], 0.5, 1e-7);

	model.params[0] = 0.1;
	prob.clear();
	model.predict(prob, events);
	CYBOZU_TEST_EQUAL(prob.size(), K);
	CYBOZU_TEST_NEAR(prob[0]+prob[1], 1.0, 1e-7);
	CYBOZU_TEST_NEAR(prob[0], exp(0.1)/(exp(0.1)+1), 1e-7); // 0.52497918747
	CYBOZU_TEST_NEAR(prob[1], 1.0/(exp(0.1)+1), 1e-7);      // 0.47502081252

	cybozu::ldig::Events events1;
	events1[1] = 1;
	model.update(events1, 1, 0.1);
	CYBOZU_TEST_NEAR(model.params[0], 0.1, 1e-7);
	CYBOZU_TEST_NEAR(model.params[1], 0.0, 1e-7);
	CYBOZU_TEST_NEAR(model.params[2], -0.05, 1e-7);
	CYBOZU_TEST_NEAR(model.params[3], 0.05, 1e-7);

	prob.clear();
	model.predict(prob, events1);

	CYBOZU_TEST_NEAR(prob[0]+prob[1], 1.0, 1e-7);
	CYBOZU_TEST_NEAR(prob[0], exp(-0.05)/(exp(-0.05)+exp(0.05)), 1e-7);
	CYBOZU_TEST_NEAR(prob[1], exp( 0.05)/(exp(-0.05)+exp(0.05)), 1e-7);
}

CYBOZU_TEST_AUTO(test_model1a)
{
	const size_t K = 3;


	cybozu::ldig::Model model(K);
	model.features.settext(LdigString("catdog"));
	model.features.push_back(cybozu::ldig::Feature(1,1,1)); // a
	model.features.push_back(cybozu::ldig::Feature(1,2,1)); // at
	model.features.push_back(cybozu::ldig::Feature(0,1,1)); // c
	model.features.push_back(cybozu::ldig::Feature(0,2,1)); // ca
	model.features.push_back(cybozu::ldig::Feature(0,3,1)); // cat
	model.features.push_back(cybozu::ldig::Feature(3,1,1)); // d
	model.features.push_back(cybozu::ldig::Feature(3,3,1)); // dog
	model.features.push_back(cybozu::ldig::Feature(5,1,1)); // g
	model.features.push_back(cybozu::ldig::Feature(4,1,1)); // o
	model.features.push_back(cybozu::ldig::Feature(2,1,1)); // t
	model.post_features();
	
	CYBOZU_TEST_EQUAL(model.K, K);
	CYBOZU_TEST_EQUAL(model.M, 10);

	cybozu::ldig::Events events;
	events[0] = 1;
	events[1] = 2;
	events[2] = 1;
	std::vector<LdigFloat> prob(K);
	model.predict(prob, events);
	CYBOZU_TEST_EQUAL(prob.size(), K);
	CYBOZU_TEST_NEAR(prob[0], 0.3333333, 1e-7);
	CYBOZU_TEST_NEAR(prob[1], 0.3333333, 1e-7);
	CYBOZU_TEST_NEAR(prob[2], 0.3333333, 1e-7);

	model.update(events, 1, 0.1);
	//cybozu::ldig::printvec(model.params);

	prob.clear();
	size_t predict_k = model.predict(prob, events);

	CYBOZU_TEST_EQUAL(predict_k, 1);
	CYBOZU_TEST_NEAR(prob[0]+prob[1]+prob[2], 1.0, 1e-7);
	CYBOZU_TEST_NEAR(prob[0], 0.26163499, 1e-7);
	CYBOZU_TEST_NEAR(prob[1], 0.47673003, 1e-7);
	CYBOZU_TEST_NEAR(prob[2], 0.26163499, 1e-7);
}


CYBOZU_TEST_AUTO(test_model2)
{
	cybozu::ldig::Corpus corpus;
	corpus.addText("en", "This is a pen.");
	corpus.addText("en", "It is rainy today.");
	corpus.addText("en", "Good morning! Have a nice day!");
	corpus.addText("it", "Come ti va la vita?");
	corpus.addText("it", "Molto bene, bravi tutti.");
	corpus.addText("it", "Grazie mille allora!");
	corpus.postLoad();


	auto& fulltext = corpus.text();
	auto& texts = corpus.texts();

	const size_t K = 2;
	cybozu::ldig::Model model(K);
	model.generate_features(fulltext, 2);
	model.labelmap = corpus.labels();
	model.generate_labellist();

	{
		std::vector<size_t> correct;
		CYBOZU_TEST_NEAR(model.likelihood(correct, corpus), 6 * log(2.0), 1e-7);
	}

	CYBOZU_TEST_EQUAL(texts[0].label, "en");

	auto testOneEvents = [&texts, &fulltext, &model](const size_t k, const size_t j) {
		auto &pos = texts[k].vec[j];
		cybozu::ldig::Events events;
		model.trie.extract_features(events, fulltext, pos);
		//CYBOZU_TEST_EQUAL(events.size(), 10);

		std::vector<LdigFloat> prob1;
		model.predict(prob1, events);

		//printevents(events, da, fulltext);
		model.update(events, k, 0.1);

		std::vector<LdigFloat> prob2;
		model.predict(prob2, events);

		//std::cout << prob2[k] << " > " << prob1[k] << std::endl;
		CYBOZU_TEST_ASSERT(1.0-prob1[k]<1e-7 || prob2[k] > prob1[k]);

	};

	for (size_t i=0;i<10;++i) {
	testOneEvents(0,0);
	testOneEvents(1,0);
	testOneEvents(0,1);
	testOneEvents(1,1);
	testOneEvents(0,2);
	testOneEvents(1,2);
	
	std::vector<size_t> correct;
	//std::cout << model.likelihood(correct, corpus) << std::endl;
	}
}
