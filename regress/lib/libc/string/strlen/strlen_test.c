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
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>

void check_strlen(void);

size_t (*volatile strlen_fn)(const char *s);

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

void
check_strlen(void)
{
    /* try to trick the compiler */

    int a;
    int t;
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
	 * returns non-zero even though there are no zero bytes in the
	 * word.
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

    for (a = 0; a < sizeof(long); ++a) {
	for (t = 0; t < (sizeof(tab) / sizeof(tab[0])); ++t) {
	    
	    memcpy(&buf[a], tab[t].val, tab[t].len + 1);
	    len = strlen_fn(&buf[a]);

	    if (len != tab[t].len) {
		/* Write error without using printf (and strlen) */
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
		exit(1);
	    }
	}
    }
}

int
main(void)
{
	/* During testing it is useful have the rest of the program
	 * use a known good version! */
	strlen_fn = dlsym(dlopen(NULL, RTLD_LAZY), "test_strlen");
	if (!strlen_fn)
		strlen_fn = strlen;
	check_strlen();
	return 0;
}
