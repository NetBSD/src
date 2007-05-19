/*	$NetBSD: recipient_list.h,v 1.1.1.6 2007/05/19 16:28:17 heas Exp $	*/

#ifndef _RECIPIENT_LIST_H_INCLUDED_
#define _RECIPIENT_LIST_H_INCLUDED_

/*++
/* NAME
/*	recipient_list 3h
/* SUMMARY
/*	recipient list structures
/* SYNOPSIS
/*	#include <recipient_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Information about a recipient is kept in this structure. The file offset
  * tells us the position of the REC_TYPE_RCPT byte in the message queue
  * file, This byte is replaced by REC_TYPE_DONE when the delivery status to
  * that recipient is established.
  *
  * Rather than bothering with subclasses that extend this structure with
  * application-specific fields we just add them here.
  */
typedef struct RECIPIENT {
    long    offset;			/* REC_TYPE_RCPT byte */
    const char *dsn_orcpt;		/* DSN original recipient */
    int     dsn_notify;			/* DSN notify flags */
    const char *orig_addr;		/* null or original recipient */
    const char *address;		/* complete address */
    union {				/* Application specific. */
	int     status;			/* SMTP client */
	struct QMGR_QUEUE *queue;	/* Queue manager */
	const char *addr_type;		/* DSN */
    }       u;
} RECIPIENT;

#define RECIPIENT_ASSIGN(rcpt, offs, orcpt, notify, orig, addr) do { \
    (rcpt)->offset = (offs); \
    (rcpt)->dsn_orcpt = (orcpt); \
    (rcpt)->dsn_notify = (notify); \
    (rcpt)->orig_addr = (orig); \
    (rcpt)->address = (addr); \
    (rcpt)->u.status = (0); \
} while (0)

#define RECIPIENT_UPDATE(ptr, new) do { \
    myfree((char *) (ptr)); (ptr) = mystrdup(new); \
} while (0)

typedef struct RECIPIENT_LIST {
    RECIPIENT *info;
    int     len;
    int     avail;
    int     variant;
} RECIPIENT_LIST;

extern void recipient_list_init(RECIPIENT_LIST *, int);
extern void recipient_list_add(RECIPIENT_LIST *, long, const char *, int, const char *, const char *);
extern void recipient_list_swap(RECIPIENT_LIST *, RECIPIENT_LIST *);
extern void recipient_list_free(RECIPIENT_LIST *);

#define RCPT_LIST_INIT_STATUS	1
#define RCPT_LIST_INIT_QUEUE	2
#define RCPT_LIST_INIT_ADDR	3

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
