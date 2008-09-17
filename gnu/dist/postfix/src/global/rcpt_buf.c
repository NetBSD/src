/*++
/* NAME
/*	rcpt_buf
/* SUMMARY
/*	recipient buffer manager
/* SYNOPSIS
/*	#include <rcpt_buf.h>
/*
/*	typedef struct {
/*		RECIPIENT rcpt;		/* convenience */
/* .in +4
/*		VSTRING *address;	/* final recipient */
/*		VSTRING *orig_addr;	/* original recipient */
/*		VSTRING *dsn_orcpt;	/* dsn original recipient */
/*		int     dsn_notify;	/* DSN notify flags */
/*		long    offset;		/* REC_TYPE_RCPT byte */
/* .in -4
/*	} RCPT_BUF;
/*
/*	RECIPIENT *RECIPIENT_FROM_RCPT_BUF(rcpb)
/*	RCPT_BUF *rcpb;
/*
/*	RCPT_BUF *rcpb_create(void)
/*
/*	void	rcpb_reset(rcpb)
/*	RCPT_BUF *rcpb;
/*
/*	void	rcpb_free(rcpb)
/*	RCPT_BUF *rcpb;
/*
/*	int	rcpb_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_MASTER_FN scan_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	RECIPIENT_FROM_RCPT_BUF() populates the rcpt member with
/*	a shallow copy of the contents of the other fields.
/*
/*	rcpb_scan() reads a recipient buffer from the named stream
/*	using the specified attribute scan routine. rcpb_scan()
/*	is meant to be passed as a call-back to attr_scan(), thusly:
/*
/*	... ATTR_TYPE_FUNC, rcpb_scan, (void *) rcpt_buf, ...
/*
/*	rcpb_create(), rcpb_reset() and rcpb_free() create, wipe
/*	and destroy recipient buffer instances.
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

/* Syste, library. */

#include <sys_defs.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include <mail_proto.h>
#include <rcpt_buf.h>

/* Application-specific. */

/* rcpb_create - create recipient buffer */

RCPT_BUF *rcpb_create(void)
{
    RCPT_BUF *rcpt;

    rcpt = (RCPT_BUF *) mymalloc(sizeof(*rcpt));
    rcpt->offset = 0;
    rcpt->dsn_orcpt = vstring_alloc(10);
    rcpt->dsn_notify = 0;
    rcpt->orig_addr = vstring_alloc(10);
    rcpt->address = vstring_alloc(10);
    return (rcpt);
}

/* rcpb_reset - reset recipient buffer */

void    rcpb_reset(RCPT_BUF *rcpt)
{
#define BUF_TRUNCATE(s) (vstring_str(s)[0] = 0)

    rcpt->offset = 0;
    BUF_TRUNCATE(rcpt->dsn_orcpt);
    rcpt->dsn_notify = 0;
    BUF_TRUNCATE(rcpt->orig_addr);
    BUF_TRUNCATE(rcpt->address);
}

/* rcpb_free - destroy recipient buffer */

void    rcpb_free(RCPT_BUF *rcpt)
{
    vstring_free(rcpt->dsn_orcpt);
    vstring_free(rcpt->orig_addr);
    vstring_free(rcpt->address);
    myfree((char *) rcpt);
}

/* rcpb_scan - receive recipient buffer */

int     rcpb_scan(ATTR_SCAN_MASTER_FN scan_fn, VSTREAM *fp,
		          int flags, void *ptr)
{
    RCPT_BUF *rcpt = (RCPT_BUF *) ptr;
    int     ret;

    /*
     * The order of attributes is determined by historical compatibility and
     * can be fixed after all the ad-hoc read/write code is replaced.
     */
    ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		  ATTR_TYPE_STR, MAIL_ATTR_ORCPT, rcpt->orig_addr,
		  ATTR_TYPE_STR, MAIL_ATTR_RECIP, rcpt->address,
		  ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, &rcpt->offset,
		  ATTR_TYPE_STR, MAIL_ATTR_DSN_ORCPT, rcpt->dsn_orcpt,
		  ATTR_TYPE_INT, MAIL_ATTR_DSN_NOTIFY, &rcpt->dsn_notify,
		  ATTR_TYPE_END);
    return (ret == 5 ? 1 : -1);
}
