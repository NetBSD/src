/* $NetBSD: t_string.c,v 1.1 2010/12/25 21:10:24 pgoyette Exp $ */

/*
 * Written by J.T. Conklin <jtc@acorntoolworks.com>
 * Public domain.
 */

/*
 * str*() regression suite
 *
 * Trivial str*() implementations can be audited by hand.  Optimized
 * versions that unroll loops, use naturally-aligned memory acesses,
 * and "magic" arithmetic sequences to detect zero-bytes, written in
 * assembler are much harder to validate.  This program attempts to
 * catch the corner cases.
 *
 * BUGS:
 *   Misssing checks for strncpy, strncat, strncmp, etc.
 *
 * TODO:
 *   Use mmap/mprotect to ensure the functions don't access memory
 *   across page boundaries.
 *
 *   Consider generating tables programmatically.  It would reduce
 *   the size, but it's also one more thing to go wrong.
 *
 *   Share tables between strlen, strcpy, and strcat?
 *   Share tables between strchr and strrchr?
 */

#include <atf-c.h>

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

/* for strchr */
char * (*volatile strchr_fn)(const char *, int);

ATF_TC(check_memchr);

ATF_TC_HEAD(check_memchr, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test memchr results");
}

ATF_TC_BODY(check_memchr, tc)
{
	/* try to trick the compiler */
	void * (*f)(const void *, int, size_t) = memchr;
    
	unsigned int a, t;
	void *off, *off2;
	char buf[32];

	struct tab {
		const char	*val;
		size_t  	 len;
		char		 match;
		ssize_t		 off;
	};

	const struct tab tab[] = {
		{ "",			0, 0, 0 },

		{ "/",			0, 0, 0 },
		{ "/",			1, 1, 0 },
		{ "/a",			2, 1, 0 },
		{ "/ab",		3, 1, 0 },
		{ "/abc",		4, 1, 0 },
		{ "/abcd",		5, 1, 0 },
		{ "/abcde",		6, 1, 0 },
		{ "/abcdef",		7, 1, 0 },
		{ "/abcdefg",		8, 1, 0 },

		{ "a/",			1, 0, 0 },
		{ "a/",			2, 1, 1 },
		{ "a/b",		3, 1, 1 },
		{ "a/bc",		4, 1, 1 },
		{ "a/bcd",		5, 1, 1 },
		{ "a/bcde",		6, 1, 1 },
		{ "a/bcdef",		7, 1, 1 },
		{ "a/bcdefg",		8, 1, 1 },

		{ "ab/",		2, 0, 0 },
		{ "ab/",		3, 1, 2 },
		{ "ab/c",		4, 1, 2 },
		{ "ab/cd",		5, 1, 2 },
		{ "ab/cde",		6, 1, 2 },
		{ "ab/cdef",		7, 1, 2 },
		{ "ab/cdefg",		8, 1, 2 },

		{ "abc/",		3, 0, 0 },
		{ "abc/",		4, 1, 3 },
		{ "abc/d",		5, 1, 3 },
		{ "abc/de",		6, 1, 3 },
		{ "abc/def",		7, 1, 3 },
		{ "abc/defg",		8, 1, 3 },

		{ "abcd/",		4, 0, 0 },
		{ "abcd/",		5, 1, 4 },
		{ "abcd/e",		6, 1, 4 },
		{ "abcd/ef",		7, 1, 4 },
		{ "abcd/efg",		8, 1, 4 },

		{ "abcde/",		5, 0, 0 },
		{ "abcde/",		6, 1, 5 },
		{ "abcde/f",		7, 1, 5 },
		{ "abcde/fg",		8, 1, 5 },

		{ "abcdef/",		6, 0, 0 },
		{ "abcdef/",		7, 1, 6 },
		{ "abcdef/g",		8, 1, 6 },

		{ "abcdefg/",		7, 0, 0 },
		{ "abcdefg/",		8, 1, 7 },

		{ "\xff\xff\xff\xff" "efg/",	8, 1, 7 },
		{ "a" "\xff\xff\xff\xff" "fg/",	8, 1, 7 },
		{ "ab" "\xff\xff\xff\xff" "g/",	8, 1, 7 },
		{ "abc" "\xff\xff\xff\xff" "/",	8, 1, 7 },
	};

	for (a = 1; a < 1 + sizeof(long); ++a) {
		for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
			buf[a-1] = '/';
			strcpy(&buf[a], tab[t].val);
   
			off = f(&buf[a], '/', tab[t].len);
			if (tab[t].match == 0) {
				if (off != 0) {
					fprintf(stderr, "a = %d, t = %d\n",
					    a, t);
					atf_tc_fail("should not have found "
					    " char past len");
				}
			} else if (tab[t].match == 1) {
				if (tab[t].off != ((char*)off - &buf[a])) {
					fprintf(stderr, "a = %d, t = %d\n",
					    a, t);
					atf_tc_fail("char not found at "
					    "correct offset");
				}
	    		} else {
				fprintf(stderr, "a = %d, t = %d\n", a, t);
				atf_tc_fail("Corrupt test case data");
			}

			/* check zero extension of char arg */
			off2 = f(&buf[a], 0xffffff00 | '/', tab[t].len);
			if (off2 != off)
				atf_tc_fail("zero extension of char arg "
				    "failed");
		}
	}
}

ATF_TC(check_strcat);

ATF_TC_HEAD(check_strcat, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strcat results");
}

ATF_TC_BODY(check_strcat, tc)
{
	/* try to trick the compiler */
	char * (*f)(char *, const char *s) = strcat;
    
	unsigned int a0, a1, t0, t1;
	char buf0[64];
	char buf1[64];
	char *ret;

	struct tab {
		const char*	val;
		size_t		len;
	};

	const struct tab tab[] = {
	/*
	 * patterns that check for all combinations of leading and
	 * trailing unaligned characters (on a 64 bit processor)
	 */

		{ "",				0 },
		{ "a",				1 },
		{ "ab",				2 },
		{ "abc",			3 },
		{ "abcd",			4 },
		{ "abcde",			5 },
		{ "abcdef",			6 },
		{ "abcdefg",			7 },
		{ "abcdefgh",			8 },
		{ "abcdefghi",			9 },
		{ "abcdefghij",			10 },
		{ "abcdefghijk",		11 },
		{ "abcdefghijkl",		12 },
		{ "abcdefghijklm",		13 },
		{ "abcdefghijklmn",		14 },
		{ "abcdefghijklmno",		15 },
		{ "abcdefghijklmnop",		16 },
		{ "abcdefghijklmnopq",		17 },
		{ "abcdefghijklmnopqr",		18 },
		{ "abcdefghijklmnopqrs",	19 },
		{ "abcdefghijklmnopqrst",	20 },
		{ "abcdefghijklmnopqrstu",	21 },
		{ "abcdefghijklmnopqrstuv",	22 },
		{ "abcdefghijklmnopqrstuvw",	23 },

		/*
		 * patterns that check for the cases where the expression:
		 *
		 *	((word - 0x7f7f..7f) & 0x8080..80)
		 *
		 * returns non-zero even though there are no zero bytes in
		 * the word.
		 */

		{ "" "\xff\xff\xff\xff\xff\xff\xff\xff" "abcdefgh",	16 },
		{ "a" "\xff\xff\xff\xff\xff\xff\xff\xff" "bcdefgh",	16 },
		{ "ab" "\xff\xff\xff\xff\xff\xff\xff\xff" "cdefgh",	16 },
		{ "abc" "\xff\xff\xff\xff\xff\xff\xff\xff" "defgh",	16 },
		{ "abcd" "\xff\xff\xff\xff\xff\xff\xff\xff" "efgh",	16 },
		{ "abcde" "\xff\xff\xff\xff\xff\xff\xff\xff" "fgh",	16 },
		{ "abcdef" "\xff\xff\xff\xff\xff\xff\xff\xff" "gh",	16 },
		{ "abcdefg" "\xff\xff\xff\xff\xff\xff\xff\xff" "h",	16 },
		{ "abcdefgh" "\xff\xff\xff\xff\xff\xff\xff\xff" "",	16 },
	};

	for (a0 = 0; a0 < sizeof(long); ++a0) {
		for (a1 = 0; a1 < sizeof(long); ++a1) {
			for (t0 = 0; t0 < __arraycount(tab); ++t0) {
				for (t1 = 0; t1 < __arraycount(tab); ++t1) {
		    
					memcpy(&buf0[a0], tab[t0].val,
					    tab[t0].len + 1);
					memcpy(&buf1[a1], tab[t1].val,
					    tab[t1].len + 1);

					ret = f(&buf0[a0], &buf1[a1]);
		    
					/*
					 * verify strcat returns address
					 * of first parameter
					 */
					if (&buf0[a0] != ret) {
						fprintf(stderr, "a0 %d, a1 %d, "
						    "t0 %d, t1 %d\n", 
						    a0, a1, t0, t1);
						atf_tc_fail("strcat did not "
						    "return its first arg");
					}

					/* verify string copied correctly */
					if (memcmp(&buf0[a0] + tab[t0].len,
						   &buf1[a1],
						   tab[t1].len + 1) != 0) {
						fprintf(stderr, "a0 %d, a1 %d, "
						    "t0 %d, t1 %d\n", 
						    a0, a1, t0, t1);
						atf_tc_fail("string not copied "
						    "correctly");
					}
				}
			}
		}
	}
}

static char *
slow_strchr(char *buf, int ch)
{
	unsigned char c = 1;

	ch &= 0xff;

	for (; c != 0; buf++) {
		c = *buf;
		if (c == ch)
			return buf;
	}
	return 0;
}

static void
verify_strchr(char *buf, int ch, unsigned int t, unsigned int a)
{
	const char *off, *ok_off;

	off = strchr_fn(buf, ch);
	ok_off = slow_strchr(buf, ch);
	if (off == ok_off)
		return;

	fprintf(stderr, "test_strchr(\"%s\", %#x) gave %zd not %zd (test %d, "
	    "alignment %d)\n",
	    buf, ch, off ? off - buf : -1, ok_off ? ok_off - buf : -1, t, a);

	atf_tc_fail("Check stderr for details");
}

ATF_TC(check_strchr);

ATF_TC_HEAD(check_strchr, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strchr results");
}

ATF_TC_BODY(check_strchr, tc)
{
	unsigned int t, a;
	char *off;
	char buf[32];

	const char *tab[] = {
		"",
		"a",
		"aa",
		"abc",
		"abcd",
		"abcde",
		"abcdef",
		"abcdefg",
		"abcdefgh",

		"/",
		"//",
		"/a",
		"/a/",
		"/ab",
		"/ab/",
		"/abc",
		"/abc/",
		"/abcd",
		"/abcd/",
		"/abcde",
		"/abcde/",
		"/abcdef",
		"/abcdef/",
		"/abcdefg",
		"/abcdefg/",
		"/abcdefgh",
		"/abcdefgh/",
	
		"a/",
		"a//",
		"a/a",
		"a/a/",
		"a/ab",
		"a/ab/",
		"a/abc",
		"a/abc/",
		"a/abcd",
		"a/abcd/",
		"a/abcde",
		"a/abcde/",
		"a/abcdef",
		"a/abcdef/",
		"a/abcdefg",
		"a/abcdefg/",
		"a/abcdefgh",
		"a/abcdefgh/",
	
		"ab/",
		"ab//",
		"ab/a",
		"ab/a/",
		"ab/ab",
		"ab/ab/",
		"ab/abc",
		"ab/abc/",
		"ab/abcd",
		"ab/abcd/",
		"ab/abcde",
		"ab/abcde/",
		"ab/abcdef",
		"ab/abcdef/",
		"ab/abcdefg",
		"ab/abcdefg/",
		"ab/abcdefgh",
		"ab/abcdefgh/",
	
		"abc/",
		"abc//",
		"abc/a",
		"abc/a/",
		"abc/ab",
		"abc/ab/",
		"abc/abc",
		"abc/abc/",
		"abc/abcd",
		"abc/abcd/",
		"abc/abcde",
		"abc/abcde/",
		"abc/abcdef",
		"abc/abcdef/",
		"abc/abcdefg",
		"abc/abcdefg/",
		"abc/abcdefgh",
		"abc/abcdefgh/",
	
		"abcd/",
		"abcd//",
		"abcd/a",
		"abcd/a/",
		"abcd/ab",
		"abcd/ab/",
		"abcd/abc",
		"abcd/abc/",
		"abcd/abcd",
		"abcd/abcd/",
		"abcd/abcde",
		"abcd/abcde/",
		"abcd/abcdef",
		"abcd/abcdef/",
		"abcd/abcdefg",
		"abcd/abcdefg/",
		"abcd/abcdefgh",
		"abcd/abcdefgh/",
	
		"abcde/",
		"abcde//",
		"abcde/a",
		"abcde/a/",
		"abcde/ab",
		"abcde/ab/",
		"abcde/abc",
		"abcde/abc/",
		"abcde/abcd",
		"abcde/abcd/",
		"abcde/abcde",
		"abcde/abcde/",
		"abcde/abcdef",
		"abcde/abcdef/",
		"abcde/abcdefg",
		"abcde/abcdefg/",
		"abcde/abcdefgh",
		"abcde/abcdefgh/",
	
		"abcdef/",
		"abcdef//",
		"abcdef/a",
		"abcdef/a/",
		"abcdef/ab",
		"abcdef/ab/",
		"abcdef/abc",
		"abcdef/abc/",
		"abcdef/abcd",
		"abcdef/abcd/",
		"abcdef/abcde",
		"abcdef/abcde/",
		"abcdef/abcdef",
		"abcdef/abcdef/",
		"abcdef/abcdefg",
		"abcdef/abcdefg/",
		"abcdef/abcdefgh",
		"abcdef/abcdefgh/",
	
		"abcdefg/",
		"abcdefg//",
		"abcdefg/a",
		"abcdefg/a/",
		"abcdefg/ab",
		"abcdefg/ab/",
		"abcdefg/abc",
		"abcdefg/abc/",
		"abcdefg/abcd",
		"abcdefg/abcd/",
		"abcdefg/abcde",
		"abcdefg/abcde/",
		"abcdefg/abcdef",
		"abcdefg/abcdef/",
		"abcdefg/abcdefg",
		"abcdefg/abcdefg/",
		"abcdefg/abcdefgh",
		"abcdefg/abcdefgh/",
	
		"abcdefgh/",
		"abcdefgh//",
		"abcdefgh/a",
		"abcdefgh/a/",
		"abcdefgh/ab",
		"abcdefgh/ab/",
		"abcdefgh/abc",
		"abcdefgh/abc/",
		"abcdefgh/abcd",
		"abcdefgh/abcd/",
		"abcdefgh/abcde",
		"abcdefgh/abcde/",
		"abcdefgh/abcdef",
		"abcdefgh/abcdef/",
		"abcdefgh/abcdefg",
		"abcdefgh/abcdefg/",
		"abcdefgh/abcdefgh",
		"abcdefgh/abcdefgh/",
	};


	strchr_fn = dlsym(dlopen(0, RTLD_LAZY), "test_strchr");
	if (!strchr_fn)
		strchr_fn = strchr;

	for (a = 3; a < 3 + sizeof(long); ++a) {
		/* Put char and a \0 before the buffer */
		buf[a-1] = '/';
		buf[a-2] = '0';
		buf[a-3] = 0xff;
		for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
			int len = strlen(tab[t]) + 1;
			memcpy(&buf[a], tab[t], len);

			/* Put the char we are looking for after the \0 */
			buf[a + len] = '/';

			/* Check search for NUL at end of string */
			verify_strchr(buf + a, 0, t, a);

			/* Then for the '/' in the strings */
			verify_strchr(buf + a, '/', t, a);

		   	/* check zero extension of char arg */
		   	verify_strchr(buf + a, 0xffffff00 | '/', t, a);

		   	/* Replace all the '/' with 0xff */
		   	while ((off = slow_strchr(buf + a, '/')) != NULL)
				*off = 0xff;

			buf[a + len] = 0xff;
   
			/* Check we can search for 0xff as well as '/' */
			verify_strchr(buf + a, 0xff, t, a);
		}
	}
}

ATF_TC(check_strcmp);

ATF_TC_HEAD(check_strcmp, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strcmp results");
}

ATF_TC_BODY(check_strcmp, tc)
{
	/* try to trick the compiler */
	int (*f)(const char *, const char *s) = strcmp;
    
	unsigned int a0, a1, t;
	char buf0[64];
	char buf1[64];
	int ret;

	struct tab {
		const char*	val0;
		const char*	val1;
		int		ret;
	};

	const struct tab tab[] = {
		{ "",			"",			0 },

		{ "a",			"a",			0 },
		{ "a",			"b",			-1 },
		{ "b",			"a",			+1 },
		{ "",			"a",			-1 },
		{ "a",			"",			+1 },

		{ "aa",			"aa",			0 },
		{ "aa",			"ab",			-1 },
		{ "ab",			"aa",			+1 },
		{ "a",			"aa",			-1 },
		{ "aa",			"a",			+1 },

		{ "aaa",		"aaa",			0 },
		{ "aaa",		"aab",			-1 },
		{ "aab",		"aaa",			+1 },
		{ "aa",			"aaa",			-1 },
		{ "aaa",		"aa",			+1 },

		{ "aaaa",		"aaaa",			0 },
		{ "aaaa",		"aaab",			-1 },
		{ "aaab",		"aaaa",			+1 },
		{ "aaa",		"aaaa",			-1 },
		{ "aaaa",		"aaa",			+1 },

		{ "aaaaa",		"aaaaa",		0 },
		{ "aaaaa",		"aaaab",		-1 },
		{ "aaaab",		"aaaaa",		+1 },
		{ "aaaa",		"aaaaa",		-1 },
		{ "aaaaa",		"aaaa",			+1 },

		{ "aaaaaa",		"aaaaaa",		0 },
		{ "aaaaaa",		"aaaaab",		-1 },
		{ "aaaaab",		"aaaaaa",		+1 },
		{ "aaaaa",		"aaaaaa",		-1 },
		{ "aaaaaa",		"aaaaa",		+1 },
	};

	for (a0 = 0; a0 < sizeof(long); ++a0) {
		for (a1 = 0; a1 < sizeof(long); ++a1) {
			for (t = 0; t < __arraycount(tab); ++t) {
				memcpy(&buf0[a0], tab[t].val0,
				       strlen(tab[t].val0) + 1);
				memcpy(&buf1[a1], tab[t].val1,
				       strlen(tab[t].val1) + 1);

				ret = f(&buf0[a0], &buf1[a1]);

				if ((ret == 0 && tab[t].ret != 0) ||
				    (ret <  0 && tab[t].ret >= 0) ||
				    (ret >  0 && tab[t].ret <= 0)) {
					fprintf(stderr, "a0 %d, a1 %d, t %d\n",
					    a0, a1, t);
					fprintf(stderr, "\"%s\" \"%s\" %d\n",
					    &buf0[a0], &buf1[a1], ret);
					atf_tc_fail("Check stderr for details");
				}
			}
		}
	}
}

ATF_TC(check_strcpy);

ATF_TC_HEAD(check_strcpy, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strcpy results");
}

ATF_TC_BODY(check_strcpy, tc)
{
	/* try to trick the compiler */
	char * (*f)(char *, const char *s) = strcpy;
    
	unsigned int a0, a1, t;
	char buf0[64];
	char buf1[64];
	char *ret;

	struct tab {
		const char*	val;
		size_t		len;
	};

	const struct tab tab[] = {
		/*
		 * patterns that check for all combinations of leading and
		 * trailing unaligned characters (on a 64 bit processor)
		 */

		{ "",				0 },
		{ "a",				1 },
		{ "ab",				2 },
		{ "abc",			3 },
		{ "abcd",			4 },
		{ "abcde",			5 },
		{ "abcdef",			6 },
		{ "abcdefg",			7 },
		{ "abcdefgh",			8 },
		{ "abcdefghi",			9 },
		{ "abcdefghij",			10 },
		{ "abcdefghijk",		11 },
		{ "abcdefghijkl",		12 },
		{ "abcdefghijklm",		13 },
		{ "abcdefghijklmn",		14 },
		{ "abcdefghijklmno",		15 },
		{ "abcdefghijklmnop",		16 },
		{ "abcdefghijklmnopq",		17 },
		{ "abcdefghijklmnopqr",		18 },
		{ "abcdefghijklmnopqrs",	19 },
		{ "abcdefghijklmnopqrst",	20 },
		{ "abcdefghijklmnopqrstu",	21 },
		{ "abcdefghijklmnopqrstuv",	22 },
		{ "abcdefghijklmnopqrstuvw",	23 },

		/*
		 * patterns that check for the cases where the expression:
		 *
		 *	((word - 0x7f7f..7f) & 0x8080..80)
		 *
		 * returns non-zero even though there are no zero bytes in
		 * the word.
		 */

		{ "" "\xff\xff\xff\xff\xff\xff\xff\xff" "abcdefgh",	16 },
		{ "a" "\xff\xff\xff\xff\xff\xff\xff\xff" "bcdefgh",	16 },
		{ "ab" "\xff\xff\xff\xff\xff\xff\xff\xff" "cdefgh",	16 },
		{ "abc" "\xff\xff\xff\xff\xff\xff\xff\xff" "defgh",	16 },
		{ "abcd" "\xff\xff\xff\xff\xff\xff\xff\xff" "efgh",	16 },
		{ "abcde" "\xff\xff\xff\xff\xff\xff\xff\xff" "fgh",	16 },
		{ "abcdef" "\xff\xff\xff\xff\xff\xff\xff\xff" "gh",	16 },
		{ "abcdefg" "\xff\xff\xff\xff\xff\xff\xff\xff" "h",	16 },
		{ "abcdefgh" "\xff\xff\xff\xff\xff\xff\xff\xff" "",	16 },
	};

	for (a0 = 0; a0 < sizeof(long); ++a0) {
		for (a1 = 0; a1 < sizeof(long); ++a1) {
			for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {

				memcpy(&buf1[a1], tab[t].val, tab[t].len + 1);
				ret = f(&buf0[a0], &buf1[a1]);

				/*
				 * verify strcpy returns address of
				 * first parameter
				 */
			    	if (&buf0[a0] != ret) {
					fprintf(stderr, "a0 %d, a1 %d, t %d\n",
					    a0, a1, t);
					atf_tc_fail("strcpy did not return "
					    "its first arg");
				}

				/*
				 * verify string was copied correctly
				 */
				if (memcmp(&buf0[a0], &buf1[a1],
					   tab[t].len + 1) != 0) {
					fprintf(stderr, "a0 %d, a1 %d, t %d\n",
					    a0, a1, t);
					atf_tc_fail("not correctly copied");
				}
			}
		}
	}
}

static void
write_num(int val)
{
	char buf[20];
	int i;

	for (i = sizeof buf; --i >= 0;) {
		buf[i] = '0' + val % 10;
		val /= 10;
		if (val == 0) {
			write(2, buf + i, sizeof buf - i);
			return;
		}
	}
	write(2, "overflow", 8);
}

ATF_TC(check_strlen);

ATF_TC_HEAD(check_strlen, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strlen results");
}

ATF_TC_BODY(check_strlen, tc)
{
	/* try to trick the compiler */
	size_t (*strlen_fn)(const char *);

	unsigned int a, t;
	size_t len;
	char buf[64];

	struct tab {
		const char*	val;
		size_t		len;
	};

	const struct tab tab[] = {
		/*
		 * patterns that check for all combinations of leading and
		 * trailing unaligned characters (on a 64 bit processor)
		 */

		{ "",				0 },
		{ "a",				1 },
		{ "ab",				2 },
		{ "abc",			3 },
		{ "abcd",			4 },
		{ "abcde",			5 },
		{ "abcdef",			6 },
		{ "abcdefg",			7 },
		{ "abcdefgh",			8 },
		{ "abcdefghi",			9 },
		{ "abcdefghij",			10 },
		{ "abcdefghijk",		11 },
		{ "abcdefghijkl",		12 },
		{ "abcdefghijklm",		13 },
		{ "abcdefghijklmn",		14 },
		{ "abcdefghijklmno",		15 },
		{ "abcdefghijklmnop",		16 },
		{ "abcdefghijklmnopq",		17 },
		{ "abcdefghijklmnopqr",		18 },
		{ "abcdefghijklmnopqrs",	19 },
		{ "abcdefghijklmnopqrst",	20 },
		{ "abcdefghijklmnopqrstu",	21 },
		{ "abcdefghijklmnopqrstuv",	22 },
		{ "abcdefghijklmnopqrstuvw",	23 },

		/*
		 * patterns that check for the cases where the expression:
		 *
		 *	((word - 0x7f7f..7f) & 0x8080..80)
		 *
		 * returns non-zero even though there are no zero bytes in
		 * the word.
		 */

		{ "" "\xff\xff\xff\xff\xff\xff\xff\xff" "abcdefgh",	16 },
		{ "a" "\xff\xff\xff\xff\xff\xff\xff\xff" "bcdefgh",	16 },
		{ "ab" "\xff\xff\xff\xff\xff\xff\xff\xff" "cdefgh",	16 },
		{ "abc" "\xff\xff\xff\xff\xff\xff\xff\xff" "defgh",	16 },
		{ "abcd" "\xff\xff\xff\xff\xff\xff\xff\xff" "efgh",	16 },
		{ "abcde" "\xff\xff\xff\xff\xff\xff\xff\xff" "fgh",	16 },
		{ "abcdef" "\xff\xff\xff\xff\xff\xff\xff\xff" "gh",	16 },
		{ "abcdefg" "\xff\xff\xff\xff\xff\xff\xff\xff" "h",	16 },
		{ "abcdefgh" "\xff\xff\xff\xff\xff\xff\xff\xff" "",	16 },
	};

	/*
	 * During testing it is useful have the rest of the program
	 * use a known good version!
	 */
	strlen_fn = dlsym(dlopen(NULL, RTLD_LAZY), "test_strlen");
	if (!strlen_fn)
		strlen_fn = strlen;

	for (a = 0; a < sizeof(long); ++a) {
		for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
	    
			memcpy(&buf[a], tab[t].val, tab[t].len + 1);
			len = strlen_fn(&buf[a]);

			if (len != tab[t].len) {
				/* Write error without using printf / strlen */
				write(2, "alignment ", 10);
				write_num(a);
				write(2, ", test ", 7);
				write_num(t);
				write(2, ", got len ", 10);
				write_num(len);
				write(2, ", not ", 6);
				write_num(tab[t].len);
				write(2, ", for '", 7);
				write(2, tab[t].val, tab[t].len);
				write(2, "'\n", 2);
				atf_tc_fail("See stderr for details");
			}
		}
	}
}

ATF_TC(check_strrchr);

ATF_TC_HEAD(check_strrchr, tc)
{

        atf_tc_set_md_var(tc, "descr", "Test strrchr results");
}

ATF_TC_BODY(check_strrchr, tc)
{
	/* try to trick the compiler */
	char * (*f)(const char *, int) = strrchr;

	unsigned int a, t;
	char *off, *off2;
	char buf[32];

	struct tab {
		const char*	val;
		char		match;
		ssize_t		f_off;	/* offset of first match */
		ssize_t		l_off;	/* offset of last match */
	};

	const struct tab tab[] = {
		{ "",			0, 0, 0 },
		{ "a",			0, 0, 0 },
		{ "aa",			0, 0, 0 },
		{ "abc",		0, 0, 0 },
		{ "abcd",		0, 0, 0 },
		{ "abcde",		0, 0, 0 },
		{ "abcdef",		0, 0, 0 },
		{ "abcdefg",		0, 0, 0 },
		{ "abcdefgh",		0, 0, 0 },

		{ "/",			1, 0, 0 },
		{ "//",                 1, 0, 1 },
		{ "/a",                 1, 0, 0 },
		{ "/a/",                1, 0, 2 },
		{ "/ab",                1, 0, 0 },
		{ "/ab/",               1, 0, 3 },
		{ "/abc",               1, 0, 0 },
		{ "/abc/",              1, 0, 4 },
		{ "/abcd",              1, 0, 0 },
		{ "/abcd/",             1, 0, 5 },
		{ "/abcde",             1, 0, 0 },
		{ "/abcde/",            1, 0, 6 },
		{ "/abcdef",            1, 0, 0 },
		{ "/abcdef/",           1, 0, 7 },
		{ "/abcdefg",           1, 0, 0 },
		{ "/abcdefg/",          1, 0, 8 },
		{ "/abcdefgh",          1, 0, 0 },
		{ "/abcdefgh/",         1, 0, 9 },
	
		{ "a/",                 1, 1, 1 },
		{ "a//",                1, 1, 2 },
		{ "a/a",                1, 1, 1 },
		{ "a/a/",               1, 1, 3 },
		{ "a/ab",               1, 1, 1 },
		{ "a/ab/",              1, 1, 4 },
		{ "a/abc",              1, 1, 1 },
		{ "a/abc/",             1, 1, 5 },
		{ "a/abcd",             1, 1, 1 },
		{ "a/abcd/",            1, 1, 6 },
		{ "a/abcde",            1, 1, 1 },
		{ "a/abcde/",           1, 1, 7 },
		{ "a/abcdef",           1, 1, 1 },
		{ "a/abcdef/",          1, 1, 8 },
		{ "a/abcdefg",          1, 1, 1 },
		{ "a/abcdefg/",         1, 1, 9 },
		{ "a/abcdefgh",         1, 1, 1 },
		{ "a/abcdefgh/",        1, 1, 10 },
	
		{ "ab/",                1, 2, 2 },
		{ "ab//",               1, 2, 3 },
		{ "ab/a",               1, 2, 2 },
		{ "ab/a/",              1, 2, 4 },
		{ "ab/ab",              1, 2, 2 },
		{ "ab/ab/",             1, 2, 5 },
		{ "ab/abc",             1, 2, 2 },
		{ "ab/abc/",            1, 2, 6 },
		{ "ab/abcd",            1, 2, 2 },
		{ "ab/abcd/",           1, 2, 7 },
		{ "ab/abcde",           1, 2, 2 },
		{ "ab/abcde/",          1, 2, 8 },
		{ "ab/abcdef",          1, 2, 2 },
		{ "ab/abcdef/",         1, 2, 9 },
		{ "ab/abcdefg",         1, 2, 2 },
		{ "ab/abcdefg/",        1, 2, 10 },
		{ "ab/abcdefgh",        1, 2, 2 },
		{ "ab/abcdefgh/",       1, 2, 11 },
	
		{ "abc/",               1, 3, 3 },
		{ "abc//",              1, 3, 4 },
		{ "abc/a",              1, 3, 3 },
		{ "abc/a/",             1, 3, 5 },
		{ "abc/ab",             1, 3, 3 },
		{ "abc/ab/",            1, 3, 6 },
		{ "abc/abc",            1, 3, 3 },
		{ "abc/abc/",           1, 3, 7 },
		{ "abc/abcd",           1, 3, 3 },
		{ "abc/abcd/",          1, 3, 8 },
		{ "abc/abcde",          1, 3, 3 },
		{ "abc/abcde/",         1, 3, 9 },
		{ "abc/abcdef",         1, 3, 3 },
		{ "abc/abcdef/",        1, 3, 10 },
		{ "abc/abcdefg",        1, 3, 3 },
		{ "abc/abcdefg/",       1, 3, 11 },
		{ "abc/abcdefgh",       1, 3, 3 },
		{ "abc/abcdefgh/",      1, 3, 12 },
	
		{ "abcd/",              1, 4, 4 },
		{ "abcd//",             1, 4, 5 },
		{ "abcd/a",             1, 4, 4 },
		{ "abcd/a/",            1, 4, 6 },
		{ "abcd/ab",            1, 4, 4 },
		{ "abcd/ab/",           1, 4, 7 },
		{ "abcd/abc",           1, 4, 4 },
		{ "abcd/abc/",          1, 4, 8 },
		{ "abcd/abcd",          1, 4, 4 },
		{ "abcd/abcd/",         1, 4, 9 },
		{ "abcd/abcde",         1, 4, 4 },
		{ "abcd/abcde/",        1, 4, 10 },
		{ "abcd/abcdef",        1, 4, 4 },
		{ "abcd/abcdef/",       1, 4, 11 },
		{ "abcd/abcdefg",       1, 4, 4 },
		{ "abcd/abcdefg/",      1, 4, 12 },
		{ "abcd/abcdefgh",      1, 4, 4 },
		{ "abcd/abcdefgh/",     1, 4, 13 },
	
		{ "abcde/",             1, 5, 5 },
		{ "abcde//",            1, 5, 6 },
		{ "abcde/a",            1, 5, 5 },
		{ "abcde/a/",           1, 5, 7 },
		{ "abcde/ab",           1, 5, 5 },
		{ "abcde/ab/",          1, 5, 8 },
		{ "abcde/abc",          1, 5, 5 },
		{ "abcde/abc/",         1, 5, 9 },
		{ "abcde/abcd",         1, 5, 5 },
		{ "abcde/abcd/",        1, 5, 10 },
		{ "abcde/abcde",        1, 5, 5 },
		{ "abcde/abcde/",       1, 5, 11 },
		{ "abcde/abcdef",       1, 5, 5 },
		{ "abcde/abcdef/",      1, 5, 12 },
		{ "abcde/abcdefg",      1, 5, 5 },
		{ "abcde/abcdefg/",     1, 5, 13 },
		{ "abcde/abcdefgh",     1, 5, 5 },
		{ "abcde/abcdefgh/",    1, 5, 14 },
	
		{ "abcdef/",            1, 6, 6 },
		{ "abcdef//",           1, 6, 7 },
		{ "abcdef/a",           1, 6, 6 },
		{ "abcdef/a/",          1, 6, 8 },
		{ "abcdef/ab",          1, 6, 6 },
		{ "abcdef/ab/",         1, 6, 9 },
		{ "abcdef/abc",         1, 6, 6 },
		{ "abcdef/abc/",        1, 6, 10 },
		{ "abcdef/abcd",        1, 6, 6 },
		{ "abcdef/abcd/",       1, 6, 11 },
		{ "abcdef/abcde",       1, 6, 6 },
		{ "abcdef/abcde/",      1, 6, 12 },
		{ "abcdef/abcdef",      1, 6, 6 },
		{ "abcdef/abcdef/",     1, 6, 13 },
		{ "abcdef/abcdefg",     1, 6, 6 },
		{ "abcdef/abcdefg/",    1, 6, 14 },
		{ "abcdef/abcdefgh",    1, 6, 6 },
		{ "abcdef/abcdefgh/",   1, 6, 15 },
	
		{ "abcdefg/",           1, 7, 7 },
		{ "abcdefg//",          1, 7, 8 },
		{ "abcdefg/a",          1, 7, 7 },
		{ "abcdefg/a/",         1, 7, 9 },
		{ "abcdefg/ab",         1, 7, 7 },
		{ "abcdefg/ab/",        1, 7, 10 },
		{ "abcdefg/abc",        1, 7, 7 },
		{ "abcdefg/abc/",       1, 7, 11 },
		{ "abcdefg/abcd",       1, 7, 7 },
		{ "abcdefg/abcd/",      1, 7, 12 },
		{ "abcdefg/abcde",      1, 7, 7 },
		{ "abcdefg/abcde/",     1, 7, 13 },
		{ "abcdefg/abcdef",     1, 7, 7 },
		{ "abcdefg/abcdef/",    1, 7, 14 },
		{ "abcdefg/abcdefg",    1, 7, 7 },
		{ "abcdefg/abcdefg/",   1, 7, 15 },
		{ "abcdefg/abcdefgh",   1, 7, 7 },
		{ "abcdefg/abcdefgh/",  1, 7, 16 },
	
		{ "abcdefgh/",          1, 8, 8 },
		{ "abcdefgh//",         1, 8, 9 },
		{ "abcdefgh/a",         1, 8, 8 },
		{ "abcdefgh/a/",        1, 8, 10 },
		{ "abcdefgh/ab",        1, 8, 8 },
		{ "abcdefgh/ab/",       1, 8, 11 },
		{ "abcdefgh/abc",       1, 8, 8 },
		{ "abcdefgh/abc/",      1, 8, 12 },
		{ "abcdefgh/abcd",      1, 8, 8 },
		{ "abcdefgh/abcd/",     1, 8, 13 },
		{ "abcdefgh/abcde",     1, 8, 8 },
		{ "abcdefgh/abcde/",    1, 8, 14 },
		{ "abcdefgh/abcdef",    1, 8, 8 },
		{ "abcdefgh/abcdef/",   1, 8, 15 },
		{ "abcdefgh/abcdefg",   1, 8, 8 },
		{ "abcdefgh/abcdefg/",  1, 8, 16 },
		{ "abcdefgh/abcdefgh",  1, 8, 8 },
		{ "abcdefgh/abcdefgh/", 1, 8, 17 },
	};

	for (a = 0; a < sizeof(long); ++a) {
		for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
			strcpy(&buf[a], tab[t].val);
	    
			off = f(&buf[a], '/');
			if (tab[t].match == 0) {
				if (off != 0) {
					fprintf(stderr, "a %d, t %d\n", a, t);
					atf_tc_fail("strrchr should not have "
					    "found the character");
				}
			} else if (tab[t].match == 1) {
				 if (tab[t].l_off != (off - &buf[a])) {
					fprintf(stderr, "a %d, t %d\n", a, t);
					atf_tc_fail("strrchr returns wrong "
					    "offset");
				}
			} else {
				fprintf(stderr, "a %d, t %d\n", a, t);
				atf_tc_fail("bad test case data");
			}
	    
			/* check zero extension of char arg */
			off2 = f(&buf[a], 0xffffff00 | '/');
			if (off != off2) {
				fprintf(stderr, "a %d, t %d\n", a, t);
				atf_tc_fail("zero extension of char arg fails");
			}
		}
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, check_memchr);
	ATF_TP_ADD_TC(tp, check_strcat);
	ATF_TP_ADD_TC(tp, check_strchr);
	ATF_TP_ADD_TC(tp, check_strcmp);
	ATF_TP_ADD_TC(tp, check_strcpy);
	ATF_TP_ADD_TC(tp, check_strlen);
	ATF_TP_ADD_TC(tp, check_strrchr);

	return atf_no_error();
}
