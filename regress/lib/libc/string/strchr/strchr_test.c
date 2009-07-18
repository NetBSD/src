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

extern char *xstrchr(const char *, int);
void check_strchr(void);

/* try to trick the compiler */
char * (*volatile f)(const char *, int);

void
check_strchr(void)
{
    int a;
    int t;
    char* off;
    char buf[32];

    struct tab {
	const char*	val;
	char		match;
	size_t		f_off;	/* offset of first match */
	size_t		l_off;	/* offset of last match */
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
	if (a >= 2) {
		/* Put char and a \0 before the buffer */
		buf[a-1] = '/';
		buf[a-2] = '0';
	}
	for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
	    int len = strlen(tab[t].val) + 1;
	    memcpy(&buf[a], tab[t].val, len);
	    /* Put the char we are looking for after the \0 */
	    buf[a + len] = '/';
	    
	    off = f(&buf[a], '/');
	    assert((tab[t].match == 0 && off == 0) ||
		   (tab[t].match == 1 && (tab[t].f_off == (off - &buf[a]))));
	    
	    // check zero extension of char arg
	    off = f(&buf[a], 0xffffff00 | '/');
	    assert((tab[t].match == 0 && off == 0) ||
		   (tab[t].match == 1 && (tab[t].f_off == (off - &buf[a]))));
	}
    }
}

int
main(void)
{
	f = strchr;
	check_strchr();
	return 0;
}
