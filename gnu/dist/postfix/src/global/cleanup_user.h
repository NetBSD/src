#ifndef _CLEANUP_USER_H_INCLUDED_
#define _CLEANUP_USER_H_INCLUDED_

/*++
/* NAME
/*	cleanup_user 3h
/* SUMMARY
/*	cleanup user interface codes
/* SYNOPSIS
/*	#include <cleanup_user.h>
/* DESCRIPTION
/* .nf

 /*
  * Options.
  */
#define CLEANUP_FLAG_NONE	0	/* No special features */
#define CLEANUP_FLAG_BOUNCE	(1<<0)	/* Bounce bad messages */
#define CLEANUP_FLAG_FILTER	(2<<0)	/* Enable content filter */

 /*
  * Diagnostics.
  */

#define CLEANUP_STAT_OK		0	/* Success. */

#define CLEANUP_STAT_BAD	(1<<0)	/* Internal protocol error */
#define CLEANUP_STAT_WRITE	(1<<1)	/* Error writing message file */
#define CLEANUP_STAT_SIZE	(1<<2)	/* Message file too big */
#define CLEANUP_STAT_CONT	(1<<3)	/* Message content rejected */
#define CLEANUP_STAT_HOPS	(1<<4)	/* Too many hops */
#define CLEANUP_STAT_SYN	(1<<5)	/* Bad address syntax */
#define CLEANUP_STAT_RCPT	(1<<6)	/* No recipients found */
#define CLEANUP_STAT_HOVFL	(1<<7)	/* Header overflow */
#define CLEANUP_STAT_ROVFL	(1<<8)	/* Recipient overflow */

 /*
  * These are set when we can't bounce even if we were asked to.
  */
#define CLEANUP_STAT_MASK_CANT_BOUNCE \
	(CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE)

 /*
  * These are set when we can't examine every record of a message.
  */
#define CLEANUP_STAT_MASK_INCOMPLETE \
	(CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE | CLEANUP_STAT_SIZE)

 /*
  * These are relevant for extracting recipients from headers.
  */
#define CLEANUP_STAT_MASK_EXTRACT_RCPT \
	(CLEANUP_STAT_HOVFL | CLEANUP_STAT_ROVFL | CLEANUP_STAT_RCPT)

extern const char *cleanup_strerror(unsigned);

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
