/*	$NetBSD: mbox_conf.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

#ifndef _MBOX_CONF_H_INCLUDED_
#define _MBOX_CONF_H_INCLUDED_

/*++
/* NAME
/*	mbox_conf 3h
/* SUMMARY
/*	mailbox lock configuration
/* SYNOPSIS
/*	#include <mbox_conf.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface.
  */
#define MBOX_FLOCK_LOCK		(1<<0)
#define MBOX_FCNTL_LOCK		(1<<1)
#define MBOX_DOT_LOCK		(1<<2)
#define MBOX_DOT_LOCK_MAY_FAIL	(1<<3)	/* XXX internal only */

extern int mbox_lock_mask(const char *);
extern ARGV *mbox_lock_names(void);

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
