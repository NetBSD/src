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
  * Client processing options. Flags 16- are reserved for cleanup.h.
  */
#define CLEANUP_FLAG_NONE	0	/* No special features */
#define CLEANUP_FLAG_BOUNCE	(1<<0)	/* Bounce bad messages */
#define CLEANUP_FLAG_FILTER	(1<<1)	/* Enable content filter */
#define CLEANUP_FLAG_HOLD	(1<<2)	/* Place message on hold */
#define CLEANUP_FLAG_DISCARD	(1<<3)	/* Discard message silently */
#define CLEANUP_FLAG_BCC_OK	(1<<4)	/* Ok to add auto-BCC addresses */
#define CLEANUP_FLAG_MAP_OK	(1<<5)	/* Ok to map addresses */

 /*
  * These are normally set when receiving mail from outside.
  */
#define CLEANUP_FLAG_MASK_EXTERNAL \
	(CLEANUP_FLAG_FILTER | CLEANUP_FLAG_BCC_OK | CLEANUP_FLAG_MAP_OK)

 /*
  * These are normally set when generating notices or when forwarding mail
  * internally.
  */
#define CLEANUP_FLAG_MASK_INTERNAL CLEANUP_FLAG_MAP_OK

 /*
  * These are set on the fly while processing SMTP envelopes or message
  * content.
  */
#define CLEANUP_FLAG_MASK_EXTRA \
	(CLEANUP_FLAG_HOLD | CLEANUP_FLAG_DISCARD)

 /*
  * Diagnostics.
  */
#define CLEANUP_STAT_OK		0	/* Success. */
#define CLEANUP_STAT_BAD	(1<<0)	/* Internal protocol error */
#define CLEANUP_STAT_WRITE	(1<<1)	/* Error writing message file */
#define CLEANUP_STAT_SIZE	(1<<2)	/* Message file too big */
#define CLEANUP_STAT_CONT	(1<<3)	/* Message content rejected */
#define CLEANUP_STAT_HOPS	(1<<4)	/* Too many hops */
#define CLEANUP_STAT_RCPT	(1<<6)	/* No recipients found */
#define CLEANUP_STAT_PROXY	(1<<7)	/* Proxy reject */

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

extern const char *cleanup_strerror(unsigned);
extern const char *cleanup_strflags(unsigned);

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
