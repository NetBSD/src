/*++
/* NAME
/*	sys_exits 3
/* SUMMARY
/*	sendmail-compatible exit status handling
/* SYNOPSIS
/*	#include <sys_exits.h>
/*
/*	int	SYS_EXITS_CODE(code)
/*	int	code;
/*
/*	const char *sys_exits_strerror(code)
/*	int	code;
/*
/*	int	sys_exits_softerror(code)
/*	int	code;
/* DESCRIPTION
/*	This module interprets sendmail-compatible process exit status
/*	codes. A default result is returned for other exit codes.
/*
/*	SYS_EXITS_CODE() returns non-zero when the specified code
/*	is a sendmail-compatible process exit status code.
/*
/*	sys_exits_strerror() returns a descriptive text for the
/*	specified sendmail-compatible status code.
/*
/*	sys_exits_softerror() returns non-zero when the specified
/*	sendmail-compatible status code corresponds to a recoverable error.
/* DIAGNOSTICS
/*	Panic: invalid status code. Fatal error: out of memory.
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

/* Global library. */

#include <sys_exits.h>

/* Application-specific. */

typedef struct {
    int     flags;			/* non-zero if recoverable */
    int     code;			/* exit status code */
    const char *text;			/* descriptive text */
} SYS_EXITS_TABLE;

static SYS_EXITS_TABLE sys_exits_table[] = {
    0, EX_USAGE, "command line usage error",
    0, EX_DATAERR, "data format error",
    0, EX_NOINPUT, "cannot open input",
    0, EX_NOUSER, "user unknown",
    0, EX_NOHOST, "host name unknown",
    0, EX_UNAVAILABLE, "service unavailable",
    0, EX_SOFTWARE, "internal software error",
    1, EX_OSERR, "system resource problem",
    0, EX_OSFILE, "critical OS file missing",
    0, EX_CANTCREAT, "can't create user output file",
    0, EX_IOERR, "input/output error",
    1, EX_TEMPFAIL, "temporary failure",
    0, EX_PROTOCOL, "remote error in protocol",
    0, EX_NOPERM, "permission denied",
    0, EX_CONFIG, "local configuration error",
};

/* sys_exits_strerror - map exit status to error string */

const char *sys_exits_strerror(int code)
{
    char   *myname = "sys_exits_strerror";

    if (!SYS_EXITS_CODE(code))
	msg_panic("%s: bad code: %d", myname, code);

    return (sys_exits_table[code - EX__BASE].text);
}

/* sys_exits_softerror  - determine if error is transient */

int     sys_exits_softerror(int code)
{
    char   *myname = "sys_exits_softerror";

    if (!SYS_EXITS_CODE(code))
	msg_panic("%s: bad code: %d", myname, code);

    return (sys_exits_table[code - EX__BASE].flags);
}
