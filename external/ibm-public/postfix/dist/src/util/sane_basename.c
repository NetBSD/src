/*	$NetBSD: sane_basename.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	sane_basename 3
/* SUMMARY
/*	split pathname into last component and parent directory
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*sane_basename(buf, path)
/*	VSTRING	*buf;
/*	const char *path;
/*
/*	char	*sane_dirname(buf, path)
/*	VSTRING	*buf;
/*	const char *path;
/* DESCRIPTION
/*	These functions split a pathname into its last component
/*	and its parent directory, excluding any trailing "/"
/*	characters from the input. The result is a pointer to "/"
/*	when the input is all "/" characters, or a pointer to "."
/*	when the input is a null pointer or zero-length string.
/*
/*	sane_basename() and sane_dirname() differ as follows
/*	from standard basename() and dirname() implementations:
/* .IP \(bu
/*	They can use caller-provided storage or private storage.
/* .IP \(bu
/*	They never modify their input.
/* .PP
/*	sane_basename() returns a pointer to string with the last
/*	pathname component.
/*
/*	sane_dirname() returns a pointer to string with the parent
/*	directory. The result is a pointer to "." when the input
/*	contains no '/' character.
/*
/*	Arguments:
/* .IP buf
/*	Result storage. If a null pointer is specified, each function
/*	uses its own private memory that is overwritten upon each call.
/* .IP path
/*	The input pathname.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <stringops.h>

#define STR(x)	vstring_str(x)

/* sane_basename - skip directory prefix */

char   *sane_basename(VSTRING *bp, const char *path)
{
    static VSTRING *buf;
    const char *first;
    const char *last;

    /*
     * Your buffer or mine?
     */
    if (bp == 0) {
	bp = buf;
	if (bp == 0)
	    bp = buf = vstring_alloc(10);
    }

    /*
     * Special case: return "." for null or zero-length input.
     */
    if (path == 0 || *path == 0)
	return (STR(vstring_strcpy(bp, ".")));

    /*
     * Remove trailing '/' characters from input. Return "/" if input is all
     * '/' characters.
     */
    last = path + strlen(path) - 1;
    while (*last == '/') {
	if (last == path)
	    return (STR(vstring_strcpy(bp, "/")));
	last--;
    }

    /*
     * The pathname does not end in '/'. Skip to last '/' character if any.
     */
    first = last - 1;
    while (first >= path && *first != '/')
	first--;

    return (STR(vstring_strncpy(bp, first + 1, last - first)));
}

/* sane_dirname - keep directory prefix */

char   *sane_dirname(VSTRING *bp, const char *path)
{
    static VSTRING *buf;
    const char *last;

    /*
     * Your buffer or mine?
     */
    if (bp == 0) {
	bp = buf;
	if (bp == 0)
	    bp = buf = vstring_alloc(10);
    }

    /*
     * Special case: return "." for null or zero-length input.
     */
    if (path == 0 || *path == 0)
	return (STR(vstring_strcpy(bp, ".")));

    /*
     * Remove trailing '/' characters from input. Return "/" if input is all
     * '/' characters.
     */
    last = path + strlen(path) - 1;
    while (*last == '/') {
	if (last == path)
	    return (STR(vstring_strcpy(bp, "/")));
	last--;
    }

    /*
     * This pathname does not end in '/'. Skip to last '/' character if any.
     */
    while (last >= path && *last != '/')
	last--;
    if (last < path)				/* no '/' */
	return (STR(vstring_strcpy(bp, ".")));

    /*
     * Strip trailing '/' characters from dirname (not strictly needed).
     */
    while (last > path && *last == '/')
	last--;

    return (STR(vstring_strncpy(bp, path, last - path + 1)));
}

#ifdef TEST
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(10);
    char   *dir;
    char   *base;

    while (vstring_get_nonl(buf, VSTREAM_IN) > 0) {
	dir = sane_dirname((VSTRING *) 0, STR(buf));
	base = sane_basename((VSTRING *) 0, STR(buf));
	vstream_printf("input=\"%s\" dir=\"%s\" base=\"%s\"\n",
		       STR(buf), dir, base);
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(buf);
    return (0);
}

#endif
