/*	$NetBSD: test.c,v 1.2 2003/07/26 19:38:45 salo Exp $	*/

/*
 * Regression test for basename(3).
 *
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, Oct. 2002.
 * Public domain.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

struct {
	const char *input;
	const char *output;
} test_table[] = {
/*
 * The following are taken from the "Sample Input and Output Strings
 * for basename()" table in IEEE Std 1003.1-2001.
 */
	{ "/usr/lib",		"lib" },
	{ "/usr/",		"usr" },
	{ "/",			"/" },
	{ "///",		"/" },
	{ "//usr//lib//",	"lib" },
/*
 * IEEE Std 1003.1-2001:
 *
 *	If path is a null pointer or points to an empty string,
 *	basename() shall return a pointer to the string "." .
 */
	{ "",			"." },
	{ NULL,			"." },
/*
 * IEEE Std 1003.1-2001:
 *
 *	If the string is exactly "//", it is implementation-defined
 *	whether "/" or "//" is returned.
 *
 * The NetBSD implementation returns "/".
 */
	{ "//",			"/" },

	{ NULL,			NULL }
};

int	main(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
	char testbuf[32], *base;
	int i, rv = 0;

	for (i = 0; test_table[i].output != NULL; i++) {
		if (test_table[i].input != NULL) {
			assert(strlen(test_table[i].input) < sizeof(testbuf));
			strcpy(testbuf, test_table[i].input);
			base = basename(testbuf);
		} else
			base = basename(NULL);

		/*
		 * basename(3) is allowed to modify the input buffer.
		 * However, that is considered hostile by some programs,
		 * and so we elect to consider this an error.
		 *
		 * This is not a problem, as basename(3) is also allowed
		 * to return a pointer to a statically-allocated buffer
		 * (it is explicitly not required to be reentrant).
		 */
		if (test_table[i].input != NULL &&
		    strcmp(test_table[i].input, testbuf) != 0) {
			fprintf(stderr,
			    "Input buffer for \"%s\" was modified\n",
			    test_table[i].input);
			rv = 1;
		}

		/* Make sure the result is correct. */
		if (strcmp(test_table[i].output, base) != 0) {
			fprintf(stderr,
			    "Input \"%s\", output \"%s\", expected \"%s\"\n",
			    test_table[i].input == NULL ? "(null)"
			    				: test_table[i].input,
			    base, test_table[i].output);
			rv = 1;
		}
	}

	exit(rv);
}
