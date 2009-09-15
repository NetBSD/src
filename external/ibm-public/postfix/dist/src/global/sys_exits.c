/*	$NetBSD: sys_exits.c,v 1.1.1.1.2.2 2009/09/15 06:02:54 snj Exp $	*/

/*++
/* NAME
/*	sys_exits 3
/* SUMMARY
/*	sendmail-compatible exit status handling
/* SYNOPSIS
/*	#include <sys_exits.h>
/*
/*	typedef struct {
/* .in +4
/*	    int   status;	/* exit status */
/*	    const char *dsn;	/* RFC 3463 */
/*	    const char *text;	/* free text */
/* .in -4
/*	} SYS_EXITS_DETAIL;
/*
/*	int	SYS_EXITS_CODE(code)
/*	int	code;
/*
/*	const char *sys_exits_strerror(code)
/*	int	code;
/*
/*	const SYS_EXITS_DETAIL *sys_exits_detail(code)
/*	int	code;
/*
/*	int	sys_exits_softerror(code)
/*	int	code;
/* DESCRIPTION
/*	This module interprets sendmail-compatible process exit status
/*	codes.
/*
/*	SYS_EXITS_CODE() returns non-zero when the specified code
/*	is a sendmail-compatible process exit status code.
/*
/*	sys_exits_strerror() returns a descriptive text for the
/*	specified sendmail-compatible status code, or a generic
/*	text for an unknown status code.
/*
/*	sys_exits_detail() returns a table entry with assorted
/*	information about the specified sendmail-compatible status
/*	code, or a generic entry for an unknown status code.
/*
/*	sys_exits_softerror() returns non-zero when the specified
/*	sendmail-compatible status code corresponds to a recoverable error.
/*	An unknown status code is always unrecoverable.
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <sys_exits.h>

/* Application-specific. */

static const SYS_EXITS_DETAIL sys_exits_table[] = {
    EX_USAGE, "5.3.0", "command line usage error",
    EX_DATAERR, "5.6.0", "data format error",
    EX_NOINPUT, "5.3.0", "cannot open input",
    EX_NOUSER, "5.1.1", "user unknown",
    EX_NOHOST, "5.1.2", "host name unknown",
    EX_UNAVAILABLE, "5.3.0", "service unavailable",
    EX_SOFTWARE, "5.3.0", "internal software error",
    EX_OSERR, "4.3.0", "system resource problem",
    EX_OSFILE, "5.3.0", "critical OS file missing",
    EX_CANTCREAT, "5.2.0", "can't create user output file",
    EX_IOERR, "5.3.0", "input/output error",
    EX_TEMPFAIL, "4.3.0", "temporary failure",
    EX_PROTOCOL, "5.5.0", "remote error in protocol",
    EX_NOPERM, "5.7.0", "permission denied",
    EX_CONFIG, "5.3.5", "local configuration error",
};

static VSTRING *sys_exits_def_text = 0;

static SYS_EXITS_DETAIL sys_exits_default[] = {
    0, "5.3.0", 0,
};

/* sys_exits_fake - fake an entry for an unknown code */

static SYS_EXITS_DETAIL *sys_exits_fake(int code)
{
    if (sys_exits_def_text == 0)
	sys_exits_def_text = vstring_alloc(30);

    vstring_sprintf(sys_exits_def_text, "unknown mail system error %d", code);
    sys_exits_default->text = vstring_str(sys_exits_def_text);
    return(sys_exits_default);
}

/* sys_exits_strerror - map exit status to error string */

const char *sys_exits_strerror(int code)
{
    if (!SYS_EXITS_CODE(code)) {
	return (sys_exits_fake(code)->text);
    } else {
	return (sys_exits_table[code - EX__BASE].text);
    }
}

/* sys_exits_detail - map exit status info table entry */

const SYS_EXITS_DETAIL *sys_exits_detail(int code)
{
    if (!SYS_EXITS_CODE(code)) {
	return (sys_exits_fake(code));
    } else {
	return (sys_exits_table + code - EX__BASE);
    }
}

/* sys_exits_softerror  - determine if error is transient */

int     sys_exits_softerror(int code)
{
    if (!SYS_EXITS_CODE(code)) {
	return (sys_exits_default->dsn[0] == '4');
    } else {
	return (sys_exits_table[code - EX__BASE].dsn[0] == '4');
    }
}
