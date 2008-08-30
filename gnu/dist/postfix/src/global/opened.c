/*++
/* NAME
/*	opened 3
/* SUMMARY
/*	log that a message was opened
/* SYNOPSIS
/*	#include <opened.h>
/*
/*	void	opened(queue_id, sender, size, nrcpt, format, ...)
/*	const char *queue_id;
/*	const char *sender;
/*	long	size;
/*	int	nrcpt;
/*	const char *format;
/* DESCRIPTION
/*	opened() logs that a message was successfully delivered.
/*
/*	vopened() implements an alternative interface.
/*
/*	Arguments:
/* .IP queue_id
/*	Message queue ID.
/* .IP sender
/*	Sender address.
/* .IP size
/*	Message content size.
/* .IP nrcpt
/*	Number of recipients.
/* .IP format
/*	Format of optional text.
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* BUGS
/*	Should be replaced by routines with an attribute-value based
/*	interface instead of an interface that uses a rigid argument list.
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

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include "opened.h"

/* opened - log that a message was opened */

void    opened(const char *queue_id, const char *sender, long size, int nrcpt,
	               const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vopened(queue_id, sender, size, nrcpt, fmt, ap);
    va_end(ap);
}

/* vopened - log that a message was opened */

void    vopened(const char *queue_id, const char *sender, long size, int nrcpt,
		        const char *fmt, va_list ap)
{
    VSTRING *text = vstring_alloc(100);

#define TEXT (vstring_str(text))

    vstring_vsprintf(text, fmt, ap);
    msg_info("%s: from=<%s>, size=%ld, nrcpt=%d%s%s%s",
	     queue_id, sender, size, nrcpt,
	     *TEXT ? " (" : "", TEXT, *TEXT ? ")" : "");
    vstring_free(text);
}
