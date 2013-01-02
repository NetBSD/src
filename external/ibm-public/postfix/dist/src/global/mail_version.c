/*	$NetBSD: mail_version.c,v 1.1.1.2 2013/01/02 18:58:58 tron Exp $	*/

/*++
/* NAME
/*	mail_version 3
/* SUMMARY
/*	time-dependent probe sender addresses
/* SYNOPSIS
/*	#include <mail_version.h>
/*
/*	typedef struct {
/*	    char   *program;		/* postfix */
/*	    int     major;		/* 2 */
/*	    int     minor;		/* 9 */
/*	    int     patch;		/* patchlevel or -1 */
/*	    char   *snapshot;		/* null or snapshot info */
/*	} MAIL_VERSION;
/*
/*	MAIL_VERSION *mail_version_parse(version_string, why)
/*	const char *version_string;
/*	const char **why;
/*
/*	void	mail_version_free(mp)
/*	MAIL_VERSION *mp;
/*
/*	const char *get_mail_version()
/*
/*	int	check_mail_version(version_string)
/*	const char *version_string;
/* DESCRIPTION
/*	This module understands the format of Postfix version strings
/*	(for example the default value of "mail_version"), and
/*	provides support to compare the compile-time version of a
/*	Postfix program with the run-time version of a Postfix
/*	library. Apparently, some distributions don't use proper
/*	so-number versioning, causing programs to fail erratically
/*	after an update replaces the library but not the program.
/*
/*	A Postfix version string consists of two or three parts
/*	separated by a single "-" character:
/* .IP \(bu
/*	The first part is a string with the program name.
/* .IP \(bu
/*	The second part is the program version: either two or three
/*	non-negative integer numbers separated by single "."
/*	character. Stable releases have a major version, minor
/*	version and patchlevel; experimental releases (snapshots)
/*	have only major and minor version numbers.
/* .IP \(bu
/*	The third part is ignored with a stable release, otherwise
/*	it is a string with the snapshot release date plus some
/*	optional information.
/*
/*	mail_version_parse() parses a version string.
/*
/*	get_mail_version() returns the version string (the value
/*	of DEF_MAIL_VERSION) that is compiled into the library.
/*
/*	check_mail_version() compares the caller's version string
/*	(usually the value of DEF_MAIL_VERSION) that is compiled
/*	into the caller, and logs a warning when the strings differ.
/* DIAGNOSTICS
/*	In the case of a parsing error, mail_version_parse() returns
/*	a null pointer, and sets the why argument to a string with
/*	problem details.
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
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <split_at.h>

/* Global library. */

#include <mail_version.h>

/* mail_version_int - convert integer */

static int mail_version_int(const char *strval)
{
    char   *end;
    int     intval;
    long    longval;

    errno = 0;
    intval = longval = strtol(strval, &end, 10);
    if (*strval == 0 || *end != 0 || errno == ERANGE || longval != intval)
	intval = (-1);
    return (intval);
}

/* mail_version_worker - do the parsing work */

static const char *mail_version_worker(MAIL_VERSION *mp, char *cp)
{
    char   *major_field;
    char   *minor_field;
    char   *patch_field;

    /*
     * Program name.
     */
    if ((mp->program = mystrtok(&cp, "-")) == 0)
	return ("no program name");

    /*
     * Major, minor, patchlevel. If this is a stable release, then we ignore
     * text after the patchlevel, in case there are vendor extensions.
     */
    if ((major_field = mystrtok(&cp, "-")) == 0)
	return ("missing major version");

    if ((minor_field = split_at(major_field, '.')) == 0)
	return ("missing minor version");
    if ((mp->major = mail_version_int(major_field)) < 0)
	return ("bad major version");
    patch_field = split_at(minor_field, '.');
    if ((mp->minor = mail_version_int(minor_field)) < 0)
	return ("bad minor version");

    if (patch_field == 0)
	mp->patch = -1;
    else if ((mp->patch = mail_version_int(patch_field)) < 0)
	return ("bad patchlevel");

    /*
     * Experimental release. If this is not a stable release, we take
     * everything to the end of the string.
     */
    if (patch_field != 0)
	mp->snapshot = 0;
    else if ((mp->snapshot = mystrtok(&cp, "")) == 0)
	return ("missing snapshot field");

    return (0);
}

/* mail_version_parse - driver */

MAIL_VERSION *mail_version_parse(const char *string, const char **why)
{
    MAIL_VERSION *mp;
    char   *saved_string;
    const char *err;

    mp = (MAIL_VERSION *) mymalloc(sizeof(*mp));
    saved_string = mystrdup(string);
    if ((err = mail_version_worker(mp, saved_string)) != 0) {
	*why = err;
	myfree(saved_string);
	myfree((char *) mp);
	return (0);
    } else {
	return (mp);
    }
}

/* mail_version_free - destroy version information */

void    mail_version_free(MAIL_VERSION *mp)
{
    myfree(mp->program);
    myfree((char *) mp);
}

/* get_mail_version - return parsed mail version string */

const char *get_mail_version(void)
{
    return (DEF_MAIL_VERSION);
}

/* check_mail_version - compare caller version with library version */

void    check_mail_version(const char *version_string)
{
    if (strcmp(version_string, DEF_MAIL_VERSION) != 0)
	msg_warn("Postfix library version mis-match: wanted %s, found %s",
		 version_string, DEF_MAIL_VERSION);
}

#ifdef TEST

#include <unistd.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>

#define STR(x) vstring_str(x)

/* parse_sample - parse a sample string from argv or stdin */

static void parse_sample(const char *sample)
{
    MAIL_VERSION *mp;
    const char *why;

    mp = mail_version_parse(sample, &why);
    if (mp == 0) {
	vstream_printf("ERROR: %s: %s\n", sample, why);
    } else {
	vstream_printf("program: %s\t", mp->program);
	vstream_printf("major: %d\t", mp->major);
	vstream_printf("minor: %d\t", mp->minor);
	if (mp->patch < 0)
	    vstream_printf("snapshot: %s\n", mp->snapshot);
	else
	    vstream_printf("patch: %d\n", mp->patch);
	mail_version_free(mp);
    }
    vstream_fflush(VSTREAM_OUT);
}

/* main - the main program */

int     main(int argc, char **argv)
{
    VSTRING *inbuf = vstring_alloc(1);
    int     have_tty = isatty(0);

    if (argc > 1) {
	while (--argc > 0 && *++argv)
	    parse_sample(*argv);
    } else {
	for (;;) {
	    if (have_tty) {
		vstream_printf("> ");
		vstream_fflush(VSTREAM_OUT);
	    }
	    if (vstring_fgets_nonl(inbuf, VSTREAM_IN) <= 0)
		break;
	    if (have_tty == 0)
		vstream_printf("> %s\n", STR(inbuf));
	    if (*STR(inbuf) == 0 || *STR(inbuf) == '#')
		continue;
	    parse_sample(STR(inbuf));
	}
    }
    vstring_free(inbuf);
    return (0);
}

#endif
