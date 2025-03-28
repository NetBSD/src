/*	$NetBSD: t_ctype.c,v 1.6 2025/03/28 22:52:35 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_ctype.c,v 1.6 2025/03/28 22:52:35 riastradh Exp $");

#include <atf-c.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>

#include "h_macros.h"

#ifdef __CHAR_UNSIGNED__
enum { CHAR_UNSIGNED = 1 };
#else
enum { CHAR_UNSIGNED = 0 };
#endif

static const char *locales[] = { "C.UTF-8", "fr_FR.ISO8859-1", "C" };

static int isalpha_wrapper(int ch) { return isalpha(ch); }
static int isupper_wrapper(int ch) { return isupper(ch); }
static int islower_wrapper(int ch) { return islower(ch); }
static int isdigit_wrapper(int ch) { return isdigit(ch); }
static int isxdigit_wrapper(int ch) { return isxdigit(ch); }
static int isalnum_wrapper(int ch) { return isalnum(ch); }
static int isspace_wrapper(int ch) { return isspace(ch); }
static int ispunct_wrapper(int ch) { return ispunct(ch); }
static int isprint_wrapper(int ch) { return isprint(ch); }
static int isgraph_wrapper(int ch) { return isgraph(ch); }
static int iscntrl_wrapper(int ch) { return iscntrl(ch); }
static int isblank_wrapper(int ch) { return isblank(ch); }
static int toupper_wrapper(int ch) { return toupper(ch); }
static int tolower_wrapper(int ch) { return tolower(ch); }

jmp_buf env;

static void
handle_sigsegv(int signo)
{

	longjmp(env, 1);
}

static void
test_abuse(const char *name, int (*ctypefn)(int))
{
	volatile int ch;	/* for longjmp */

	for (ch = CHAR_MIN; ch < 0; ch++) {
		void (*h)(int) = SIG_DFL;
		volatile int result;

		if (ch == EOF)
			continue;
		ATF_REQUIRE_MSG(ch != (int)(unsigned char)ch, "ch=%d", ch);
		if (setjmp(env) == 0) {
			REQUIRE_LIBC(h = signal(SIGSEGV, &handle_sigsegv),
			    SIG_ERR);
			result = (*ctypefn)(ch);
			REQUIRE_LIBC(signal(SIGSEGV, h), SIG_ERR);
			atf_tc_fail_nonfatal("%s failed to detect invalid %d,"
			    " returned %d",
			    name, ch, result);
		} else {
			REQUIRE_LIBC(signal(SIGSEGV, h), SIG_ERR);
		}
	}

	for (; ch <= CHAR_MAX; ch++)
		ATF_REQUIRE_MSG(ch == (int)(unsigned char)ch, "ch=%d", ch);
}

static void
test_use(const char *name, int (*ctypefn)(int))
{
	volatile int ch;	/* for longjmp */

	for (ch = EOF; ch <= CHAR_MAX; ch = (ch == EOF ? 0 : ch + 1)) {
		void (*h)(int) = SIG_DFL;
		volatile int result;

		if (setjmp(env) == 0) {
			REQUIRE_LIBC(h = signal(SIGSEGV, &handle_sigsegv),
			    SIG_ERR);
			result = (*ctypefn)(ch);
			REQUIRE_LIBC(signal(SIGSEGV, h), SIG_ERR);
			(void)result;
		} else {
			atf_tc_fail_nonfatal("%s(%d) raised SIGSEGV",
			    name, ch);
			REQUIRE_LIBC(signal(SIGSEGV, h), SIG_ERR);
		}
	}
}

static void
test_isalpha_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isalpha", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isupper_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isupper", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_islower_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("islower", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isdigit_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isdigit", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isxdigit_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isxdigit", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isalnum_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isalnum", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isspace_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isspace", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_ispunct_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("ispunct", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isprint_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isprint", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isgraph_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isgraph", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_iscntrl_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("iscntrl", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_isblank_locale(const char *L, int (*ctypefn)(int))
{

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("isblank", ctypefn);
	ATF_CHECK(!(*ctypefn)(EOF));
}

static void
test_toupper_locale(const char *L, int (*ctypefn)(int))
{
	int result;

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("toupper", ctypefn);
	ATF_CHECK_MSG((result = (*ctypefn)(EOF)) == EOF,
	    "result=%d, expected EOF=%d", result, EOF);
}

static void
test_tolower_locale(const char *L, int (*ctypefn)(int))
{
	int result;

	ATF_REQUIRE_MSG(setlocale(LC_CTYPE, L) != NULL, "L=%s", L);
	test_use("tolower", ctypefn);
	ATF_CHECK_MSG((result = (*ctypefn)(EOF)) == EOF,
	    "result=%d, expected EOF=%d", result, EOF);
}

static void
test_isalpha_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'i': case 'I':
		case 'j': case 'J':
		case 'k': case 'K':
		case 'l': case 'L':
		case 'm': case 'M':
		case 'n': case 'N':
		case 'o': case 'O':
		case 'p': case 'P':
		case 'q': case 'Q':
		case 'r': case 'R':
		case 's': case 'S':
		case 't': case 'T':
		case 'u': case 'U':
		case 'v': case 'V':
		case 'w': case 'W':
		case 'x': case 'X':
		case 'y': case 'Y':
		case 'z': case 'Z':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isupper_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q':
		case 'R':
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_islower_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isdigit_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isxdigit_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isalnum_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'i': case 'I':
		case 'j': case 'J':
		case 'k': case 'K':
		case 'l': case 'L':
		case 'm': case 'M':
		case 'n': case 'N':
		case 'o': case 'O':
		case 'p': case 'P':
		case 'q': case 'Q':
		case 'r': case 'R':
		case 's': case 'S':
		case 't': case 'T':
		case 'u': case 'U':
		case 'v': case 'V':
		case 'w': case 'W':
		case 'x': case 'X':
		case 'y': case 'Y':
		case 'z': case 'Z':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isspace_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case ' ':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_ispunct_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		default:
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		case 0 ... 0x1f:
		case 0x20:	/* space is printing but not punctuation */
		case 0x7f:
		case 0x80 ... 0xff:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		case 'a': case 'A':
		case 'b': case 'B':
		case 'c': case 'C':
		case 'd': case 'D':
		case 'e': case 'E':
		case 'f': case 'F':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'i': case 'I':
		case 'j': case 'J':
		case 'k': case 'K':
		case 'l': case 'L':
		case 'm': case 'M':
		case 'n': case 'N':
		case 'o': case 'O':
		case 'p': case 'P':
		case 'q': case 'Q':
		case 'r': case 'R':
		case 's': case 'S':
		case 't': case 'T':
		case 'u': case 'U':
		case 'v': case 'V':
		case 'w': case 'W':
		case 'x': case 'X':
		case 'y': case 'Y':
		case 'z': case 'Z':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isprint_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 0x20:	/* space is printing but not graphic */
		default:
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		case 0 ... 0x1f:
		case 0x7f:
		case 0x80 ... 0xff:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isgraph_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		default:
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		case 0 ... 0x1f:
		case 0x20:	/* space is printing but not graphic */
		case 0x7f:
		case 0x80 ... 0xff:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_iscntrl_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 0 ... 0x1f:
		case 0x7f:
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_isblank_c(int (*ctypefn)(int))
{
	int ch;

	ATF_CHECK(!(*ctypefn)(EOF));
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case ' ':
		case '\t':
			ATF_CHECK_MSG((*ctypefn)(ch), "ch=0x%x", ch);
			break;
		default:
			ATF_CHECK_MSG(!(*ctypefn)(ch), "ch=0x%x", ch);
			break;
		}
	}
}

static void
test_toupper_c(int (*ctypefn)(int))
{
	int ch, result, expected;

	ATF_CHECK_MSG((result = (*ctypefn)(EOF)) == EOF,
	    "result=%d, expected EOF=%d", result, EOF);
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'a': case 'A': expected = 'A'; break;
		case 'b': case 'B': expected = 'B'; break;
		case 'c': case 'C': expected = 'C'; break;
		case 'd': case 'D': expected = 'D'; break;
		case 'e': case 'E': expected = 'E'; break;
		case 'f': case 'F': expected = 'F'; break;
		case 'g': case 'G': expected = 'G'; break;
		case 'h': case 'H': expected = 'H'; break;
		case 'i': case 'I': expected = 'I'; break;
		case 'j': case 'J': expected = 'J'; break;
		case 'k': case 'K': expected = 'K'; break;
		case 'l': case 'L': expected = 'L'; break;
		case 'm': case 'M': expected = 'M'; break;
		case 'n': case 'N': expected = 'N'; break;
		case 'o': case 'O': expected = 'O'; break;
		case 'p': case 'P': expected = 'P'; break;
		case 'q': case 'Q': expected = 'Q'; break;
		case 'r': case 'R': expected = 'R'; break;
		case 's': case 'S': expected = 'S'; break;
		case 't': case 'T': expected = 'T'; break;
		case 'u': case 'U': expected = 'U'; break;
		case 'v': case 'V': expected = 'V'; break;
		case 'w': case 'W': expected = 'W'; break;
		case 'x': case 'X': expected = 'X'; break;
		case 'y': case 'Y': expected = 'Y'; break;
		case 'z': case 'Z': expected = 'Z'; break;
		default:
			expected = ch;
			break;
		}
		ATF_CHECK_MSG((result = (*ctypefn)(ch)) == expected,
		    "result=%d expected=%d", result, expected);
	}
}

static void
test_tolower_c(int (*ctypefn)(int))
{
	int ch, result, expected;

	ATF_CHECK_MSG((result = (*ctypefn)(EOF)) == EOF,
	    "result=%d, expected EOF=%d", result, EOF);
	for (ch = 0; ch <= UCHAR_MAX; ch++) {
		switch (ch) {
		case 'a': case 'A': expected = 'a'; break;
		case 'b': case 'B': expected = 'b'; break;
		case 'c': case 'C': expected = 'c'; break;
		case 'd': case 'D': expected = 'd'; break;
		case 'e': case 'E': expected = 'e'; break;
		case 'f': case 'F': expected = 'f'; break;
		case 'g': case 'G': expected = 'g'; break;
		case 'h': case 'H': expected = 'h'; break;
		case 'i': case 'I': expected = 'i'; break;
		case 'j': case 'J': expected = 'j'; break;
		case 'k': case 'K': expected = 'k'; break;
		case 'l': case 'L': expected = 'l'; break;
		case 'm': case 'M': expected = 'm'; break;
		case 'n': case 'N': expected = 'n'; break;
		case 'o': case 'O': expected = 'o'; break;
		case 'p': case 'P': expected = 'p'; break;
		case 'q': case 'Q': expected = 'q'; break;
		case 'r': case 'R': expected = 'r'; break;
		case 's': case 'S': expected = 's'; break;
		case 't': case 'T': expected = 't'; break;
		case 'u': case 'U': expected = 'u'; break;
		case 'v': case 'V': expected = 'v'; break;
		case 'w': case 'W': expected = 'w'; break;
		case 'x': case 'X': expected = 'x'; break;
		case 'y': case 'Y': expected = 'y'; break;
		case 'z': case 'Z': expected = 'z'; break;
		default:
			expected = ch;
			break;
		}
		ATF_CHECK_MSG((result = (*ctypefn)(ch)) == expected,
		    "result=%d expected=%d", result, expected);
	}
}

#define	ADD_TEST_ABUSE(TP, FN) do					      \
{									      \
	ATF_TP_ADD_TC(TP, abuse_##FN##_macro_c);			      \
	ATF_TP_ADD_TC(TP, abuse_##FN##_function_c);			      \
	ATF_TP_ADD_TC(TP, abuse_##FN##_macro_locale);			      \
	ATF_TP_ADD_TC(TP, abuse_##FN##_function_locale);		      \
} while (0)

#define	DEF_TEST_ABUSE(FN)						      \
ATF_TC(abuse_##FN##_macro_c);						      \
ATF_TC_HEAD(abuse_##FN##_macro_c, tc)					      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test abusing "#FN" macro with default LC_CTYPE=C");	      \
}									      \
ATF_TC_BODY(abuse_##FN##_macro_c, tc)					      \
{									      \
	if (CHAR_UNSIGNED) {						      \
		atf_tc_skip("runtime ctype(3) abuse is impossible with"	      \
		    " unsigned char");					      \
	}								      \
	atf_tc_expect_fail("PR lib/58208:"				      \
	    " ctype(3) provides poor runtime feedback of abuse");	      \
	test_abuse(#FN, &FN##_wrapper);					      \
}									      \
ATF_TC(abuse_##FN##_function_c);					      \
ATF_TC_HEAD(abuse_##FN##_function_c, tc)				      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test abusing "#FN" function with default LC_CTYPE=C");	      \
}									      \
ATF_TC_BODY(abuse_##FN##_function_c, tc)				      \
{									      \
	if (CHAR_UNSIGNED) {						      \
		atf_tc_skip("runtime ctype(3) abuse is impossible with"	      \
		    " unsigned char");					      \
	}								      \
	atf_tc_expect_fail("PR lib/58208:"				      \
	    " ctype(3) provides poor runtime feedback of abuse");	      \
	test_abuse(#FN, &FN);						      \
}									      \
ATF_TC(abuse_##FN##_macro_locale);					      \
ATF_TC_HEAD(abuse_##FN##_macro_locale, tc)				      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test abusing "#FN" macro with locales");			      \
}									      \
ATF_TC_BODY(abuse_##FN##_macro_locale, tc)				      \
{									      \
	size_t i;							      \
									      \
	if (CHAR_UNSIGNED) {						      \
		atf_tc_skip("runtime ctype(3) abuse is impossible with"	      \
		    " unsigned char");					      \
	}								      \
	for (i = 0; i < __arraycount(locales); i++) {			      \
		char buf[128];						      \
									      \
		ATF_REQUIRE_MSG(setlocale(LC_CTYPE, locales[i]) != NULL,      \
		    "locales[i]=%s", locales[i]);			      \
		snprintf(buf, sizeof(buf), "[%s]%s", locales[i], #FN);	      \
		if (strcmp(locales[i], "C") == 0) {			      \
			atf_tc_expect_fail("PR lib/58208: ctype(3)"	      \
			    " provides poor runtime feedback of abuse");      \
		}							      \
		test_abuse(buf, &FN##_wrapper);				      \
		if (strcmp(locales[i], "C") == 0)			      \
			atf_tc_expect_pass();				      \
	}								      \
}									      \
ATF_TC(abuse_##FN##_function_locale);					      \
ATF_TC_HEAD(abuse_##FN##_function_locale, tc)				      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test abusing "#FN" function with locales");		      \
}									      \
ATF_TC_BODY(abuse_##FN##_function_locale, tc)				      \
{									      \
	size_t i;							      \
									      \
	if (CHAR_UNSIGNED) {						      \
		atf_tc_skip("runtime ctype(3) abuse is impossible with"	      \
		    " unsigned char");					      \
	}								      \
	for (i = 0; i < __arraycount(locales); i++) {			      \
		char buf[128];						      \
									      \
		ATF_REQUIRE_MSG(setlocale(LC_CTYPE, locales[i]) != NULL,      \
		    "locales[i]=%s", locales[i]);			      \
		snprintf(buf, sizeof(buf), "[%s]%s", locales[i], #FN);	      \
		if (strcmp(locales[i], "C") == 0) {			      \
			atf_tc_expect_fail("PR lib/58208: ctype(3)"	      \
			    " provides poor runtime feedback of abuse");      \
		}							      \
		test_abuse(buf, &FN);					      \
		if (strcmp(locales[i], "C") == 0)			      \
			atf_tc_expect_pass();				      \
	}								      \
}

#define	ADD_TEST_USE(TP, FN) do						      \
{									      \
	ATF_TP_ADD_TC(TP, FN##_macro_c);				      \
	ATF_TP_ADD_TC(TP, FN##_function_c);				      \
	ATF_TP_ADD_TC(TP, FN##_macro_locale);				      \
	ATF_TP_ADD_TC(TP, FN##_function_locale);			      \
} while (0)

#define	DEF_TEST_USE(FN)						      \
ATF_TC(FN##_macro_c);							      \
ATF_TC_HEAD(FN##_macro_c, tc)						      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test "#FN" macro with default LC_CTYPE=C");		      \
}									      \
ATF_TC_BODY(FN##_macro_c, tc)						      \
{									      \
	test_##FN##_c(&FN##_wrapper);					      \
}									      \
ATF_TC(FN##_function_c);						      \
ATF_TC_HEAD(FN##_function_c, tc)					      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test "#FN" function with default LC_CTYPE=C");		      \
}									      \
ATF_TC_BODY(FN##_function_c, tc)					      \
{									      \
	test_##FN##_c(&FN);						      \
}									      \
ATF_TC(FN##_macro_locale);						      \
ATF_TC_HEAD(FN##_macro_locale, tc)					      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test "#FN" macro with locales");				      \
}									      \
ATF_TC_BODY(FN##_macro_locale, tc)					      \
{									      \
	size_t i;							      \
									      \
	for (i = 0; i < __arraycount(locales); i++)			      \
		test_##FN##_locale(locales[i], &FN##_wrapper);		      \
}									      \
ATF_TC(FN##_function_locale);						      \
ATF_TC_HEAD(FN##_function_locale, tc)					      \
{									      \
	atf_tc_set_md_var(tc, "descr",					      \
	    "Test "#FN" function with locales");			      \
}									      \
ATF_TC_BODY(FN##_function_locale, tc)					      \
{									      \
	size_t i;							      \
									      \
	for (i = 0; i < __arraycount(locales); i++)			      \
		test_##FN##_locale(locales[i], &FN);			      \
}

DEF_TEST_ABUSE(isalpha)
DEF_TEST_ABUSE(isupper)
DEF_TEST_ABUSE(islower)
DEF_TEST_ABUSE(isdigit)
DEF_TEST_ABUSE(isxdigit)
DEF_TEST_ABUSE(isalnum)
DEF_TEST_ABUSE(isspace)
DEF_TEST_ABUSE(ispunct)
DEF_TEST_ABUSE(isprint)
DEF_TEST_ABUSE(isgraph)
DEF_TEST_ABUSE(iscntrl)
DEF_TEST_ABUSE(isblank)
DEF_TEST_ABUSE(toupper)
DEF_TEST_ABUSE(tolower)

DEF_TEST_USE(isalpha)
DEF_TEST_USE(isupper)
DEF_TEST_USE(islower)
DEF_TEST_USE(isdigit)
DEF_TEST_USE(isxdigit)
DEF_TEST_USE(isalnum)
DEF_TEST_USE(isspace)
DEF_TEST_USE(ispunct)
DEF_TEST_USE(isprint)
DEF_TEST_USE(isgraph)
DEF_TEST_USE(iscntrl)
DEF_TEST_USE(isblank)
DEF_TEST_USE(toupper)
DEF_TEST_USE(tolower)

ATF_TC(eof_confusion_iso8859_1);
ATF_TC_HEAD(eof_confusion_iso8859_1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test potential confusion with EOF in ISO-8859-1");
}
ATF_TC_BODY(eof_confusion_iso8859_1, tc)
{
	int ydots = 0xff;	/* ÿ, LATIN SMALL LETTER Y WITH DIAERESIS */
	int ch;

	/*
	 * The LATIN SMALL LETTER Y WITH DIAERESIS code point 0xff in
	 * ISO-8859-1 is curious primarily because its bit pattern
	 * coincides with an 8-bit signed -1, which is to say, EOF as
	 * an 8-bit quantity; of course, for EOF, all of the is*
	 * functions are supposed to return false (as we test above).
	 * It also has the curious property that it lacks any
	 * corresponding uppercase code point in ISO-8859-1, so we
	 * can't distinguish it from EOF by tolower/toupper.
	 */
	ATF_REQUIRE(setlocale(LC_CTYPE, "fr_FR.ISO8859-1") != NULL);
	ATF_CHECK(isalpha(ydots));
	ATF_CHECK(!isupper(ydots));
	ATF_CHECK(islower(ydots));
	ATF_CHECK(!isdigit(ydots));
	ATF_CHECK(!isxdigit(ydots));
	ATF_CHECK(isalnum(ydots));
	ATF_CHECK(!isspace(ydots));
	ATF_CHECK(!ispunct(ydots));
	ATF_CHECK(isprint(ydots));
	ATF_CHECK(isgraph(ydots));
	ATF_CHECK(!iscntrl(ydots));
	ATF_CHECK(!isblank(ydots));
	ATF_CHECK_MSG((ch = toupper(ydots)) == ydots, "ch=0x%x", ch);
	ATF_CHECK_MSG((ch = tolower(ydots)) == ydots, "ch=0x%x", ch);
}

ATF_TC(eof_confusion_koi8_u);
ATF_TC_HEAD(eof_confusion_koi8_u, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test potential confusion with EOF in KOI8-U");
}
ATF_TC_BODY(eof_confusion_koi8_u, tc)
{
	int Hard = 0xff;	/* Ъ, CYRILLIC CAPITAL LETTER HARD SIGN */
	int hard = 0xdf;	/* ъ, CYRILLIC SMALL LETTER HARD SIGN */
	int ch;

	/*
	 * The CYRILLIC CAPITAL LETTER HARD SIGN code point 0xff in
	 * KOI8-U (and KOI8-R) also coincides with the bit pattern of
	 * an 8-bit signed -1.  Unlike LATIN SMALL LETTER Y WITH
	 * DIAERESIS, it has a lowercase equivalent in KOI8-U.
	 */
	ATF_REQUIRE(setlocale(LC_CTYPE, "uk_UA.KOI8-U") != NULL);
	ATF_CHECK(isalpha(Hard));
	ATF_CHECK(isupper(Hard));
	ATF_CHECK(!islower(Hard));
	ATF_CHECK(!isdigit(Hard));
	ATF_CHECK(!isxdigit(Hard));
	ATF_CHECK(isalnum(Hard));
	ATF_CHECK(!isspace(Hard));
	ATF_CHECK(!ispunct(Hard));
	ATF_CHECK(isprint(Hard));
	ATF_CHECK(isgraph(Hard));
	ATF_CHECK(!iscntrl(Hard));
	ATF_CHECK(!isblank(Hard));
	ATF_CHECK_MSG((ch = toupper(Hard)) == Hard, "ch=0x%x", ch);
	ATF_CHECK_MSG((ch = tolower(Hard)) == hard, "ch=0x%x", ch);
}

ATF_TC(eof_confusion_pt154);
ATF_TC_HEAD(eof_confusion_pt154, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test potential confusion with EOF in PT154");
}
ATF_TC_BODY(eof_confusion_pt154, tc)
{
	int ya = 0xff;		/* я, CYRILLIC SMALL LETTER YA */
	int Ya = 0xdf;		/* Я, CYRILLIC CAPITAL LETTER YA */
	int ch;

	/*
	 * The CYRILLIC SMALL LETTER YA code point 0xff in PT154 also
	 * coincides with the bit pattern of an 8-bit signed -1, and is
	 * lowercase with a corresponding uppercase code point in
	 * PT154.
	 */
	ATF_REQUIRE(setlocale(LC_CTYPE, "kk_KZ.PT154") != NULL);
	ATF_CHECK(isalpha(ya));
	ATF_CHECK(!isupper(ya));
	ATF_CHECK(islower(ya));
	ATF_CHECK(!isdigit(ya));
	ATF_CHECK(!isxdigit(ya));
	ATF_CHECK(isalnum(ya));
	ATF_CHECK(!isspace(ya));
	ATF_CHECK(!ispunct(ya));
	ATF_CHECK(isprint(ya));
	ATF_CHECK(isgraph(ya));
	ATF_CHECK(!iscntrl(ya));
	ATF_CHECK(!isblank(ya));
	ATF_CHECK_MSG((ch = toupper(ya)) == Ya, "ch=0x%x", ch);
	ATF_CHECK_MSG((ch = tolower(ya)) == ya, "ch=0x%x", ch);
}

ATF_TP_ADD_TCS(tp)
{

	ADD_TEST_ABUSE(tp, isalpha);
	ADD_TEST_ABUSE(tp, isupper);
	ADD_TEST_ABUSE(tp, islower);
	ADD_TEST_ABUSE(tp, isdigit);
	ADD_TEST_ABUSE(tp, isxdigit);
	ADD_TEST_ABUSE(tp, isalnum);
	ADD_TEST_ABUSE(tp, isspace);
	ADD_TEST_ABUSE(tp, ispunct);
	ADD_TEST_ABUSE(tp, isprint);
	ADD_TEST_ABUSE(tp, isgraph);
	ADD_TEST_ABUSE(tp, iscntrl);
	ADD_TEST_ABUSE(tp, isblank);
	ADD_TEST_ABUSE(tp, toupper);
	ADD_TEST_ABUSE(tp, tolower);

	ADD_TEST_USE(tp, isalpha);
	ADD_TEST_USE(tp, isupper);
	ADD_TEST_USE(tp, islower);
	ADD_TEST_USE(tp, isdigit);
	ADD_TEST_USE(tp, isxdigit);
	ADD_TEST_USE(tp, isalnum);
	ADD_TEST_USE(tp, isspace);
	ADD_TEST_USE(tp, ispunct);
	ADD_TEST_USE(tp, isprint);
	ADD_TEST_USE(tp, isgraph);
	ADD_TEST_USE(tp, iscntrl);
	ADD_TEST_USE(tp, isblank);
	ADD_TEST_USE(tp, toupper);
	ADD_TEST_USE(tp, tolower);

	ATF_TP_ADD_TC(tp, eof_confusion_iso8859_1);
	ATF_TP_ADD_TC(tp, eof_confusion_koi8_u);
	ATF_TP_ADD_TC(tp, eof_confusion_pt154);

	return atf_no_error();
}
