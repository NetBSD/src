/*	$NetBSD: mkdir.c,v 1.3 2006/05/10 19:11:50 mrg Exp $	*/

/*
 * Written by Jason R. Thorpe, Jan 27, 2002.
 * Public domain.
 */

#include <sys/types.h>
#include <err.h>
#include <unistd.h>
#include <stdlib.h>

static const char *testdirnames[] = {
	/*
	 * IEEE 1003.1 second ed. 2.2.2.78:
	 *
	 *	If the pathname refers to a directory, it may also have
	 *	one or more trailing slashes.  Multiple successive slashes
	 *	are considered to be the same as one slash.
	 */
	"dir1/",
	"dir2//",

	NULL,
};

int	main(int, char *[]);

int
main(int argc, char *argv[])
{
	int i;

	(void) rmdir("foo");
	for (i = 0; testdirnames[i] != NULL; i++) {
		(void) rmdir(testdirnames[i]);
		if (mkdir(testdirnames[i], 0777))
			err(1, "mkdir(\"%s\", 0777)", testdirnames[i]);
		if (rename(testdirnames[i], "foo"))
			err(1, "rename(\"%s\", \"foo\")", testdirnames[i]);
		if (rename("foo/", testdirnames[i]))
			err(1, "rename(\"foo/\", \"%s\")", testdirnames[i]);
		if (rmdir(testdirnames[i]))
			err(1, "rmdir(\"%s\")", testdirnames[i]);
	}

	exit(0);
}
