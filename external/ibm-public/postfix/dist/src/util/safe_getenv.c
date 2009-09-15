/*	$NetBSD: safe_getenv.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	safe_getenv 3
/* SUMMARY
/*	guarded getenv()
/* SYNOPSIS
/*	#include <safe.h>
/*
/*	char	*safe_getenv(const name)
/*	char	*name;
/* DESCRIPTION
/*	The \fBsafe_getenv\fR() routine reads the named variable from the
/*	environment, provided that the unsafe() routine agrees.
/* SEE ALSO
/*	unsafe(3), detect non-user privileges
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
#include <stdlib.h>

/* Utility library. */

#include "safe.h"

/* safe_getenv - read environment variable with guard */

char   *safe_getenv(const char *name)
{
    return (unsafe() == 0 ? getenv(name) : 0);
}
