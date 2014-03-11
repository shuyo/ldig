#define CYBOZU_TEST_DISABLE_AUTO_RUN
inline void worksize() {}

#include <iostream>
#ifdef __linux__
#include <boost/regex.hpp>
namespace std {
    using boost::regex;
    using boost::regex_match;
    using boost::regex_search;
    using boost::regex_iterator;
    using boost::match_results;
    using boost::smatch;
}
#else
#include <regex>
#endif
#include "cybozu/regex.hpp"
#include "cybozu/test.hpp"
#include "da.hpp"
#include "util.hpp"

typedef cybozu::ldig::Feature Feature;

CYBOZU_TEST_AUTO(test_doublearray1)
{
	cybozu::String st("cat");
	std::vector<Feature> list;
	list.push_back(Feature(0,3,1));
	cybozu::ldig::Features features(st, list);
	cybozu::ldig::DoubleArray da(features);
	CYBOZU_TEST_EQUAL(da.size(), 4);
	CYBOZU_TEST_EQUAL(da.get("cat"), 0);
	CYBOZU_TEST_EQUAL(da.get("ca"), -1);
	CYBOZU_TEST_EQUAL(da.get(""), -1);
	CYBOZU_TEST_EQUAL(da.get("catt"), -1);
	CYBOZU_TEST_EQUAL(da.get("xxx"), -1);
}

CYBOZU_TEST_AUTO(test_doublearray2)
{
	cybozu::String st("catdog");
	std::vector<Feature> list;
	list.push_back(Feature(0,3,1));
	list.push_back(Feature(3,3,1));
	cybozu::ldig::Features features(st, list);
	cybozu::ldig::DoubleArray da(features);

	CYBOZU_TEST_EQUAL(da.size(), 7);
	CYBOZU_TEST_EQUAL(da.get("cat"), 0);
	CYBOZU_TEST_EQUAL(da.get("dog"), 1);
	CYBOZU_TEST_EQUAL(da.get(""), -1);
	CYBOZU_TEST_EQUAL(da.get("catt"), -1);
	CYBOZU_TEST_EQUAL(da.get("xxx"), -1);
}

CYBOZU_TEST_AUTO(test_doublearray3)
{
	cybozu::String st("catdogdeerfoxrat");
	std::vector<Feature> list;
	list.push_back(Feature(0,2,1));	//ca
	list.push_back(Feature(0,3,1));	//cat
	list.push_back(Feature(6,4,1)); //deer
	list.push_back(Feature(3,3,1)); //dog
	list.push_back(Feature(10,3,1)); //fox
	list.push_back(Feature(13,3,1)); //rat
	cybozu::ldig::Features features(st, list);
	cybozu::ldig::DoubleArray da(features);

	CYBOZU_TEST_EQUAL(da.size(), 17);
	CYBOZU_TEST_EQUAL(da.get("ca"), 0);
	CYBOZU_TEST_EQUAL(da.get("cat"), 1);
	CYBOZU_TEST_EQUAL(da.get("deer"), 2);
	CYBOZU_TEST_EQUAL(da.get("dog"), 3);
	CYBOZU_TEST_EQUAL(da.get("fox"), 4);
	CYBOZU_TEST_EQUAL(da.get("rat"), 5);

	CYBOZU_TEST_EQUAL(da.get(""), -1);
	CYBOZU_TEST_EQUAL(da.get("catt"), -1);
	CYBOZU_TEST_EQUAL(da.get("xxx"), -1);

	{
		cybozu::ldig::Events r;
		da.extract_features(r, "cat");
		CYBOZU_TEST_EQUAL(r.size(), 2);
		CYBOZU_TEST_EQUAL(r[0], 1);
		CYBOZU_TEST_EQUAL(r[1], 1);
	}

	{
		cybozu::ldig::Events r;
		da.extract_features(r, "deerat");
		CYBOZU_TEST_EQUAL(r.size(), 2);
		CYBOZU_TEST_EQUAL(r[2], 1);
		CYBOZU_TEST_EQUAL(r[5], 1);
	}

	{
		cybozu::ldig::Events r;
		da.extract_features(r, "abcdef");
		CYBOZU_TEST_EQUAL(r.size(), 0);
	}
}

CYBOZU_TEST_AUTO(test_doublearray4)
{
	cybozu::String st(" pt");
	std::vector<Feature> list;
	list.push_back(Feature(0,1,1));	//' '
	list.push_back(Feature(0,2,1));	// p
	list.push_back(Feature(1,1,1)); //p
	list.push_back(Feature(2,1,1)); //t

	CYBOZU_TEST_EQUAL(list[0].str(st), " ");
	CYBOZU_TEST_EQUAL(list[1].str(st), " p");
	CYBOZU_TEST_EQUAL(list[2].str(st), "p");
	CYBOZU_TEST_EQUAL(list[3].str(st), "t");

	cybozu::ldig::Features features(st, list);
	cybozu::ldig::DoubleArray da(features);
	//cybozu::ldig::printvec(da.base);
	//cybozu::ldig::printvec(da.check);
	//cybozu::ldig::printvec(da.value);

	CYBOZU_TEST_EQUAL(da.size(), 86);
	CYBOZU_TEST_EQUAL(da.get(" "), 0);
	CYBOZU_TEST_EQUAL(da.get(" p"), 1);
	CYBOZU_TEST_EQUAL(da.get("p"), 2);
	CYBOZU_TEST_EQUAL(da.get("t"), 3);
	CYBOZU_TEST_EQUAL(da.get("ca"), -1);
	CYBOZU_TEST_EQUAL(da.get(""), -1);
	CYBOZU_TEST_EQUAL(da.get("catt"), -1);
	CYBOZU_TEST_EQUAL(da.get("xxx"), -1);
}


CYBOZU_TEST_AUTO(test_abracadabra)
{
	cybozu::String st("abracadabra");
	cybozu::ldig::Features features(st, 2);

/*
	printvec(ex.SA);
	printvec(ex.L);
	printvec(ex.R);
	printvec(ex.D);
	printvec(ex.rank);
	CYBOZU_TEST_EQUAL(ex.len, 11);
*/
	//printvec(features);
	CYBOZU_TEST_EQUAL(features.nodesize(), 5);
	CYBOZU_TEST_EQUAL(features.size(), 2);
	CYBOZU_TEST_EQUAL(features.str(0), "a");
	CYBOZU_TEST_EQUAL(features[0].begin, 10);
	CYBOZU_TEST_EQUAL(features[0].len, 1);
	//CYBOZU_TEST_EQUAL(features[0].count, 5);
	CYBOZU_TEST_EQUAL(features.str(1), "abra");
	CYBOZU_TEST_EQUAL(features[1].begin, 7);
	CYBOZU_TEST_EQUAL(features[1].len, 4);
	//CYBOZU_TEST_EQUAL(features[1].count, 2);

	// shrink features
	cybozu::ldig::Features new_features(features);
	CYBOZU_TEST_EQUAL(new_features.nodesize(), 0);
	CYBOZU_TEST_EQUAL(new_features.size(), 2);
	CYBOZU_TEST_EQUAL(new_features.text(), "abra");
	CYBOZU_TEST_EQUAL(new_features.str(0), "a");
	//CYBOZU_TEST_EQUAL(new_features[0].count, 5);
	CYBOZU_TEST_EQUAL(new_features.str(1), "abra");
	//CYBOZU_TEST_EQUAL(new_features[1].count, 2);



}

CYBOZU_TEST_AUTO(test_shrink1)
{
	cybozu::String st("abracadabra");
	std::vector<Feature> list;
	list.push_back(Feature(10,1,1));
	list.push_back(Feature(7,4,1));
	cybozu::ldig::Features features(st, list);

	CYBOZU_TEST_EQUAL(features[0].begin, 10);
	CYBOZU_TEST_EQUAL(features[0].len, 1);
	CYBOZU_TEST_EQUAL(features[1].begin, 7);
	CYBOZU_TEST_EQUAL(features[1].len, 4);

	features.shrink();
	CYBOZU_TEST_EQUAL(features.size(), 2);
	CYBOZU_TEST_EQUAL(features.text(), "abra");
	CYBOZU_TEST_EQUAL(features[0].begin, 3);
	CYBOZU_TEST_EQUAL(features[0].len, 1);
	CYBOZU_TEST_EQUAL(features[1].begin, 0);
	CYBOZU_TEST_EQUAL(features[1].len, 4);
}

CYBOZU_TEST_AUTO(test_shrink2)
{
	cybozu::String st("abracadabraxy");
	std::vector<Feature> list;
	list.push_back(Feature(7,4,1));
	list.push_back(Feature(12,1,1));
	cybozu::ldig::Features features(st, list);

	features.shrink();
	CYBOZU_TEST_EQUAL(features.size(), 2);
	CYBOZU_TEST_EQUAL(features.text(), "abray");
	CYBOZU_TEST_EQUAL(features[0].begin, 0);
	CYBOZU_TEST_EQUAL(features[0].len, 4);
	CYBOZU_TEST_EQUAL(features[1].begin, 4);
	CYBOZU_TEST_EQUAL(features[1].len, 1);
}

CYBOZU_TEST_AUTO(test_shrink3)
{
	cybozu::String st("abracadabraay");
	std::vector<Feature> list;
	list.push_back(Feature(10,3,1));
	list.push_back(Feature(7,4,1));
	cybozu::ldig::Features features(st, list);

	features.shrink();
	CYBOZU_TEST_EQUAL(features.size(), 2);
	CYBOZU_TEST_EQUAL(features.text(), "abraay");
	CYBOZU_TEST_EQUAL(features[0].begin, 3);
	CYBOZU_TEST_EQUAL(features[0].len, 3);
	CYBOZU_TEST_EQUAL(features[1].begin, 0);
	CYBOZU_TEST_EQUAL(features[1].len, 4);
}

CYBOZU_TEST_AUTO(test_long)
{
	cybozu::String st(CYBOZU_RE("Felicitar al cap de seguretat x fer cada dia més difícil l'accès al Camp Nou.Ès + fàcil arribar a una roda de premsa a T ..."));
	cybozu::ldig::Features features(st, 2);

	CYBOZU_TEST_EQUAL(features.nodesize(), 46);
	CYBOZU_TEST_EQUAL(features.size(), 34);
	CYBOZU_TEST_EQUAL(features.str(0), " a");
	CYBOZU_TEST_EQUAL(features.str(1), " a ");
	CYBOZU_TEST_EQUAL(features.str(2), " al ");
	CYBOZU_TEST_EQUAL(features.str(3), " ca");
	CYBOZU_TEST_EQUAL(features.str(4), " d");
	CYBOZU_TEST_EQUAL(features.str(5), " de ");
	CYBOZU_TEST_EQUAL(features.str(6), " di");
	CYBOZU_TEST_EQUAL(features.str(7), " f");

	cybozu::ldig::Features new_features(features);
	CYBOZU_TEST_EQUAL(new_features.text(), " al  de cil ar ada d a  ca di fp res tamou");
	CYBOZU_TEST_EQUAL(new_features.str(0), " a");
	CYBOZU_TEST_EQUAL(new_features.str(1), " a ");
	CYBOZU_TEST_EQUAL(new_features.str(2), " al ");
	CYBOZU_TEST_EQUAL(new_features.str(3), " ca");
	CYBOZU_TEST_EQUAL(new_features.str(4), " d");
	CYBOZU_TEST_EQUAL(new_features.str(5), " de ");
	CYBOZU_TEST_EQUAL(new_features.str(6), " di");
	CYBOZU_TEST_EQUAL(new_features.str(7), " f");

	features.shrink();
	CYBOZU_TEST_EQUAL(features.text(), "tat f ca di al mp us cil ar aoda de re a ");
	CYBOZU_TEST_EQUAL(features.str(0), " a");
	CYBOZU_TEST_EQUAL(features.str(1), " a ");
	CYBOZU_TEST_EQUAL(features.str(2), " al ");
	CYBOZU_TEST_EQUAL(features.str(3), " ca");
	CYBOZU_TEST_EQUAL(features.str(4), " d");
	CYBOZU_TEST_EQUAL(features.str(5), " de ");
	CYBOZU_TEST_EQUAL(features.str(6), " di");
	CYBOZU_TEST_EQUAL(features.str(7), " f");
}




CYBOZU_TEST_AUTO(test_regex)
{
	std::regex re("^(\\S+)(\\t[^\\t\\n]+)*\\t([^\\t\\n]+)$");
	std::string st("en\tThis is a pen.\nen\thoge\tfuga\tI have no idea.\n");
	std::regex_iterator<std::string::const_iterator> i( st.begin(), st.end(), re ), iend;
	CYBOZU_TEST_EQUAL((*i)[1].str(), "en");
	CYBOZU_TEST_EQUAL((*i)[3].str(), "This is a pen.");
	++i;
	CYBOZU_TEST_EQUAL((*i)[1].str(), "en");
	CYBOZU_TEST_EQUAL((*i)[3].str(), "I have no idea.");
	++i;
	CYBOZU_TEST_ASSERT(i == iend);

}

CYBOZU_TEST_AUTO(test_cybozu_regex)
{
	cybozu::regex r("h([aiueo])(h\\1)+");
	const cybozu::String input("haha hihi hahu");
	const cybozu::String fmt("x");
	cybozu::String s;
	s =	cybozu::regex_replace(input, r, fmt);
	CYBOZU_TEST_EQUAL(s, "x x hahu");
}
