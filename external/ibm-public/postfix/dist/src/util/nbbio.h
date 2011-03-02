/*	$NetBSD: nbbio.h,v 1.1.1.1 2011/03/02 19:32:44 tron Exp $	*/

#ifndef _NBBIO_H_INCLUDED_
#define _NBBIO_H_INCLUDED_

/*++
/* NAME
/*	nbbio 3h
/* SUMMARY
/*	non-blocking buffered I/O
/* SYNOPSIS
/*	#include "nbbio.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <events.h>			/* Needed for EVENT_READ etc. */

 /*
  * External interface. All structure members are private.
  */
typedef void (*NBBIO_ACTION) (int, char *);

typedef struct {
    int     fd;				/* socket file descriptor */
    ssize_t bufsize;			/* read/write buffer size */
    char   *label;			/* diagnostics */
    NBBIO_ACTION action;		/* call-back routine */
    char   *context;			/* call-back context */
    int     flags;			/* buffer-pair status */

    char   *read_buf;			/* start of buffer */
    ssize_t read_pend;			/* nr of unread bytes */

    char   *write_buf;			/* start of buffer */
    ssize_t write_pend;			/* nr of unwritten bytes */
} NBBIO;

#define NBBIO_FLAG_READ		(1<<0)
#define NBBIO_FLAG_WRITE	(1<<1)
#define NBBIO_FLAG_EOF		(1<<2)
#define NBBIO_FLAG_ERROR	(1<<3)
#define NBBIO_FLAG_TIMEOUT	(1<<4)

#define NBBIO_OP_NAME(np) \
	(((np)->flags & NBBIO_FLAG_READ) ? "read" : \
	 ((np)->flags & NBBIO_FLAG_WRITE) ? "write" : \
	 "unknown")

#define NBBIO_MASK_ACTIVE \
	(NBBIO_FLAG_READ | NBBIO_FLAG_WRITE)

#define NBBIO_MASK_ERROR \
	(NBBIO_FLAG_EOF | NBBIO_FLAG_ERROR | NBBIO_FLAG_TIMEOUT)

#define NBBIO_BUFSIZE(np)		(((np)->bufsize) + 0)	/* Read-only */

#define NBBIO_READ_PEND(np)		((np)->read_pend)
#define NBBIO_READ_BUF(np)		((np)->read_buf + 0)	/* Read-only */

#define NBBIO_WRITE_PEND(np)		((np)->write_pend)
#define NBBIO_WRITE_BUF(np)		((np)->write_buf + 0)	/* Read-only */

#define NBBIO_ACTIVE_FLAGS(np)		((np)->flags & NBBIO_MASK_ACTIVE)
#define NBBIO_ERROR_FLAGS(np)		((np)->flags & NBBIO_MASK_ERROR)

extern NBBIO *nbbio_create(int, ssize_t, const char *, NBBIO_ACTION, char *);
extern void nbbio_free(NBBIO *);
extern void nbbio_enable_read(NBBIO *, int);
extern void nbbio_enable_write(NBBIO *, int);
extern void nbbio_disable_readwrite(NBBIO *);
extern void nbbio_slumber(NBBIO *, int);

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
