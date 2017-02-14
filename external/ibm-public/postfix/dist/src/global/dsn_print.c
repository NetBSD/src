/*	$NetBSD: dsn_print.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	dsn_print
/* SUMMARY
/*	write DSN structure to stream
/* SYNOPSIS
/*	#include <dsn_print.h>
/*
/*	int	dsn_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	dsn_print() writes a DSN structure to the named stream using
/*	the specified attribute print routine. dsn_print() is meant
/*	to be passed as a call-back to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(dsn_print, (void *) dsn), ...
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
#include <dsn_print.h>

/* dsn_print - write DSN to stream */

int     dsn_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
		          int flags, void *ptr)
{
    DSN    *dsn = (DSN *) ptr;
    int     ret;

    /*
     * The attribute order is determined by backwards compatibility. It can
     * be sanitized after all the ad-hoc DSN read/write code is replaced.
     */
    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_STR(MAIL_ATTR_DSN_STATUS, dsn->status),
		   SEND_ATTR_STR(MAIL_ATTR_DSN_DTYPE, dsn->dtype),
		   SEND_ATTR_STR(MAIL_ATTR_DSN_DTEXT, dsn->dtext),
		   SEND_ATTR_STR(MAIL_ATTR_DSN_MTYPE, dsn->mtype),
		   SEND_ATTR_STR(MAIL_ATTR_DSN_MNAME, dsn->mname),
		   SEND_ATTR_STR(MAIL_ATTR_DSN_ACTION, dsn->action),
		   SEND_ATTR_STR(MAIL_ATTR_WHY, dsn->reason),
		   ATTR_TYPE_END);
    return (ret);
}
