/*	$NetBSD: dsb_scan.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	dsb_scan
/* SUMMARY
/*	read DSN_BUF from stream
/* SYNOPSIS
/*	#include <dsb_scan.h>
/*
/*	int	dsb_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	dsb_scan() reads a DSN_BUF from the named stream using the
/*	specified attribute scan routine. dsb_scan() is meant
/*	to be passed as a call-back to attr_scan(), thusly:
/*
/*	... RECV_ATTR_FUNC(dsb_scan, (void *) &dsbuf), ...
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
#include <dsb_scan.h>

/* dsb_scan - read DSN_BUF from stream */

int     dsb_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
		         int flags, void *ptr)
{
    DSN_BUF *dsb = (DSN_BUF *) ptr;
    int     ret;

    /*
     * The attribute order is determined by backwards compatibility. It can
     * be sanitized after all the ad-hoc DSN read/write code is replaced.
     */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  RECV_ATTR_STR(MAIL_ATTR_DSN_STATUS, dsb->status),
		  RECV_ATTR_STR(MAIL_ATTR_DSN_DTYPE, dsb->dtype),
		  RECV_ATTR_STR(MAIL_ATTR_DSN_DTEXT, dsb->dtext),
		  RECV_ATTR_STR(MAIL_ATTR_DSN_MTYPE, dsb->mtype),
		  RECV_ATTR_STR(MAIL_ATTR_DSN_MNAME, dsb->mname),
		  RECV_ATTR_STR(MAIL_ATTR_DSN_ACTION, dsb->action),
		  RECV_ATTR_STR(MAIL_ATTR_WHY, dsb->reason),
		  ATTR_TYPE_END);
    return (ret == 7 ? 1 : -1);
}
