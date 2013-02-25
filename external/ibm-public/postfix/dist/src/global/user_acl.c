/*	$NetBSD: user_acl.c,v 1.1.1.1.16.1 2013/02/25 00:27:20 tls Exp $	*/

/*++
/* NAME
/*	user_acl 3
/* SUMMARY
/*	user name based access control
/* SYNOPSIS
/*	#include <user_acl.h>
/*
/*	const char *check_user_acl_byuid(acl, uid)
/*	const char *acl;
/*	uid_t	uid;
/* DESCRIPTION
/*	check_user_acl_byuid() converts the given uid into a user
/*	name, and checks the result against a user name matchlist.
/*	If the uid cannot be resolved to a user name, "unknown"
/*	is used as the lookup key instead.
/*	The result is NULL on success, the username upon failure.
/*	The error result lives in static storage and must be saved
/*	if it is to be used to across multiple check_user_acl_byuid()
/*	calls.
/*
/*	Arguments:
/* .IP acl
/*	Authorized user name list suitable for input to string_list_init(3).
/* .IP uid
/*	The uid to be checked against the access list.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <dict_static.h>

/* Global library. */

#include <string_list.h>
#include <mypwd.h>

/* Application-specific. */

#include "user_acl.h"

/* check_user_acl_byuid - check user authorization */

const char *check_user_acl_byuid(char *acl, uid_t uid)
{
    struct mypasswd *mypwd;
    STRING_LIST *list;
    static VSTRING *who = 0;
    int     matched;
    const char *name;

    /*
     * Optimize for the most common case. This also makes Postfix a little
     * more robust in the face of local infrastructure failures. Note that we
     * only need to match the "static:" substring, not the result value.
     */
    if (strncmp(acl, DICT_TYPE_STATIC ":", sizeof(DICT_TYPE_STATIC)) == 0)
	return (0);

    /*
     * XXX: Substitute "unknown" for UIDs without username, so that
     * static:anyone results in "permit" even when the uid is not found in
     * the password file, and so that a pattern of !unknown can be used to
     * block non-existent accounts.
     * 
     * The alternative is to use the UID as a surrogate lookup key for
     * non-existent accounts. There are several reasons why this is not a
     * good idea. 1) An ACL with a numerical UID should work regardless of
     * whether or not an account has a password file entry. Therefore we
     * would always have search on the numerical UID whenever the username
     * fails to produce a match. 2) The string-list infrastructure is not
     * really suitable for mixing numerical and non-numerical user
     * information, because the numerical match is done in a separate pass
     * from the non-numerical match. This breaks when the ! operator is used.
     * 
     * XXX To avoid waiting until the lookup completes (e.g., LDAP or NIS down)
     * invoke mypwuid_err(), and either change the user_acl() API to
     * propagate the error to the caller, or treat lookup errors as fatal.
     */
    if ((mypwd = mypwuid(uid)) == 0) {
	name = "unknown";
    } else {
	name = mypwd->pw_name;
    }

    list = string_list_init(MATCH_FLAG_NONE, acl);
    if ((matched = string_list_match(list, name)) == 0) {
	if (!who)
	    who = vstring_alloc(10);
	vstring_strcpy(who, name);
    }
    string_list_free(list);
    if (mypwd)
	mypwfree(mypwd);

    return (matched ? 0 : vstring_str(who));
}
