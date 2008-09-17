#ifndef _RCPT_BUF_H_INCLUDED_
#define _RCPT_BUF_H_INCLUDED_

/*++
/* NAME
/*	rcpt_buf 3h
/* SUMMARY
/*	recipient buffer manager
/* SYNOPSIS
/*	#include <rcpt_buf.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <attr.h>

 /*
  * Global library.
  */
#include <recipient_list.h>

 /*
  * External interface.
  */
typedef struct {
    RECIPIENT rcpt;			/* convenience */
    VSTRING *address;			/* final recipient */
    VSTRING *orig_addr;			/* original recipient */
    VSTRING *dsn_orcpt;			/* dsn original recipient */
    int     dsn_notify;			/* DSN notify flags */
    long    offset;			/* REC_TYPE_RCPT byte */
} RCPT_BUF;

extern RCPT_BUF *rcpb_create(void);
extern void rcpb_reset(RCPT_BUF *);
extern void rcpb_free(RCPT_BUF *);
extern int rcpb_scan(ATTR_SCAN_MASTER_FN, VSTREAM *, int, void *);

#define RECIPIENT_FROM_RCPT_BUF(buf) \
    ((buf)->rcpt.address = vstring_str((buf)->address), \
     (buf)->rcpt.orig_addr = vstring_str((buf)->orig_addr), \
     (buf)->rcpt.dsn_orcpt = vstring_str((buf)->dsn_orcpt), \
     (buf)->rcpt.dsn_notify = (buf)->dsn_notify, \
     (buf)->rcpt.offset = (buf)->offset, \
     &(buf)->rcpt)

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

#endif
