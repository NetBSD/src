/*++
/* NAME
/*	username 3
/* SUMMARY
/*	lookup name of real user
/* SYNOPSIS
/*	#include <username.h>
/*
/*	const char *username()
/* DESCRIPTION
/*	username() jumps whatever system-specific hoops it takes to
/*	get the name of the user who started the process. The result
/*	is volatile. Make a copy if it is to be used for an appreciable
/*	amount of time.
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
#include <unistd.h>
#include <pwd.h>

/* Utility library. */

#include "username.h"

/* username - get name of user */

const char *username(void)
{
    uid_t   uid;
    struct passwd *pwd;

    uid = getuid();
    if ((pwd = getpwuid(uid)) == 0)
	return (0);
    return (pwd->pw_name);
}
