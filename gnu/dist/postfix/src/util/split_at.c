/*++
/* NAME
/*	split_at 3
/* SUMMARY
/*	trivial token splitter
/* SYNOPSIS
/*	#include <split_at.h>
/*
/*	char	*split_at(string, delimiter)
/*	char	*string;
/*	int	delimiter
/*
/*	char	*split_at_right(string, delimiter)
/*	char	*string;
/*	int	delimiter
/* DESCRIPTION
/*	split_at() null-terminates the \fIstring\fR at the first
/*	occurrence of the \fIdelimiter\fR character found, and
/*	returns a pointer to the remainder.
/*
/*	split_at_right() looks for the rightmost delimiter
/*	occurrence, but is otherwise identical to split_at().
/* DIAGNOSTICS
/*	The result is a null pointer when the delimiter character
/*	was not found.
/* HISTORY
/* .ad
/* .fi
/*	A split_at() routine appears in the TCP Wrapper software
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

/* System libraries */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include "split_at.h"

/* split_at - break string at first delimiter, return remainder */

char   *split_at(char *string, int delimiter)
{
    char   *cp;

    if ((cp = strchr(string, delimiter)) != 0)
	*cp++ = 0;
    return (cp);
}

/* split_at_right - break string at last delimiter, return remainder */

char   *split_at_right(char *string, int delimiter)
{
    char   *cp;

    if ((cp = strrchr(string, delimiter)) != 0)
	*cp++ = 0;
    return (cp);
}
