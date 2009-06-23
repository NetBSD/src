/*	$NetBSD: strip_addr.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	strip_addr 3
/* SUMMARY
/*	strip extension from full or localpart-only address
/* SYNOPSIS
/*	#include <strip_addr.h>
/*
/*	char	*strip_addr(address, extension, delimiter)
/*	const char *address;
/*	char	**extension;
/*	int	delimiter;
/* DESCRIPTION
/*	strip_addr() takes an address and either returns a null
/*	pointer when the address contains no address extension,
/*	or returns a copy of the address without address extension.
/*	The caller is expected to pass the copy to myfree().
/*
/*	Arguments:
/* .IP address
/*	Address localpart or user@domain form.
/* .IP extension
/*	A null pointer, or the address of a pointer that is set to
/*	the address of a dynamic memory copy of the address extension
/*	that had to be chopped off.
/*	The copy includes the recipient address delimiter.
/*	The caller is expected to pass the copy to myfree().
/* .IP delimiter
/*	Recipient address delimiter.
/* SEE ALSO
/*	split_addr(3) strip extension from localpart
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
#include <string.h>

/* Utility library. */

#include <mymalloc.h>

/* Global library. */

#include <split_addr.h>
#include <strip_addr.h>

/* strip_addr - strip extension from address */

char   *strip_addr(const char *full, char **extension, int delimiter)
{
    char   *ratsign;
    char   *extent;
    char   *saved_ext;
    char   *stripped;

    /*
     * A quick test to eliminate inputs without delimiter anywhere.
     */
    if (delimiter == 0 || strchr(full, delimiter) == 0) {
	stripped = saved_ext = 0;
    } else {
	stripped = mystrdup(full);
	if ((ratsign = strrchr(stripped, '@')) != 0)
	    *ratsign = 0;
	if ((extent = split_addr(stripped, delimiter)) != 0) {
	    extent -= 1;
	    if (extension) {
		*extent = delimiter;
		saved_ext = mystrdup(extent);
		*extent = 0;
	    } else
		saved_ext = 0;
	    if (ratsign != 0) {
		*ratsign = '@';
		memmove(extent, ratsign, strlen(ratsign) + 1);
	    }
	} else {
	    myfree(stripped);
	    stripped = saved_ext = 0;
	}
    }
    if (extension)
	*extension = saved_ext;
    return (stripped);
}

#ifdef TEST

#include <msg.h>
#include <mail_params.h>

char   *var_double_bounce_sender = DEF_DOUBLE_BOUNCE;

int     main(int unused_argc, char **unused_argv)
{
    char   *extension;
    char   *stripped;
    int     delim = '-';

    /*
     * Incredible. This function takes only three arguments, and the tests
     * already take more lines of code than the code being tested.
     */
    stripped = strip_addr("foo", (char **) 0, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 1");

    stripped = strip_addr("foo", &extension, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 2");
    if (extension != 0)
	msg_panic("strip_addr botch 3");

    stripped = strip_addr("foo", (char **) 0, delim);
    if (stripped != 0)
	msg_panic("strip_addr botch 4");

    stripped = strip_addr("foo", &extension, delim);
    if (stripped != 0)
	msg_panic("strip_addr botch 5");
    if (extension != 0)
	msg_panic("strip_addr botch 6");

    stripped = strip_addr("foo@bar", (char **) 0, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 7");

    stripped = strip_addr("foo@bar", &extension, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 8");
    if (extension != 0)
	msg_panic("strip_addr botch 9");

    stripped = strip_addr("foo@bar", (char **) 0, delim);
    if (stripped != 0)
	msg_panic("strip_addr botch 10");

    stripped = strip_addr("foo@bar", &extension, delim);
    if (stripped != 0)
	msg_panic("strip_addr botch 11");
    if (extension != 0)
	msg_panic("strip_addr botch 12");

    stripped = strip_addr("foo-ext", (char **) 0, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 13");

    stripped = strip_addr("foo-ext", &extension, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 14");
    if (extension != 0)
	msg_panic("strip_addr botch 15");

    stripped = strip_addr("foo-ext", (char **) 0, delim);
    if (stripped == 0)
	msg_panic("strip_addr botch 16");
    msg_info("wanted:    foo-ext -> %s", "foo");
    msg_info("strip_addr foo-ext -> %s", stripped);
    myfree(stripped);

    stripped = strip_addr("foo-ext", &extension, delim);
    if (stripped == 0)
	msg_panic("strip_addr botch 17");
    if (extension == 0)
	msg_panic("strip_addr botch 18");
    msg_info("wanted:    foo-ext -> %s %s", "foo", "-ext");
    msg_info("strip_addr foo-ext -> %s %s", stripped, extension);
    myfree(stripped);
    myfree(extension);

    stripped = strip_addr("foo-ext@bar", (char **) 0, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 19");

    stripped = strip_addr("foo-ext@bar", &extension, 0);
    if (stripped != 0)
	msg_panic("strip_addr botch 20");
    if (extension != 0)
	msg_panic("strip_addr botch 21");

    stripped = strip_addr("foo-ext@bar", (char **) 0, delim);
    if (stripped == 0)
	msg_panic("strip_addr botch 22");
    msg_info("wanted:    foo-ext@bar -> %s", "foo@bar");
    msg_info("strip_addr foo-ext@bar -> %s", stripped);
    myfree(stripped);

    stripped = strip_addr("foo-ext@bar", &extension, delim);
    if (stripped == 0)
	msg_panic("strip_addr botch 23");
    if (extension == 0)
	msg_panic("strip_addr botch 24");
    msg_info("wanted:    foo-ext@bar -> %s %s", "foo@bar", "-ext");
    msg_info("strip_addr foo-ext@bar -> %s %s", stripped, extension);
    myfree(stripped);
    myfree(extension);

    return (0);
}

#endif
