/*++
/* NAME
/*	dir_forest 3
/* SUMMARY
/*	file name to directory forest
/* SYNOPSIS
/*	#include <dir_forest.h>
/*
/*	char	*dir_forest(buf, path, depth)
/*	VSTRING *buf;
/*	const char *path;
/*	int	depth;
/* DESCRIPTION
/*	This module implements support for directory forests: a file
/*	organization that introduces one or more levels of intermediate
/*	subdirectories in order to reduce the number of files per directory.
/*
/*	dir_forest() maps a file basename to a directory forest and
/*	returns the resulting string: file name "abcd" becomes "a/b/"
/*	and so on. The number of subdirectory levels is adjustable.
/*
/*	Arguments:
/* .IP buf
/*	A buffer that is overwritten with the result. The result
/*	ends in "/" and is null terminated. If a null pointer is
/*	specified, the result is written to a private buffer that
/*	is overwritten upon each call.
/* .IP path
/*	A null-terminated string of printable characters. Characters
/*	special to the file system are not permitted.
/*	The first subdirectory is named after the first character
/*	in \fIpath\fR, and so on. When the path is shorter than the
/*	desired number of subdirectory levels, directory names
/*	of '_' (underscore) are used as replacement.
/* .IP depth
/*	The desired number of subdirectory levels.
/* DIAGNOSTICS
/*	Panic: interface violations. Fatal error: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>

/* Utility library. */

#include "msg.h"
#include "dir_forest.h"

/* dir_forest - translate base name to directory forest */

char   *dir_forest(VSTRING *buf, const char *path, int depth)
{
    char   *myname = "dir_forest";
    static VSTRING *private_buf = 0;
    int     n;
    const char *cp;
    int     ch;

    /*
     * Sanity checks.
     */
    if (*path == 0)
	msg_panic("%s: empty path", myname);
    if (depth < 1)
	msg_panic("%s: depth %d", myname, depth);

    /*
     * Your buffer or mine?
     */
    if (buf == 0) {
	if (private_buf == 0)
	    private_buf = vstring_alloc(1);
	buf = private_buf;
    }

    /*
     * Generate one or more subdirectory levels, depending on the pathname
     * contents. When the pathname is short, use underscores instead.
     * Disallow non-printable characters or characters that are special to
     * the file system.
     */
    VSTRING_RESET(buf);
    for (cp = path, n = 0; n < depth; n++) {
	if ((ch = *cp) == 0) {
	    ch = '_';
	} else {
	    if (!ISPRINT(ch) || ch == '.' || ch == '/')
		msg_panic("%s: invalid pathname: %s", myname, path);
	    cp++;
	}
	VSTRING_ADDCH(buf, ch);
	VSTRING_ADDCH(buf, '/');
    }
    VSTRING_TERMINATE(buf);

    if (msg_verbose)
	msg_info("%s: %s -> %s", myname, path, vstring_str(buf));
    return (vstring_str(buf));
}
