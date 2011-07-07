/* $NetBSD: t_strcmp.c,v 1.1 2011/07/07 08:59:33 jruoho Exp $ */

/*
 * Written by J.T. Conklin <jtc@acorntoolworks.com>
 * Public domain.
 */

#include <atf-c.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

ATF_TC(strcmp_basic);
ATF_TC_HEAD(strcmp_basic, tc)
{
        atf_tc_set_md_var(tc, "descr", "Test strcmp(3) results");
}

ATF_TC_BODY(strcmp_basic, tc)
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

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, strcmp_basic);

	return atf_no_error();
}
