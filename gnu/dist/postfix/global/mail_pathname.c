/*++
/* NAME
/*	mail_pathname 3
/* SUMMARY
/*	generate pathname from mailer service class and name
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	char	*mail_pathname(service_class, service_name)
/*	char	*service_class;
/*	char	*service_name;
/* DESCRIPTION
/*	mail_pathname() translates the specified service class and name
/*	to a pathname. The result should be passed to myfree() when it
/*	no longer needed.
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

/* Utility library. */

#include <stringops.h>

/* Global library. */

#include "mail_proto.h"

/* mail_pathname - map service class and service name to pathname */

char   *mail_pathname(const char *service_class, const char *service_name)
{
    return (concatenate(service_class, "/", service_name, (char *) 0));
}
