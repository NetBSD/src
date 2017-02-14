/*	$NetBSD: msg_stats_print.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	msg_stats_print
/* SUMMARY
/*	write MSG_STATS structure to stream
/* SYNOPSIS
/*	#include <msg_stats.h>
/*
/*	int	msg_stats_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	msg_stats_print() writes an MSG_STATS structure to the named
/*	stream using the specified attribute print routine.
/*	msg_stats_print() is meant to be passed as a call-back to
/*	attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(msg_stats_print, (void *) stats), ...
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

#include <attr.h>

/* Global library. */

#include <mail_proto.h>
#include <msg_stats.h>

/* msg_stats_print - write MSG_STATS to stream */

int     msg_stats_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
			        int flags, void *ptr)
{
    int     ret;

    /*
     * Send the entire structure. This is not only simpler but also likely to
     * be quicker than having the sender figure out what fields need to be
     * sent, converting numbers to string and back, and having the receiver
     * initialize the unused fields by hand.
     */
    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_DATA(MAIL_ATTR_TIME, sizeof(MSG_STATS), ptr),
		   ATTR_TYPE_END);
    return (ret);
}
