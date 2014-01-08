/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test for bitmap class, compression performance

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestDfaRegex
#include <boost/test/auto_unit_test.hpp>

#include "log.hpp"
#define LOGNULL

#include "regex.hpp"

#include <vector>
#include <sstream>

using namespace re;

void regex_test(Regex & p_regex,
                const char * p_str,
                bool p_exact_result_search,
                bool p_result_search)
{
    BOOST_CHECK_EQUAL(p_regex.exact_search(p_str), p_exact_result_search);
    BOOST_CHECK_EQUAL(p_regex.search(p_str), p_result_search);
}

#include <iostream>
#define regex_test(p_regex,\
                   p_str,\
                   p_exact_result_search,\
                   p_result_search)\
std::cout << "l." << __LINE__ << std::endl; \
regex_test(p_regex, p_str, p_exact_result_search, p_result_search)

void test_re(re::Regex::flag_t flags)
{
    const char * str_regex = "a";
    Regex regex(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 0, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 1);
    regex_test(regex, "ba", 0, 1);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 1);
    regex_test(regex, "abc", 0, 1);
    regex_test(regex, "dabc", 0, 1);

    str_regex = "^";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 1, 1);

    str_regex = "$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 1, 1);

    str_regex = "^$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaa", 0, 0);
    regex_test(regex, "a", 0, 0);
    regex_test(regex, "", 1, 1);

//     str_regex = "$^";
//     regex.reset(str_regex, flags);
//     if (regex.message_error()) {
//         std::ostringstream os;
//         os << str_regex << (regex.message_error())
//         << " at offset " << regex.position_error();
//         BOOST_CHECK_MESSAGE(false, os.str());
//     }
//     regex_test(regex, "aaa", 0, 0);
//     regex_test(regex, "a", 0, 0);
//     regex_test(regex, "", 1, 1);

    const char * str = "";
    regex_test(regex, "", 1, 1);

    str_regex = "^a";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 0, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 0);
    regex_test(regex, "ba", 0, 0);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 1);
    regex_test(regex, "abc", 0, 1);
    regex_test(regex, "dabc", 0, 0);

    str_regex = "a^b";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a", 0, 0);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "ba", 0, 0);
    regex_test(regex, "b", 0, 0);

    str_regex = "a$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 0, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 1);
    regex_test(regex, "aaaab", 0, 0);
    regex_test(regex, "ba", 0, 1);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 0);
    regex_test(regex, "abc", 0, 0);
    regex_test(regex, "dabc", 0, 0);

    str_regex = "^a$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 0, 0);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 0);
    regex_test(regex, "ba", 0, 0);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 0);
    regex_test(regex, "abc", 0, 0);
    regex_test(regex, "dabc", 0, 0);

    str_regex = "^a|^.?";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 1, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 1, 1);
    regex_test(regex, "ba", 1, 1);

    str_regex = "a+";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 1, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 1);
    regex_test(regex, "ba", 0, 1);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 1);
    regex_test(regex, "abc", 0, 1);
    regex_test(regex, "dabc", 0, 1);

    str_regex = ".*a.*$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 1, 1);
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 1, 1);
    regex_test(regex, "ba", 1, 1);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 1, 1);
    regex_test(regex, "abc", 1, 1);
    regex_test(regex, "dabc", 1, 1);

    str_regex = "aa+$";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaa", 1, 1);
    regex_test(regex, "a", 0, 0);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baaaa", 0, 1);
    regex_test(regex, "baa", 0, 1);
    regex_test(regex, "ba", 0, 0);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "ab", 0, 0);
    regex_test(regex, "dabc", 0, 0);

    str_regex = "aa*b?a";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aa", 1, 1);
    regex_test(regex, "aaaa", 1, 1);
    regex_test(regex, "ab", 0, 0);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "baabaa", 0, 1);
    regex_test(regex, "baab", 0, 1);
    regex_test(regex, "bba", 0, 0);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "aba", 1, 1);
    regex_test(regex, "dabc", 0, 0);

    str_regex = "[a-cd]";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a", 1, 1);
    regex_test(regex, "b", 1, 1);
    regex_test(regex, "c", 1, 1);
    regex_test(regex, "d", 1, 1);
    regex_test(regex, "", 0, 0);
    regex_test(regex, "lka", 0, 1);

    str_regex = "[^a-cd]";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a", 0, 0);
    regex_test(regex, "b", 0, 0);
    regex_test(regex, "c", 0, 0);
    regex_test(regex, "d", 0, 0);
    regex_test(regex, "lka", 0, 1);


    str_regex = "\\a";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "a";
    regex_test(regex, str, 1, 1);


    str_regex = "[\\a]";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "a";
    regex_test(regex, str, 1, 1);


    str_regex = "\\[a]";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "[a]";
    regex_test(regex, str, 1, 1);

    str_regex = "(?:.)?";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "a";
    regex_test(regex, str, 1, 1);
    str = "";
    regex_test(regex, str, 1, 1);


    str_regex = "(?:a(bv)|(av))(d)";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "abvd";
    regex_test(regex, str, 1, 1);
//     BOOST_CHECK(regex.match_result(false) == matches);

    str = "avd";
    regex_test(regex, str, 1, 1);

    str_regex = " *|(?:.)?";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "";
    regex_test(regex, str, 1, 1);


    str_regex = "(\\d){3}";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    str = "012";
    regex_test(regex, str, 1, 1);

    str_regex = "b?a{,1}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "a{,1}c", 1, 1);

    str_regex = "b?a{0,1}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "ac", 1, 1);

    str_regex = "b?a{0,3}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "ac", 1, 1);

    str_regex = "b?a{2,4}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaac", 1, 1);

    str_regex = "b?a{2,}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "aaaaaac", 1, 1);

    str_regex = "b?a{0,}c";
    regex.reset(str_regex, flags);
    if (regex.message_error()) {
        std::ostringstream os;
        os << str_regex << (regex.message_error())
        << " at offset " << regex.position_error();
        BOOST_CHECK_MESSAGE(false, os.str());
    }
    regex_test(regex, "c", 1, 1);
}


BOOST_AUTO_TEST_CASE(TestRegex)
{
    test_re(re::Regex::DEFAULT_FLAG);
}

// BOOST_AUTO_TEST_CASE(TestRegexOptimize)
// {
//     test_re(re::Regex::MINIMAL_MEMORY|re::Regex::OPTIMIZE_MEMORY);
// }
