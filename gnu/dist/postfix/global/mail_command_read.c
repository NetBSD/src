/*++
/* NAME
/*	mail_command_read 3
/* SUMMARY
/*	single-command server
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_command_read(stream, format, ...)
/*	VSTREAM	*stream;
/*	char	*format;
/* DESCRIPTION
/*	This module implements the server interface for single-command
/*	requests: a clients sends a single command and expects a single
/*	completion status code.
/*
/*	Arguments:
/* .IP stream
/*	Server endpoint.
/* .IP format
/*	Format string understood by mail_print(3) and mail_scan(3).
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* SEE ALSO
/*	mail_scan(3)
/*	mail_command_write(3) client interface
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
#include <stdlib.h>		/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include "mail_proto.h"

/* mail_command_read - read single-command request */

int     mail_command_read(VSTREAM *stream, char *fmt,...)
{
    VSTRING *eof = vstring_alloc(10);
    va_list ap;
    int     count;

    va_start(ap, fmt);
    count = mail_vscan(stream, fmt, ap);
    va_end(ap);
    if (mail_scan(stream, "%s", eof) != 1 || strcmp(vstring_str(eof), MAIL_EOF))
	count = -1;
    vstring_free(eof);
    return (count);
}
