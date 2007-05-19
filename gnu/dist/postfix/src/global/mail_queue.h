/*	$NetBSD: mail_queue.h,v 1.1.1.7 2007/05/19 16:28:13 heas Exp $	*/

#ifndef _MAIL_QUEUE_H_INCLUDED_
#define _MAIL_QUEUE_H_INCLUDED_

/*++
/* NAME
/*	mail_queue 3h
/* SUMMARY
/*	mail queue access
/* SYNOPSIS
/*	#include <mail_queue.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/time.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Mail queue names.
  */
#define MAIL_QUEUE_MAILDROP	"maildrop"
#define MAIL_QUEUE_HOLD		"hold"
#define MAIL_QUEUE_INCOMING	"incoming"
#define MAIL_QUEUE_ACTIVE	"active"
#define MAIL_QUEUE_DEFERRED	"deferred"
#define MAIL_QUEUE_TRACE	"trace"
#define MAIL_QUEUE_DEFER	"defer"
#define MAIL_QUEUE_BOUNCE	"bounce"
#define MAIL_QUEUE_CORRUPT	"corrupt"
#define MAIL_QUEUE_FLUSH	"flush"

 /*
  * Queue file modes.
  * 
  * 4.4BSD-like systems don't allow (sticky AND executable) together, so we use
  * group read permission bits instead. These are more portable, but they
  * also are more likely to be turned on by accident. It would not be the end
  * of the world.
  */
#define MAIL_QUEUE_STAT_READY	(S_IRUSR | S_IWUSR | S_IXUSR)
#define MAIL_QUEUE_STAT_CORRUPT	(S_IRUSR)
#ifndef MAIL_QUEUE_STAT_UNTHROTTLE
#define MAIL_QUEUE_STAT_UNTHROTTLE (S_IRGRP)
#endif

extern struct VSTREAM *mail_queue_enter(const char *, mode_t, struct timeval *);
extern struct VSTREAM *mail_queue_open(const char *, const char *, int, mode_t);
extern int mail_queue_rename(const char *, const char *, const char *);
extern int mail_queue_remove(const char *, const char *);
extern const char *mail_queue_dir(VSTRING *, const char *, const char *);
extern const char *mail_queue_path(VSTRING *, const char *, const char *);
extern int mail_queue_mkdirs(const char *);
extern int mail_queue_name_ok(const char *);
extern int mail_queue_id_ok(const char *);

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
