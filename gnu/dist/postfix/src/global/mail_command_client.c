/*++
/* NAME
/*	mail_command_client 3
/* SUMMARY
/*	single-command client
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_command_client(class, name, type, attr, ...)
/*	const char *class;
/*	const char *name;
/*	int	type;
/*	const char *attr;
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
/* .IP "type, attr, ..."
/*	Attribute information as defined in attr_print(3).
/* DIAGNOSTICS
/*	The result is -1 if the request could not be sent, otherwise
/*	the result is the status reported by the server.
/*	Warnings: problems connecting to the requested service.
/*	Fatal: out of memory.
/* SEE ALSO
/*	attr_print(3), send attributes over byte stream
/*	mail_command_server(3), server interface
/*	mail_proto(3h), client-server protocol
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

#include <mail_proto.h>

/* mail_command_client - single-command transaction with completion status */

int     mail_command_client(const char *class, const char *name,...)
{
    va_list ap;
    VSTREAM *stream;
    int     status;

    /*
     * Talk a little protocol with the specified service.
     */
    if ((stream = mail_connect(class, name, BLOCKING)) == 0)
	return (-1);
    va_start(ap, name);
    status = attr_vprint(stream, ATTR_FLAG_NONE, ap);
    va_end(ap);
    if (status != 0
	|| attr_scan(stream, ATTR_FLAG_STRICT,
		     ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status, 0) != 1)
	status = -1;
    (void) vstream_fclose(stream);
    return (status);
}
