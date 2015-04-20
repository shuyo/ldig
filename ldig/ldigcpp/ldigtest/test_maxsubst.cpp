#include <unordered_map>
#include "cybozu/test.hpp"
#include <cybozu/regex.hpp>

#include "da.hpp"
#include "util.hpp"

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
	CYBOZU_TEST_EQUAL(features[0].count, 5);
	CYBOZU_TEST_EQUAL(features.str(1), "abra");
	CYBOZU_TEST_EQUAL(features[1].count, 2);

	// shrink features
	cybozu::ldig::Features new_features(features);
	CYBOZU_TEST_EQUAL(new_features.nodesize(), 0);
	CYBOZU_TEST_EQUAL(new_features.size(), 2);
	CYBOZU_TEST_EQUAL(new_features.text(), "abra");
	CYBOZU_TEST_EQUAL(new_features.str(0), "a");
	CYBOZU_TEST_EQUAL(new_features[0].count, 5);
	CYBOZU_TEST_EQUAL(new_features.str(1), "abra");
	CYBOZU_TEST_EQUAL(new_features[1].count, 2);



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
