/*++
/* NAME
/*	mail_command_write 3
/* SUMMARY
/*	single-command client
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_command_write(class, name, format, ...)
/*	const char *class;
/*	const char *name;
/*	const char *format;
/* DESCRIPTION
/*	This module implements a client interface for single-command
/*	clients: a client that sends a single command and expects
/*	a single completion status code.
/*
/*	Arguments:
/* .IP class
/*	Service type: MAIL_CLASS_PUBLIC or MAIL_CLASS_PRIVATE
/* .IP name
/*	Service name (master.cf).
/* .IP format
/*	Format string understood by mail_print(3).
/* DIAGNOSTICS
/*	The result is -1 if the request could not be sent, otherwise
/*	the result is the status reported by the server.
/*	Warnings: problems connecting to the requested service.
/*	Fatal: out of memory.
/* SEE ALSO
/*	mail_command_read(3), server interface
/*	mail_proto(5h), client-server protocol
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
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>

/* Utility library. */

#include <vstream.h>

/* Global library. */

#include "mail_proto.h"

/* mail_command_write - single-command transaction with completion status */

int     mail_command_write(const char *class, const char *name,
			           const char *fmt,...)
{
    va_list ap;
    VSTREAM *stream;
    int     status;

    /*
     * Talk a little protocol with the specified service.
     */
    if ((stream = mail_connect(class, name, BLOCKING)) == 0)
	return (-1);
    va_start(ap, fmt);
    status = mail_vprint(stream, fmt, ap);
    va_end(ap);
    if (status != 0
	|| mail_print(stream, "%s", MAIL_EOF) != 0
	|| vstream_fflush(stream) != 0
	|| mail_scan(stream, "%d", &status) != 1)
	status = -1;
    (void) vstream_fclose(stream);
    return (status);
}
