/*	$NetBSD: mail_command_server.c,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

/*++
/* NAME
/*	mail_command_server 3
/* SUMMARY
/*	single-command server
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_command_server(stream, type, name, ...)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *name;
/* DESCRIPTION
/*	This module implements the server interface for single-command
/*	requests: a clients sends a single command and expects a single
/*	completion status code.
/*
/*	Arguments:
/* .IP stream
/*	Server endpoint.
/* .IP "type, name, ..."
/*	Attribute list as defined in attr_scan(3).
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* SEE ALSO
/*	attr_scan(3)
/*	mail_command_client(3) client interface
/*	mail_proto(3h), client-server protocol
#include <mail_proto.h>
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

#include <vstream.h>

/* Global library. */

#include "mail_proto.h"

/* mail_command_server - read single-command request */

int     mail_command_server(VSTREAM *stream,...)
{
    va_list ap;
    int     count;

    va_start(ap, stream);
    count = attr_vscan(stream, ATTR_FLAG_MISSING, ap);
    va_end(ap);
    return (count);
}
