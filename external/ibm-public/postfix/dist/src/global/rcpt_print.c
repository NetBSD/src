/*	$NetBSD: rcpt_print.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	rcpt_print
/* SUMMARY
/*	write RECIPIENT structure to stream
/* SYNOPSIS
/*	#include <rcpt_print.h>
/*
/*	int     rcpt_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	rcpt_print() writes the contents of a RECIPIENT structure
/*	to the named stream using the specified attribute print
/*	routine. rcpt_print() is meant to be passed as a call-back
/*	to attr_print(), thusly:
/*
/*	... ATTR_TYPE_FUNC, rcpt_print, (void *) recipient, ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema IBM T.J. Watson Research P.O. Box 704 Yorktown
/*	Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <attr.h>

/* Global library. */

#include <mail_proto.h>
#include <recipient_list.h>
#include <rcpt_print.h>

/* rcpt_print - write recipient to stream */

int     rcpt_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
		           int flags, void *ptr)
{
    RECIPIENT *rcpt = (RECIPIENT *) ptr;
    int     ret;

    /*
     * The attribute order is determined by backwards compatibility. It can
     * be sanitized after all the ad-hoc recipient read/write code is
     * replaced.
     */
    ret =
	print_fn(fp, flags | ATTR_FLAG_MORE,
		 ATTR_TYPE_STR, MAIL_ATTR_ORCPT, rcpt->orig_addr,
		 ATTR_TYPE_STR, MAIL_ATTR_RECIP, rcpt->address,
		 ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, rcpt->offset,
		 ATTR_TYPE_STR, MAIL_ATTR_DSN_ORCPT, rcpt->dsn_orcpt,
		 ATTR_TYPE_INT, MAIL_ATTR_DSN_NOTIFY, rcpt->dsn_notify,
		 ATTR_TYPE_END);
    return (ret);
}
