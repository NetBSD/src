/*	$NetBSD: remove.c,v 1.1.1.1.10.1 2013/01/23 00:05:03 yamt Exp $	*/

/*++
/* NAME
/*	REMOVE 3
/* SUMMARY
/*	remove or stash away file
/* SYNOPSIS
/*	\fBint	REMOVE(path)\fR
/*	\fBconst char *path;\fR
/* DESCRIPTION
/*	\fBREMOVE()\fR removes a file, or renames it to a unique name,
/*	depending on the setting of the boolean \fBvar_dont_remove\fR
/*	flag.
/* DIAGNOSTICS
/*	The result is 0 in case of success, -1 in case of trouble.
/*	The global \fBerrno\fR variable reflects the nature of the
/*	problem.
/* FILES
/*	saved/*, stashed-away files.
/* SEE ALSO
/*	remove(3)
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
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <warn_stat.h>

/* Utility library. */

#include <vstring.h>

/* Global library. */

#include <mail_params.h>

/* REMOVE - squirrel away a file instead of removing it */

int     REMOVE(const char *path)
{
    static VSTRING *dest;
    char   *slash;
    struct stat st;

    if (var_dont_remove == 0) {
	return (remove(path));
    } else {
	if (dest == 0)
	    dest = vstring_alloc(10);
	vstring_sprintf(dest, "saved/%s", ((slash = strrchr(path, '/')) != 0) ?
			slash + 1 : path);
	for (;;) {
	    if (stat(vstring_str(dest), &st) < 0)
		break;
	    vstring_strcat(dest, "+");
	}
	return (rename(path, vstring_str(dest)));
    }
}
