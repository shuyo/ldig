#pragma once
/**
	@file
	@brief unicode string class like std::string
	support char*, std::string with UTF-8

	Copyright (C) 2008-2012 Cybozu Labs, Inc., all rights reserved.
*/

#include <string>
#include <cstring>
#include <assert.h>
#include <stddef.h>
#include <stdio.h> // for printf
#include <iosfwd> // for istream, ostream
#include <functional> // for unary_function

#include <cybozu/exception.hpp>

namespace cybozu {

struct StringException : public cybozu::Exception {
	StringException() : cybozu::Exception("string") { }
};

#ifdef __GNUC__
	/* avoid to use uint32_t because compiling boost::regex fails */
	typedef wchar_t Char; //!< Char for Linux
	typedef unsigned short Char16; /* unsigned is necessary for gcc */
#else
	/* can't compile with singed */
	typedef unsigned __int32 Char; //!< Char for Windows
	typedef wchar_t Char16;
#endif

typedef std::basic_string<Char16> String16;

/**
	utility function
*/
namespace string {

/*
	 code point[a, b] 1byte  2ybte  3byte  4byte
	  U+0000   U+007f 00..7f                      ; 128
	  U+0080   U+07ff c2..df 80..bf               ; 30 x 64 = 1920

	  U+0800   U+0fff e0     a0..bf 80..bf        ;  1 x 32 x 64 = 2048
	  U+1000   U+cfff e1..ec 80..bf 80..bf        ; 12 x 64 x 64 = 49152
	  U+d000   U+d7ff ed     80..9f 80..bf        ;  1 x 32 x 64 = 2048

	  U+e000   U+ffff ee..ef 80..bf 80..bf        ;  2 x 64 x 64 = 8192

	 U+10000  U+3ffff f0     90..bf 80..bf 80..bf ;  1 x 48 x 64 x 64 = 196608
	 U+40000  U+fffff f1..f3 80..bf 80..bf 80..bf ;  3 x 64 x 64 x 64 = 786432
	U+100000 U+10ffff f4     80..8f 80..bf 80..bf ;  1 x 16 x 64 x 64 = 65536
*/
inline int GetCharSize(Char c)
{
	if (c <= 0x7f) return 1;
	if (c <= 0x7ff) return 2;
	if (c <= 0xd7ff) return 3;
	if (c <= 0xdfff || c > 0x10ffff) return 0;
	if (c <= 0xffff) return 3;
	return 4;
}

// for Char/char
inline bool IsValidChar(Char c)
{
	return GetCharSize(c) != 0;
}

namespace local {

/* true if c in [min, max] */
inline bool in(unsigned char c, int min, int max)
{
//	  return min <= c && c <= max;
	return static_cast<unsigned int>(c - min) <= static_cast<unsigned int>(max - min);
}

} // local

/*
	get one character from UTF-8 string and seek begin to next char
	@note begin != end
	@note begin is not determined if false
*/
template<class Iterator>
bool GetCharFromUtf8(Char *c, Iterator& begin, const Iterator& end)
{
	unsigned char c0 = *begin++;
	if (c0 <= 0x7f) {
		*c = c0;
		return true;
	}
	if (local::in(c0, 0xc2, 0xdf)) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (local::in(c1, 0x80, 0xbf)) {
				*c = ((c0 << 6) | (c1 & 0x3f)) - 0x3000;
				return true;
			}
		}
	} else if (c0 <= 0xef) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (begin != end) {
				unsigned char c2 = *begin++;
				if (local::in(c2, 0x80, 0xbf)) {
					if ((c0 == 0xe0 && local::in(c1, 0xa0, 0xbf))
					 || (local::in(c0, 0xe1, 0xec) && local::in(c1, 0x80, 0xbf))
					 || (c0 == 0xed && local::in(c1, 0x80, 0x9f))
					 || (local::in(c0, 0xee, 0xef) && local::in(c1, 0x80, 0xbf))) {
						*c = ((c0 << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f)) - 0xe0000;
						return true;
					}
				}
			}
		}
	} else if (local::in(c0, 0xf0, 0xf4)) {
		if (begin != end) {
			unsigned char c1 = *begin++;
			if (begin != end) {
				unsigned char c2 = *begin++;
				if (begin != end) {
					unsigned char c3 = *begin++;
					if (local::in(c2, 0x80, 0xbf) && local::in(c3, 0x80, 0xbf)) {
						if ((c0 == 0xf0 && local::in(c1, 0x90, 0xbf))
						 || (local::in(c0, 0xf1, 0xf3) && local::in(c1, 0x80, 0xbf))
						 || (c0 == 0xf4 && local::in(c1, 0x80, 0x8f))) {
							*c = ((c0 << 18) | ((c1 & 0x3f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f)) - 0x3c00000;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/*
	get one character from UTF-16 string and seek begin to next char
	@note begin != end
	@note begin is not determined if false
*/
template<class Iterator>
bool GetCharFromUtf16(Char& c, Iterator& begin, const Iterator& end)
{
	struct local {
		static inline bool isLead(Char c) { return (c & 0xfffffc00) == 0xd800; }
		static inline bool isTrail(Char c) { return (c & 0xfffffc00) == 0xdc00; }
	};
	Char16 c0 = *begin++;
	if (!local::isLead(c0)) {
		c = c0;
		return true;
	}
	if (begin != end) {
		Char16 c1 = *begin++;
		if (local::isTrail(c1)) {
			const Char offset = (0xd800 << 10UL) + 0xdc00 - 0x10000;
			c = (c0 << 10) + c1 - offset;
			return true;
		}
	}
	return false;
}

inline bool AppendUtf8(std::string& out, Char c)
{
	if (c <= 0x7f) {
		out += static_cast<char>(c);
		return true;
	} else if (c <= 0x7ff) {
		char buf[2];
		buf[0] = static_cast<char>((c >> 6) | 0xc0);
		buf[1] = static_cast<char>((c & 0x3f) | 0x80);
		out.append(buf, 2);
		return true;
	} else if (c <= 0xffff) {
		if (0xd7ff < c && c <= 0xdfff) {
			return false;
		}
		char buf[3];
		buf[0] = static_cast<char>((c >> 12) | 0xe0);
		buf[1] = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
		buf[2] = static_cast<char>((c & 0x3f) | 0x80);
		out.append(buf, 3);
		return true;
	} else if (c <= 0x10ffff) {
		char buf[4];
		buf[0] = static_cast<char>((c >> 18) | 0xf0);
		buf[1] = static_cast<char>(((c >> 12) & 0x3f) | 0x80);
		buf[2] = static_cast<char>(((c >> 6) & 0x3f) | 0x80);
		buf[3] = static_cast<char>((c & 0x3f) | 0x80);
		out.append(buf, 4);
		return true;
	}
	return false;
}

inline bool AppendUtf16(String16 *out, Char c)
{
	if (c <= 0xffff) {
		*out += static_cast<Char16>(c);
		return true;
	} else if (c <= 0x0010ffff) {
		Char16 buf[2];
		buf[0] = static_cast<Char16>((c >> 10) + 0xd7c0);
		buf[1] = static_cast<Char16>((c & 0x3ff) | 0xdc00);
		out->append(buf, 2);
		return true;
	}
	return false;
}

} // string

/**
	@brief template class for cybozu::String
*/
template<class CharT, class Traits = std::char_traits<CharT>, class Alloc = std::allocator<CharT> >
class StringT {
public:
	//@{ standard typedef
	typedef std::basic_string<CharT, Traits, Alloc> BasicString;
	typedef CharT value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef CharT& reference;
	typedef const CharT& const_reference;
	typedef CharT* pointer;
	typedef const CharT* const_pointer;
	typedef typename BasicString::iterator iterator;
	typedef typename BasicString::const_iterator const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	//@}
	static const size_t npos = size_t(-1); //!< standard npos

	/**
		dump unicode of string for debug
		@param msg [local::in] user message
	*/
	void dump(const char *msg = 0) const
	{
		if (msg) printf("%s", msg);
		for (size_t i = 0; i < size(); i++) {
			printf("%08x ", str_[i]);
		}
		printf("\n");
	}

	/**
		construct empty string
	*/
	StringT() { }

	/**
		construct from str [off, off + count)
		@param str [local::in] original string
		@param off [local::in] offset
		@param count [local::in] count of character(default npos)
	*/
	StringT(const StringT& str, size_type off, size_type count = npos)
		: str_(str.str_, off, count)
	{ }

	/**
		construct from [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	StringT(const CharT *str, size_type count)
		: str_(str, count)
	{
	}

	/**
		construct from [str, NUL)
		@param str [local::in] original string
	*/
	StringT(const CharT *str)
		: str_(str)
	{
	}

	/**
		construct from count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	StringT(size_type count, CharT c)
		: str_(count, c)
	{
	}

	/**
		construct from [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	StringT(Iterator begin, Iterator end)
	{
		append(begin, end);
	}

	// construct from [begin, end), const pointers
//  StringT(const_pointer begin, const_pointer end);
	// construct from [begin, end), const_iterators
//  StringT(const_iterator begin, const_iterator end);

	/**
		construct by copying str
		@param str [local::in] original string
	*/
	StringT(const StringT& str)
		: str_(str.str_)
	{
	}

	/**
		construct by [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	StringT(const char *str, size_type count) // A
	{
		append(str, count);
	}

	/**
		construct from [str, NUL)
		@param str [local::in] original string
	*/
	StringT(const char *str) // A
	{
		append(str);
	}
	/**
		construct by copying str
		@param str [local::in] original string
	*/
	StringT(const std::string& str) // A
	{
		append(str);
	}

	/**
		construt by Char16(same ICU::UChar)
		@param str [local::in] UTF-16 format string
	*/
	StringT(const String16& str) // A
	{
		String16::const_iterator begin = str.begin(), end = str.end();
		while (begin != end) {
			Char c;
			if (!string::GetCharFromUtf16(c, begin, end)) {
				cybozu::StringException e;
				e << "cstr UTF-16";
				throw e;
			}
			str_ += c;
		}
	}
	/**
		construct by BasicString
		@param str [local::in] UTF-32 string
	*/
	StringT(const BasicString& str) // A
		: str_(str)
	{
	}

	/**
		assign str
		@param str [local::in] assign string
	*/
	StringT& operator=(const StringT& str)
	{
		return assign(str);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	StringT& operator=(const CharT *str)
	{
		return assign(str);
	}

	/**
		assign 1 * c
		@param c [local::in] initial character
	*/
	StringT& operator=(CharT c)
	{
		return assign(1, c);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	StringT& operator=(const char *str) // A
	{
		return assign(str);
	}
	/**
		assign str
		@param str [local::in] assign string
	*/
	StringT& operator=(const std::string& str) // A
	{
		return assign(str);
	}

	/**
		append str
		@param str [local::in] append string
	*/
	StringT& operator+=(const StringT& str)
	{
		return append(str);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	StringT& operator+=(const CharT *str)
	{
		return append(str);
	}

	/**
		append 1 * c
		@param c [local::in] append character
	*/
	StringT& operator+=(CharT c)
	{
		return append(1, c);
	}

	/**
		append str
		@param str [local::in] append string
	*/
	StringT& append(const StringT& str)
	{
		str_.append(str.str_); return *this;
	}

	/**
		append str [off, off + count)
		@param str [local::in] append string
		@param off [local::in] string offset
		@param count [local::in] count of character
	*/
	StringT& append(const StringT& str, size_type off, size_type count)
	{
		str_.append(str.str_, off, count); return *this;
	}

	/**
		append [str, str + count)
		@param str [local::in] append string
		@param count [local::in] count of character
	*/
	StringT& append(const CharT *str, size_type count)
	{
		return append(str, str + count);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	StringT& append(const CharT *str)
	{
		str_.append(str); return *this;
	}

	/**
		append count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	StringT& append(size_type count, CharT c)
	{
		str_.append(count, c); return *this;
	}

	/**
		append [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	StringT& append(Iterator begin, Iterator end)
	{
		while (begin != end) {
			CharT c;
			c = getOneChar(begin, end);
			str_.push_back(c);
		}
		return *this;
	}

	// append [begin, end), const pointers
//  StringT& append(const_pointer begin, const_pointer end);
	// append [begin, end), const_iterators
//  StringT& append(const_iterator begin, const_iterator end);

	/**
		append [str, str + count)
		@param str [local::in] append string
		@param count [local::in] count of character
	*/
	StringT& append(const char *str, size_type count) // A
	{
		return append(str, str + count);
	}

	/**
		append [str, NUL)
		@param str [local::in] append string
	*/
	StringT& append(const char *str) // A
	{
		return append(str, std::strlen(str));
	}
	/**
		append str
		@param str [local::in] append string
	*/
	StringT& append(const std::string& str) // A
	{
		return append(str.begin(), str.end());
	}

	/**
		assign str
		@param str [local::in] assign str
	*/
	StringT& assign(const StringT& str)
	{
		clear(); return append(str);
	}

	/**
		assign str [off, off + count)
		@param str [local::in] assign string
		@param off [local::in] offset
		@param count [local::in] count of character
	*/
	StringT& assign(const StringT& str, size_type off, size_type count)
	{
		clear(); return append(str, off, count);
	}

	/**
		assign [str, str + count)
		@param str [local::in] assign string
		@param count [local::in] count of character
	*/
	StringT& assign(const CharT *str, size_type count)
	{
		return assign(str, str + count);
	}

	/**
		assign [str, NUL)
		@param str [local::in] assign string
	*/
	StringT& assign(const CharT *str)
	{
		clear(); return append(str);
	}

	/**
		assign count * c
		@param count [local::in] count of character
		@param c [local::in] initial character
	*/
	StringT& assign(size_type count, CharT c)
	{
		clear(); return append(count, c);
	}

	/**
		assign [First, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	StringT& assign(Iterator begin, Iterator end)
	{
		clear(); return append(begin, end);
	}

	// assign [First, end), const pointers
//  StringT& assign(const_pointer begin, const_pointer end);

	// assign [First, end), const_iterators
//  StringT& assign(const_iterator begin, const_iterator end);

	/**
		assign [str, str + count)
		@param str [local::in] original string
		@param count [local::in] count of character
	*/
	StringT& assign(const char *str, size_type count) // A
	{
		return assign(str, str + count);
	}

	/**
		assign [str, NUL)
		@param str [local::in] original string
	*/
	StringT& assign(const char *str) // A
	{
		clear(); return append(str);
	}
	/**
		assign str
		@param str [local::in] original string
	*/
	StringT& assign(const std::string& str) // A
	{
		clear(); return append(str);
	}

	/**
		insert str at off
		@param off [local::in] offset
		@param str [local::in] insert str
	*/
	StringT& insert(size_type off, const StringT& str)
	{
		str_.insert(off, str.str_); return *this;
	}

	/**
		insert str [off, off + count) at off
		@param off [local::in] offset of destination
		@param rhs [local::in] source str
		@param rhsOff [local::in] offset of source str
		@param count [local::in] count of source str
	*/
	StringT& insert(size_type off, const StringT& rhs, size_type rhsOff, size_type count)
	{
		str_.insert(off, rhs.str_, rhsOff, count); return *this;
	}

	/**
		insert [str, str + count) at off
		@param off [local::in] offset of destination
		@param str [local::in] source str
		@param count [local::in] count of source str
	*/
	StringT& insert(size_type off, const CharT *str, size_type count)
	{
		str_.insert(off, str, count); return *this;
	}

	/**
		insert [str, NUL) at off
		@param off [local::in] offset of destination
		@param str [local::in] source str
	*/
	StringT& insert(size_type off, const CharT *str)
	{
		str_.insert(off, str); return *this;
	}

	/**
		insert count * c at off
		@param off [local::in] offset of destination
		@param count [local::in] count of source str
		@param c [local::in] initial character
	*/
	StringT& insert(size_type off, size_type count, CharT c)
	{
		str_.insert(off, count, c); return *this;
	}
	/**
		insert c at here
		@param here [local::in] offset of destination
		@param c [local::in] initial character(default 0)
	*/
	iterator insert(iterator here, CharT c = 0)
	{
		return str_.insert(here, c);
	}

	/**
		insert count * CharT at here
		@param here [local::in] offset of destination
		@param count [local::in] count of str
		@param c [local::in] initial character
	*/
	void insert(iterator here, size_type count, CharT c)
	{
		str_.insert(here, count, c);
	}

	/**
		insert [begin, end) at here
		@param here [local::in] offset of destination
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	template<class Iterator>
	void insert(iterator here, Iterator begin, Iterator end)
	{
		StringT str(begin, end);
		str_.insert(here, str.begin(), str.end());
	}

	// insert [begin, end) at here, const pointers
//  void insert(iterator here, const_pointer begin, const_pointer end);
	// insert [begin, end) at here, const_iterators
//  void insert(iterator here, const_iterator begin, const_iterator end);

	/**
		erase elements [off, off + count)
		@param off [local::in] offset
		@param count [local::in] count of character(default npos)
	*/
	StringT& erase(size_type off = 0, size_type count = npos)
	{
		str_.erase(off, count); return *this;
	}

	/**
		erase element at here
		@param here [local::in] erase from here
	*/
	iterator erase(iterator here)
	{
		return str_.erase(here);
	}

	/**
		erase substring [begin, end)
		@param begin [local::in] begin of iterator
		@param end [local::in] end of iterator
	*/
	iterator erase(iterator begin, iterator end)
	{
		return str_.erase(begin, end);
	}

	/**
		erase all
	*/
	void clear() { str_.clear(); }

	/**
		replace [off, off + n) with rhs
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param rhs [local::in] append string
	*/
	StringT& replace(size_type off, size_type n, const StringT& rhs)
	{
		str_.replace(off, n, rhs.str_); return *this;
	}

	/**
		replace [off, off + n) with rhs [rhsOff, rhsOff + count)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param rhs [local::in] append string
		@param rhsOff [local::in] append from
		@param count [local::in] count of append
	*/
	StringT& replace(size_type off, size_type n, const StringT& rhs, size_type rhsOff, size_type count)
	{
		str_.replace(off, n, rhs.str_, rhsOff, count); return *this;
	}

	/**
		replace [off, off + n) with [str, str + count)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param str [local::in] append string
		@param count [local::in] count of append
	*/
	StringT& replace(size_type off, size_type n, const CharT *str, size_type count)
	{
		str_.replace(off, n, str, count); return *this;
	}

	/**
		replace [off, off + n) with [str, NUL)
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param str [local::in] append string
	*/
	StringT& replace(size_type off, size_type n, const CharT *str)
	{
		str_.replace(off, n, str); return *this;
	}

	/**
		replace [off, off + n) with count * c
		@param off [local::in] start offset
		@param n [local::in] count of remove character
		@param count [local::in] count of append
		@param c [local::in] initial character
	*/
	StringT& replace(size_type off, size_type n, size_type count, CharT c)
	{
		str_.replace(off, n, count, c); return *this;
	}

	/**
		replace [begin, end) with rhs
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param rhs [local::in] append str
	*/
	StringT& replace(iterator begin, iterator end, const StringT& rhs)
	{
		str_.replace(begin, end, rhs.str_); return *this;
	}

	/**
		replace [begin, end) with [str, str + count)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param str local::in] append str
		@param count [local::in] count of append
	*/
	StringT& replace(iterator begin, iterator end, const CharT *str, size_type count)
	{
		str_.replace(begin, end, str, count); return *this;
	}

	/**
		replace [begin, end) with [str, NUL)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param str local::in] append str
	*/
	StringT& replace(iterator begin, iterator end, const CharT *str)
	{
		str_.replace(begin, end, str); return *this;
	}

	/**
		replace [begin, end) with count * c
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param count [local::in] count of append
		@param c [local::in] initial character
	*/
	StringT& replace(iterator begin, iterator end, size_type count, CharT c)
	{
		str_.replace(begin, end, count, c); return *this;
	}

	/**
		replace [begin, end) with [begin2, end2)
		@param begin [local::in] begin to remove
		@param end [local::in] end to remove
		@param begin2 [local::in] begin to append
		@param end2 [local::in] end to append
	*/
	template<class Iterator>
	StringT& replace(iterator begin, iterator end, Iterator begin2, Iterator end2)
	{
		StringT str(begin2, end2);
		str_.replace(begin, end, str.begin(), str.end());
		return *this;
	}

	// replace [begin, end) with [begin2, end2), const pointers
//  StringT& replace(iterator begin, iterator end, const_pointer begin2, const_pointer end2);

	// replace [begin, end) with [begin2, end2), const_iterators
//  StringT& replace(iterator begin, iterator end, const_iterator begin2, const_iterator end2);

	/**
		return iterator for beginning of mutable sequence
	*/
	iterator begin() { return str_.begin(); }

	/**
		return iterator for beginning of nonmutable sequence
	*/
	const_iterator begin() const { return str_.begin(); }

	/**
		return iterator for end of mutable sequence
	*/
	iterator end() { return str_.end(); }

	/**
		return iterator for end of nonmutable sequence
	*/
	const_iterator end() const { return str_.end(); }

	/**
		return iterator for beginning of reversed mutable sequence
	*/
	reverse_iterator rbegin() { return str_.rbegin(); }

	/**
		return iterator for beginning of reversed nonmutable sequence
	*/
	const_reverse_iterator rbegin() const { return str_.rbegin(); }

	/**
		return iterator for end of reversed mutable sequence
	*/
	reverse_iterator rend() { return str_.rend(); }

	/**
		return iterator for end of reversed nonmutable sequence
	*/
	const_reverse_iterator rend() const { return str_.rend(); }

	/**
		subscript mutable sequence with checking
		@param off [local::in] offset
	*/
	reference at(size_type off) { return str_.at(off); }

	/**
		get element at off
		@param off [local::in] offset
	*/
	const_reference at(size_type off) const { return str_.at(off); }

	/**
		subscript mutable sequence
		@param off [local::in] offset
	*/
	reference operator[](size_type off) { return str_[off]; }

	/**
		subscript nonmutable sequence
		@param off [local::in] offset
	*/
	const_reference operator[](size_type off) const { return str_[off]; }

	/**
		insert element at end
		@param c [local::in] append character
	*/
	void push_back(CharT c)
	{
		str_.push_back(c);
	}

	/**
		return pointer to null-terminated nonmutable array
	*/
	const CharT *c_str() const { return str_.c_str(); }

	/**
		return pointer to nonmutable array
	*/
	const CharT *data() const { return str_.data(); }

	/**
		return length of sequence
	*/
	size_type length() const { return str_.length(); }

	/**
		return length of sequence
	*/
	size_type size() const { return str_.size(); }

	/**
		return maximum possible length of sequence
	*/
	size_type max_size() const { return str_.max_size(); }

	/**
		determine new length, padding with null elements as needed
	*/
	void resize(size_type newSize) { str_.resize(newSize); }

	/**
		determine new length, padding with c elements as needed
		@param newSize [local::in] new length
		@param c [local::in] initial character
	*/
	void resize(size_type newSize, CharT c)
	{
		str_.resize(newSize, c);
	}

	/**
		return current length of allocated storage
	*/
	size_type capacity() const { return str_.capacity(); }

	/**
		determine new minimum length of allocated storage
		@param newSize [local::in] reserve size
	*/
	void reserve(size_type newSize = 0) { str_.reserve(newSize); }

	/**
		test if sequence is empty
		@return true if empty
	*/
	bool empty() const { return str_.empty(); }

	/**
		copy [off, off + count) to [dest, dest + count)
		@param dest [local::in] destination
		@param count [local::in] count of copy
		@param off [local::in] copy from here
	*/
	size_type copy(CharT *dest, size_type count, size_type off = 0) const
	{
#if defined(_MSC_VER) && (_MSC_VER < 1600)
		return str_._Copy_s(dest, count, count, off);
#else
		return str_.copy(dest, count, off);
#endif
	}

	/**
		exchange contents with rhs
		@param rhs [local::in] swap string
	*/
	void swap(StringT& rhs) { str_.swap(rhs.str_); }

	/**
		look for rhs beginnng at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
		@return position
	*/
	size_type find(const StringT& rhs, size_type off = 0) const
	{
		return str_.find(rhs.str_, off);
	}

	/**
		look for [str, str + count) beginnng at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of str
	*/
	size_type find(const CharT *str, size_type off, size_type count) const
	{
		return str_.find(str, off, count);
	}

	/**
		look for [str, NUL) beginnng at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find(const CharT *str, size_type off = 0) const
	{
		return str_.find(str, off);
	}

	/**
		look for c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find(CharT c, size_type off = 0) const
	{
		return str_.find(c, off);
	}

	/**
		look for rhs beginning before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(const StringT& rhs, size_type off = npos) const
	{
		return str_.rfind(rhs.str_, off);
	}

	/**
		look for [str, str + count) beginning before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type rfind(const CharT *str, size_type off, size_type count) const
	{
		return str_.rfind(str, off, count);
	}

	/**
		look for [str, NUL) beginning before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(const CharT *str, size_type off = npos) const
	{
		return str_.rfind(str, off);
	}

	/**
		look for c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type rfind(CharT c, size_type off = npos) const
	{
		return str_.rfind(c, off);
	}

	/**
		look for one of rhs at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(const StringT& rhs, size_type off = 0) const
	{
		return str_.find_first_of(rhs.str_, off);
	}

	/**
		look for one of [str, str + count) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_first_of(const CharT *str, size_type off, size_type count) const
	{
		return str_.find_first_of(str, off, count);
	}

	/**
		look for one of [str, NUL) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(const CharT *str, size_type off = 0) const
	{
		return str_.find_first_of(str, off);
	}

	/**
		look for c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_of(CharT c, size_type off = 0) const
	{
		return str_.find_first_of(c, off);
	}

	/**
		look for one of rhs before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(const StringT& rhs, size_type off = npos) const
	{
		return str_.find_last_of(rhs.str_, off);
	}

	/**
		look for one of [str, str + count) before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_last_of(const CharT *str, size_type off, size_type count) const
	{
		return str_.find_last_of(str, off, count);
	}

	/**
		look for one of [str, NUL) before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(const CharT *str, size_type off = npos) const
	{
		return str_.find_last_of(str, off);
	}

	/**
		look for c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_of(CharT c, size_type off = npos) const
	{
		return str_.find_last_of(c, off);
	}

	/**
		look for none of rhs at or after off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(const StringT& rhs, size_type off = 0) const
	{
		return str_.find_first_not_of(rhs.str_, off);
	}

	/**
		look for none of [str, str + count) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_first_not_of(const CharT *str, size_type off, size_type count) const
	{
		return str_.find_first_not_of(str, off, count);
	}

	/**
		look for one of [str, NUL) at or after off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(const CharT *str, size_type off = 0) const
	{
		return str_.find_first_not_of(str, off);
	}

	/**
		look for non c at or after off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_first_not_of(CharT c, size_type off = 0) const
	{
		return str_.find_first_not_of(c, off);
	}

	/**
		look for none of rhs before off
		@param rhs [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(const StringT& rhs, size_type off = npos) const
	{
		return str_.find_last_not_of(rhs.str_, off);
	}

	/**
		look for none of [str, str + count) before off
		@param str [local::in] target
		@param off [local::in] search from here
		@param count [local::in] count of character
	*/
	size_type find_last_not_of(const CharT *str, size_type off, size_type count) const
	{
		return str_.find_last_not_of(str, off, count);
	}

	/**
		look for none of [str, NUL) before off
		@param str [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(const CharT *str, size_type off = npos) const
	{
		return str_.find_last_not_of(str, off);
	}

	/**
		look for non c before off
		@param c [local::in] target
		@param off [local::in] search from here
	*/
	size_type find_last_not_of(CharT c, size_type off = npos) const
	{
		return str_.find_last_not_of(c, off);
	}
	/**
		return [off, off + count) as new string
		@param off [local::in] from here
		@param count [local::in] count of substring
	*/
	StringT substr(size_type off = 0, size_type count = npos) const
	{
		return str_.substr(off, count);
	}
	/**
		compare *this with rhs
		@param rhs [local::in] target
	*/
	int compare(const StringT& rhs) const
	{
		return str_.compare(rhs.str_);
	}

	/**
		compare [off, off + n) with rhs
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param rhs [local::in] target
	*/
	int compare(size_type off, size_type n, const StringT& rhs) const
	{
		return str_.compare(off, n, rhs.str_);
	}

	/**
		compare [off, off + n) with rhs [rhsOff, rhsOff + count)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param rhs [local::in] target
		@param rhsOff [local::in] target from here
		@param count [local::in] count of rhs
	*/
	int compare(size_type off, size_type n, const StringT& rhs, size_type rhsOff, size_type count) const
	{
		return str_.compare(off, n, rhs.str_, rhsOff, count);
	}

	/**
		compare [0, _Mysize) with [str, NUL)
		@param str [local::in] target
	*/
	int compare(const CharT *str) const
	{
		return str_.compare(str);
	}

	/**
		compare [off, off + n) with [str, NUL)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param str [local::in] target
	*/
	int compare(size_type off, size_type n, const CharT *str) const
	{
		return str_.compare(off, n, str);
	}

	/**
		compare [off, off + n) with [str, str + count)
		@param off [local::in] from here
		@param n [local::in] count of lhs
		@param str [local::in] target
		@param count [local::in] count of rhs
	*/
	int compare(size_type off,size_type n, const CharT *str, size_type count) const
	{
		return str_.compare(off, n, str, count);
	}
	/**
		convert to std::string with UTF-8
	*/
	void toUtf8(std::string& str) const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!string::AppendUtf8(str, str_[i])) {
				cybozu::StringException e;
				e << "toUtf8" << str;
				throw e;
			}
		}
	}
	std::string toUtf8() const
	{
		std::string str;
		toUtf8(str);
		return str;
	}
	/**
		convert to std::string with UTF-16LE
	*/
	void toUtf16(String16& str) const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!string::AppendUtf16(&str, str_[i])) {
				cybozu::StringException e;
				e << "toUtf16";
				throw e;
			}
		}
	}
	String16 toUtf16() const
	{
		String16 str;
		toUtf16(str);
		return str;
	}
	/**
		is this valid unicode string?
		@return true correct string
		@return false bad string
	*/
	bool isValid() const
	{
		for (size_t i = 0, n = str_.size(); i < n; i++) {
			if (!IsValidChar(str_[i])) return false;
		}
		return true;
	}
	/**
		get internal const str(don't use this function)
	*/
	const BasicString& get() const { return str_; }
	/**
		get internal str(don't use this function)
	*/
	BasicString& get() { return str_; }
private:
	template<class Iterator>
	CharT getOneChar(Iterator& begin, const Iterator& end)
	{
		return getOneCharSub(begin, end, *begin);
	}
	// dispatch
	template<class Iterator>
	CharT getOneCharSub(Iterator& begin, const Iterator&, CharT)
	{
		return *begin++;
	}
	template<class Iterator>
	CharT getOneCharSub(Iterator& begin, const Iterator& end, char)
	{
		CharT c;
		if (!cybozu::string::GetCharFromUtf8(&c, begin, end)) {
			cybozu::StringException e;
			e << "getOneCharSub";
			throw e;
		}
		return c;
	}
	BasicString str_;
};

namespace string {
typedef StringT<Char> String;
}

using string::String;

/**
	getline from std::istream
	@param is [local::in] input stream
	@param str [out] one line string
	@param delim [local::in] delimiter
	@return is
*/
inline std::istream& getline(std::istream& is, cybozu::String& str, const char delim = '\n')
{
	std::string tmp;
	std::getline(is, tmp, delim);
	str.assign(tmp);
	return is;
}

/**
	input stream operator as UTF-8
	@param is [local::in] input stream
	@param str [out] input string
	@return is
*/
inline std::istream& operator>>(std::istream& is, cybozu::String& str)
{
	std::string tmp;
	is >> tmp;
	str.assign(tmp);
	return is;
}

/**
	output stream operator as UTF-8
	@param os [local::in] output stream
	@param str [local::in] output string
	@return os
*/
inline std::ostream& operator<<(std::ostream& os, const cybozu::String& str)
{
	return os << str.toUtf8();
}

/**
	concatinate string(lhs + rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
	@return concatinated string
*/
inline cybozu::String operator+(const cybozu::String& lhs, const cybozu::String& rhs) { return cybozu::String(lhs) += rhs; }
/**
	concatinate string(lhs + c)
	@param lhs [local::in] left string
	@param c [local::in] right character
	@return concatinated string
*/
inline cybozu::String operator+(const cybozu::String& lhs, const Char c) { return cybozu::String(lhs) += c; }
/**
	concatinate string(lhs + str[0, NUL))
	@param lhs [local::in] left string
	@param rhs [local::in] right string
	@return concatinated string
*/
inline cybozu::String operator+(const cybozu::String& lhs, const Char* str) { return cybozu::String(lhs) += str; }

/**
	compare string(lhs == rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator==(const cybozu::String& lhs, const cybozu::String& rhs) { return lhs.compare(rhs) == 0; }

/**
	compare string(lhs != rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator!=(const cybozu::String& lhs, const cybozu::String& rhs) { return !(lhs == rhs); }
/**
	compare string(lhs < rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator<(const cybozu::String& lhs, const cybozu::String& rhs) { return lhs.compare(rhs) < 0; }
/**
	compare string(lhs > rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator>(const cybozu::String& lhs, const cybozu::String& rhs) { return lhs.compare(rhs) > 0; }
/**
	compare string(lhs <= rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator<=(const cybozu::String& lhs, const cybozu::String& rhs) { return lhs.compare(rhs) <= 0; }
/**
	compare string(lhs >= rhs)
	@param lhs [local::in] left string
	@param rhs [local::in] right string
*/
inline bool operator>=(const cybozu::String& lhs, const cybozu::String& rhs) { return lhs.compare(rhs) >= 0; }

inline bool ConvertUtf16ToUtf8(std::string *out, const cybozu::Char16 *begin, const cybozu::Char16 *end)
{
	out->clear();
	out->reserve((end - begin) * 3);
	while (begin != end) {
		cybozu::Char c;
		if (!string::GetCharFromUtf16(c, begin, end)) return false;
		if (!string::AppendUtf8(*out, c)) return false;
	}
	return true;
}
inline bool ConvertUtf16ToUtf8(std::string *out, const cybozu::String16& in)
{
	return ConvertUtf16ToUtf8(out, &in[0], &in[0] + in.size());
}

inline bool ConvertUtf8ToUtf16(cybozu::String16 *out, const char *begin, const char *end)
{
	out->clear();
	out->reserve((end - begin) / 2);
	while (begin != end) {
		cybozu::Char c;
		if (!string::GetCharFromUtf8(&c, begin, end)) return false;
		if (!string::AppendUtf16(out, c)) return false;
	}
	return true;
}

inline bool ConvertUtf8ToUtf16(cybozu::String16 *out, const std::string& in)
{
	return ConvertUtf8ToUtf16(out, &in[0], &in[0] + in.size());
}

} // cybozu

// specialization for boost::hash
namespace boost {

template<class T>
struct hash;

template<>
struct hash<cybozu::String> : public std::unary_function<cybozu::String, size_t> {
	size_t operator()(const cybozu::String& str) const
	{
		size_t seed = 0;
		for (size_t i = 0, n = str.size(); i < n; i++) {
			seed ^= str[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2); // copied from boost/functional/hash.hpp
		}
		return seed;
	}
};

} // boost
