#ifndef _MAIL_COPY_H_INCLUDED_
#define _MAIL_COPY_H_INCLUDED_

/*++
/* NAME
/*	mail_copy 3h
/* SUMMARY
/*	copy message with extreme prejudice
/* SYNOPSIS
/*	#include <mail_copy.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
extern int mail_copy(const char *, const char *, VSTREAM *, VSTREAM *,
		             int, const char *, VSTRING *);

#define MAIL_COPY_QUOTE		(1<<0)	/* prepend > to From_ */
#define MAIL_COPY_TOFILE	(1<<1)	/* fsync, ftruncate() */
#define MAIL_COPY_FROM		(1<<2)	/* prepend From_ */
#define MAIL_COPY_DELIVERED	(1<<3)	/* prepend Delivered-To: */
#define MAIL_COPY_RETURN_PATH	(1<<4)	/* prepend Return-Path: */
#define MAIL_COPY_DOT		(1<<5)	/* escape dots - needed for bsmtp */
#define MAIL_COPY_BLANK		(1<<6)	/* append blank line */
#define MAIL_COPY_MBOX		(MAIL_COPY_FROM | MAIL_COPY_QUOTE | \
				    MAIL_COPY_TOFILE | MAIL_COPY_DELIVERED | \
				    MAIL_COPY_RETURN_PATH | MAIL_COPY_BLANK)
#define MAIL_COPY_NONE		0	/* all turned off */

#define MAIL_COPY_STAT_OK	0
#define MAIL_COPY_STAT_CORRUPT	(1<<0)
#define MAIL_COPY_STAT_READ	(1<<1)
#define MAIL_COPY_STAT_WRITE	(1<<2)

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
