/*++
/* NAME
/*	fullname 3
/* SUMMARY
/*	lookup personal name of invoking user
/* SYNOPSIS
/*	#include <fullname.h>
/*
/*	const char *fullname()
/* DESCRIPTION
/*	fullname() looks up the personal name of the invoking user.
/*	The result is volatile. Make a copy if it is to be used for
/*	an appreciable amount of time.
/*
/*	On UNIX systems, fullname() first tries to use the NAME environment
/*	variable, provided that the environment can be trusted.
/*	If that fails, fullname() extracts the username from the GECOS
/*	field of the user's password-file entry, replacing any occurrence
/*	of "&" by the login name, first letter capitalized.
/*
/*	A null result means that no full name information was found.
/* SEE ALSO
/*	safe_getenv(3) safe getenv() interface
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include "vstring.h"
#include "safe.h"
#include "fullname.h"

/* fullname - get name of user */

const char *fullname(void)
{
    static VSTRING *result;
    char   *cp;
    int     ch;
    uid_t   uid;
    struct passwd *pwd;

    if (result == 0)
	result = vstring_alloc(10);

    /*
     * Try the environment.
     */
    if ((cp = safe_getenv("NAME")) != 0)
	return (vstring_str(vstring_strcpy(result, cp)));

    /*
     * Try the password file database.
     */
    uid = getuid();
    if ((pwd = getpwuid(uid)) == 0)
	return (0);

    /*
     * Replace all `&' characters by the login name of this user, first
     * letter capitalized. Although the full name comes from the protected
     * password file, the actual data is specified by the user so we should
     * not trust its sanity.
     */
    VSTRING_RESET(result);
    for (cp = pwd->pw_gecos; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ch == ',' || ch == ';' || ch == '%')
	    break;
	if (ch == '&') {
	    if (pwd->pw_name[0]) {
		VSTRING_ADDCH(result, TOUPPER(pwd->pw_name[0]));
		vstring_strcat(result, pwd->pw_name + 1);
	    }
	} else {
	    VSTRING_ADDCH(result, ch);
	}
    }
    VSTRING_TERMINATE(result);
    return (vstring_str(result));
}

#ifdef TEST

#include <stdio.h>

int     main(int unused_argc, char **unused_argv)
{
    const char *cp = fullname();

    printf("%s\n", cp ? cp : "null!");
}

#endif
