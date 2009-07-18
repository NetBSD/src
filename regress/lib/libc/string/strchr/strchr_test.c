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
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

char * (*volatile strchr_fn)(const char *, int);

static int a;
static int t;

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
verify_strchr(char *buf, int ch)
{
    const char *off, *ok_off;

    off = strchr_fn(buf, ch);
    ok_off = slow_strchr(buf, ch);
    if (off == ok_off)
	return;

    fprintf(stderr, "test_strchr(\"%s\", %#x) gave %zd not %zd (test %d, alignment %d)\n",
	    buf, ch, off ? off - buf : -1, ok_off ? ok_off - buf : -1, t, a);

    exit(1);
}

static void
check_strchr(void)
{
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
	    verify_strchr(buf + a, 0);

	    /* Then for the '/' in the strings */
	    verify_strchr(buf + a, '/');

	    /* check zero extension of char arg */
	    verify_strchr(buf + a, 0xffffff00 | '/');

	    /* Replace all the '/' with 0xff */
	    while ((off = slow_strchr(buf + a, '/')) != NULL)
		*off = 0xff;
	    buf[a + len] = 0xff;
   
	    /* Check we can search for 0xff as well as '/' */
	    verify_strchr(buf + a, 0xff);
	}
    }
}

int
main(void)
{
	strchr_fn = dlsym(dlopen(0, RTLD_LAZY), "test_strchr");
	if (!strchr_fn)
		strchr_fn = strchr;
	check_strchr();
	return 0;
}
