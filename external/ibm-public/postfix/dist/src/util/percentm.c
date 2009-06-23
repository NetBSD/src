/*	$NetBSD: percentm.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	percentm 3
/* SUMMARY
/*	expand %m embedded in string to system error text
/* SYNOPSIS
/*	#include <percentm.h>
/*
/*	char	*percentm(const char *src, int err)
/* DESCRIPTION
/*	The percentm() routine makes a copy of the null-terminated string
/*	given via the \fIsrc\fR argument, with %m sequences replaced by
/*	the system error text corresponding to the \fIerr\fR argument.
/*	The result is overwritten upon each successive call.
/*
/*	Arguments:
/* .IP src
/*	A null-terminated input string with zero or more %m sequences.
/* .IP err
/*	A legal \fIerrno\fR value. The text corresponding to this error
/*	value is used when expanding %m sequences.
/* SEE ALSO
/*	syslog(3) system logger library
/* HISTORY
/* .ad
/* .fi
/*	A percentm() routine appears in the TCP Wrapper software
/*	by Wietse Venema.
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

/* System libraries. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include "vstring.h"
#include "percentm.h"

/* percentm - replace %m by error message corresponding to value in err */

char   *percentm(const char *str, int err)
{
    static VSTRING *vp;
    const unsigned char *ip = (const unsigned char *) str;

    if (vp == 0)
	vp = vstring_alloc(100);		/* grows on demand */
    VSTRING_RESET(vp);

    while (*ip) {
	switch (*ip) {
	default:
	    VSTRING_ADDCH(vp, *ip++);
	    break;
	case '%':
	    switch (ip[1]) {
	    default:				/* leave %<any> alone */
		VSTRING_ADDCH(vp, *ip++);
		/* FALLTHROUGH */
	    case '\0':				/* don't fall off end */
		VSTRING_ADDCH(vp, *ip++);
		break;
	    case 'm':				/* replace %m */
		vstring_strcat(vp, strerror(err));
		ip += 2;
		break;
	    }
	}
    }
    VSTRING_TERMINATE(vp);
    return (vstring_str(vp));
}

