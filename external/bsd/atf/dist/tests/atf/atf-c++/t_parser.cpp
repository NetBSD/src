//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <sstream>

#include "atf-c++/macros.hpp"
#include "atf-c++/parser.hpp"

#include "h_lib.hpp"

// ------------------------------------------------------------------------
// Tests for the "token" class.
// ------------------------------------------------------------------------

ATF_TEST_CASE(token_getters);
ATF_TEST_CASE_HEAD(token_getters)
{
    set_md_var("descr", "Tests the token getters");
}
ATF_TEST_CASE_BODY(token_getters)
{
    using atf::parser::token;

    {
        token t(10, 0);
        ATF_CHECK_EQUAL(t.lineno(), 10);
        ATF_CHECK_EQUAL(t.type(), 0);
        ATF_CHECK(t.text().empty());
    }

    {
        token t(10, 0, "foo");
        ATF_CHECK_EQUAL(t.lineno(), 10);
        ATF_CHECK_EQUAL(t.type(), 0);
        ATF_CHECK_EQUAL(t.text(), "foo");
    }

    {
        token t(20, 1);
        ATF_CHECK_EQUAL(t.lineno(), 20);
        ATF_CHECK_EQUAL(t.type(), 1);
        ATF_CHECK(t.text().empty());
    }

    {
        token t(20, 1, "bar");
        ATF_CHECK_EQUAL(t.lineno(), 20);
        ATF_CHECK_EQUAL(t.type(), 1);
        ATF_CHECK_EQUAL(t.text(), "bar");
    }
}

// ------------------------------------------------------------------------
// Tests for the "tokenizer" class.
// ------------------------------------------------------------------------

#define EXPECT(tkz, ttype, ttext) \
    do { \
        atf::parser::token t = tkz.next(); \
        ATF_CHECK(t.type() == ttype); \
        ATF_CHECK_EQUAL(t.text(), ttext); \
    } while (false);

namespace minimal {

    static const atf::parser::token_type& eof_type = 0;
    static const atf::parser::token_type& nl_type = 1;
    static const atf::parser::token_type& word_type = 2;

    class tokenizer : public atf::parser::tokenizer< std::istream > {
    public:
        tokenizer(std::istream& is, bool skipws) :
            atf::parser::tokenizer< std::istream >
                (is, skipws, eof_type, nl_type, word_type)
        {
        }
    };

}

namespace delims {

    static const atf::parser::token_type& eof_type = 0;
    static const atf::parser::token_type& nl_type = 1;
    static const atf::parser::token_type& word_type = 2;
    static const atf::parser::token_type& plus_type = 3;
    static const atf::parser::token_type& minus_type = 4;
    static const atf::parser::token_type& equal_type = 5;

    class tokenizer : public atf::parser::tokenizer< std::istream > {
    public:
        tokenizer(std::istream& is, bool skipws) :
            atf::parser::tokenizer< std::istream >
                (is, skipws, eof_type, nl_type, word_type)
        {
            add_delim('+', plus_type);
            add_delim('-', minus_type);
            add_delim('=', equal_type);
        }
    };

}

namespace keywords {

    static const atf::parser::token_type& eof_type = 0;
    static const atf::parser::token_type& nl_type = 1;
    static const atf::parser::token_type& word_type = 2;
    static const atf::parser::token_type& var_type = 3;
    static const atf::parser::token_type& loop_type = 4;
    static const atf::parser::token_type& endloop_type = 5;

    class tokenizer : public atf::parser::tokenizer< std::istream > {
    public:
        tokenizer(std::istream& is, bool skipws) :
            atf::parser::tokenizer< std::istream >
                (is, skipws, eof_type, nl_type, word_type)
        {
            add_keyword("var", var_type);
            add_keyword("loop", loop_type);
            add_keyword("endloop", endloop_type);
        }
    };

}

namespace quotes {

    static const atf::parser::token_type& eof_type = 0;
    static const atf::parser::token_type& nl_type = 1;
    static const atf::parser::token_type& word_type = 2;
    static const atf::parser::token_type& dblquote_type = 3;

    class tokenizer : public atf::parser::tokenizer< std::istream > {
    public:
        tokenizer(std::istream& is, bool skipws) :
            atf::parser::tokenizer< std::istream >
                (is, skipws, eof_type, nl_type, word_type)
        {
            add_quote('"', dblquote_type);
        }
    };

}

ATF_TEST_CASE(tokenizer_minimal_nows);
ATF_TEST_CASE_HEAD(tokenizer_minimal_nows)
{
    set_md_var("descr", "Tests the tokenizer class using a minimal parser "
               "and not skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_minimal_nows)
{
    using namespace minimal;

    {
        std::istringstream iss("");
        tokenizer mt(iss, false);

        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\n");
        tokenizer mt(iss, false);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\n\n\n");
        tokenizer mt(iss, false);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "line 1");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\n");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "line 1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\nline 2");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "line 1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line 2");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\nline 2\nline 3\n");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "line 1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line 2");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line 3");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_minimal_ws);
ATF_TEST_CASE_HEAD(tokenizer_minimal_ws)
{
    set_md_var("descr", "Tests the tokenizer class using a minimal parser "
               "and skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_minimal_ws)
{
    using namespace minimal;

    {
        std::istringstream iss("");
        minimal::tokenizer mt(iss, true);

        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss(" \t ");
        tokenizer mt(iss, true);

        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\n");
        tokenizer mt(iss, true);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss(" \t \n \t ");
        tokenizer mt(iss, true);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\n\n\n");
        tokenizer mt(iss, true);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("   \tline\t   1\t");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\n");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\nline 2");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "2");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("line 1\nline 2\nline 3\n");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "2");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "3");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss(" \t line \t 1\n\tline\t2\n line 3 \n");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "1");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "2");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, word_type, "line");
        EXPECT(mt, word_type, "3");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_delims_nows);
ATF_TEST_CASE_HEAD(tokenizer_delims_nows)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with some "
               "additional delimiters and not skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_delims_nows)
{
    using namespace delims;

    {
        std::istringstream iss("+-=");
        tokenizer mt(iss, false);

        EXPECT(mt, plus_type, "+");
        EXPECT(mt, minus_type, "-");
        EXPECT(mt, equal_type, "=");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("+++");
        tokenizer mt(iss, false);

        EXPECT(mt, plus_type, "+");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\n+\n++\n");
        tokenizer mt(iss, false);

        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("foo+bar=baz");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "foo");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, word_type, "bar");
        EXPECT(mt, equal_type, "=");
        EXPECT(mt, word_type, "baz");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss(" foo\t+\tbar = baz ");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, " foo\t");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, word_type, "\tbar ");
        EXPECT(mt, equal_type, "=");
        EXPECT(mt, word_type, " baz ");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_delims_ws);
ATF_TEST_CASE_HEAD(tokenizer_delims_ws)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with some "
               "additional delimiters and skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_delims_ws)
{
    using namespace delims;

    {
        std::istringstream iss(" foo\t+\tbar = baz ");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "foo");
        EXPECT(mt, plus_type, "+");
        EXPECT(mt, word_type, "bar");
        EXPECT(mt, equal_type, "=");
        EXPECT(mt, word_type, "baz");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_keywords_nows);
ATF_TEST_CASE_HEAD(tokenizer_keywords_nows)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with some "
               "additional keywords and not skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_keywords_nows)
{
    using namespace keywords;

    {
        std::istringstream iss("var");
        tokenizer mt(iss, false);

        EXPECT(mt, var_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("va");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "va");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("vara");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "vara");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("var ");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "var ");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("var\nloop\nendloop");
        tokenizer mt(iss, false);

        EXPECT(mt, var_type, "var");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, loop_type, "loop");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, endloop_type, "endloop");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_keywords_ws);
ATF_TEST_CASE_HEAD(tokenizer_keywords_ws)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with some "
               "additional keywords and not skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_keywords_ws)
{
    using namespace keywords;

    {
        std::istringstream iss("var ");
        tokenizer mt(iss, true);

        EXPECT(mt, var_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss(" var \n\tloop\t\n \tendloop \t");
        tokenizer mt(iss, true);

        EXPECT(mt, var_type, "var");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, loop_type, "loop");
        EXPECT(mt, nl_type, "<<NEWLINE>>");
        EXPECT(mt, endloop_type, "endloop");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("var loop endloop");
        tokenizer mt(iss, true);

        EXPECT(mt, var_type, "var");
        EXPECT(mt, loop_type, "loop");
        EXPECT(mt, endloop_type, "endloop");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_quotes_nows);
ATF_TEST_CASE_HEAD(tokenizer_quotes_nows)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with "
               "quoted strings and not skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_quotes_nows)
{
    using namespace quotes;

    {
        std::istringstream iss("var");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("\"var\"");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("var1\"var2\"");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "var1");
        EXPECT(mt, word_type, "var2");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("var1\"  var2  \"");
        tokenizer mt(iss, false);

        EXPECT(mt, word_type, "var1");
        EXPECT(mt, word_type, "  var2  ");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

ATF_TEST_CASE(tokenizer_quotes_ws);
ATF_TEST_CASE_HEAD(tokenizer_quotes_ws)
{
    set_md_var("descr", "Tests the tokenizer class using a parser with "
               "quoted strings and skipping whitespace");
}
ATF_TEST_CASE_BODY(tokenizer_quotes_ws)
{
    using namespace quotes;

    {
        std::istringstream iss("  var  ");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("  \"var\"  ");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "var");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("  var1  \"var2\"  ");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "var1");
        EXPECT(mt, word_type, "var2");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }

    {
        std::istringstream iss("  var1  \"  var2  \"  ");
        tokenizer mt(iss, true);

        EXPECT(mt, word_type, "var1");
        EXPECT(mt, word_type, "  var2  ");
        EXPECT(mt, eof_type, "<<EOF>>");
        EXPECT(mt, eof_type, "<<EOF>>");
    }
}

// ------------------------------------------------------------------------
// Tests cases for the header file.
// ------------------------------------------------------------------------

HEADER_TC(include, "atf-c++/parser.hpp", "d_include_parser_hpp.cpp");

// ------------------------------------------------------------------------
// Main.
// ------------------------------------------------------------------------

ATF_INIT_TEST_CASES(tcs)
{
    // Add test cases for the "token" class.
    ATF_ADD_TEST_CASE(tcs, token_getters);

    // Add test cases for the "tokenizer" class.
    ATF_ADD_TEST_CASE(tcs, tokenizer_minimal_nows);
    ATF_ADD_TEST_CASE(tcs, tokenizer_minimal_ws);
    ATF_ADD_TEST_CASE(tcs, tokenizer_delims_nows);
    ATF_ADD_TEST_CASE(tcs, tokenizer_delims_ws);
    ATF_ADD_TEST_CASE(tcs, tokenizer_keywords_nows);
    ATF_ADD_TEST_CASE(tcs, tokenizer_keywords_ws);
    ATF_ADD_TEST_CASE(tcs, tokenizer_quotes_nows);
    ATF_ADD_TEST_CASE(tcs, tokenizer_quotes_ws);

    // Add the test cases for the header file.
    ATF_ADD_TEST_CASE(tcs, include);
}
