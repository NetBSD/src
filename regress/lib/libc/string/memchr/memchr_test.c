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

void check_memchr(void);

void
check_memchr(void)
{
    /* try to trick the compiler */
    void * (*f)(const void *, int, size_t) = memchr;
    
    int a;
    int t;
    void * off;
    char buf[32];

    struct tab {
	const char	*val;
	size_t  	 len;
	char		 match;
	size_t		 off;
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
	    assert((tab[t].match == 0 && off == 0) ||
		   (tab[t].match == 1 && (tab[t].off == ((char*)off - &buf[a]))));
	    
	    // check zero extension of char arg
	    off = f(&buf[a], 0xffffff00 | '/', tab[t].len);
	    assert((tab[t].match == 0 && off == 0) ||
		   (tab[t].match == 1 && (tab[t].off == ((char*)off - &buf[a]))));
	}
    }
}

int
main(void)
{
	check_memchr();
	return 0;
}
