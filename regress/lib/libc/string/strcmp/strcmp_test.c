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

#include <assert.h>
#include <string.h>

void check_strcmp(void);

void
check_strcmp(void)
{
    /* try to trick the compiler */
    int (*f)(const char *, const char *s) = strcmp;
    
    int a0;
    int a1;
    int t;
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
	    for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
		memcpy(&buf0[a0], tab[t].val0, strlen(tab[t].val0) + 1);
		memcpy(&buf1[a1], tab[t].val1, strlen(tab[t].val1) + 1);

		ret = f(&buf0[a0], &buf1[a1]);

		assert ((ret == 0 && tab[t].ret == 0) ||
			(ret <  0 && tab[t].ret <  0) ||
			(ret >  0 && tab[t].ret >  0));
	    }
	}
    }
}

int
main(void)
{
	check_strcmp();
	return 0;
}
