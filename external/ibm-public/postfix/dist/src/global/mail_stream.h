/*	$NetBSD: mail_stream.h,v 1.1.1.1.2.2 2009/09/15 06:02:46 snj Exp $	*/

#ifndef _MAIL_STREAM_H_INCLUDED_
#define _MAIL_STREAM_H_INCLUDED_

/*++
/* NAME
/*	mail_stream 3h
/* SUMMARY
/*	mail stream management
/* SYNOPSIS
/*	#include <mail_stream.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/time.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
typedef struct MAIL_STREAM MAIL_STREAM;

typedef int (*MAIL_STREAM_FINISH_FN) (MAIL_STREAM *, VSTRING *);
typedef int (*MAIL_STREAM_CLOSE_FN) (VSTREAM *);

struct MAIL_STREAM {
    VSTREAM *stream;			/* file or pipe or socket */
    char   *queue;			/* (initial) queue name */
    char   *id;				/* queue id */
    MAIL_STREAM_FINISH_FN finish;	/* finish code */
    MAIL_STREAM_CLOSE_FN close;		/* close stream */
    char   *class;			/* trigger class */
    char   *service;			/* trigger service */
    int     mode;			/* additional permissions */
#ifdef DELAY_ACTION
    int     delay;			/* deferred delivery */
#endif
    struct timeval ctime;		/* creation time */
};

#define MAIL_STREAM_CTL_END	0	/* Terminator */
#define MAIL_STREAM_CTL_QUEUE	1	/* Change queue */
#define MAIL_STREAM_CTL_CLASS	2	/* Change notification class */
#define MAIL_STREAM_CTL_SERVICE	3	/* Change notification service */
#define MAIL_STREAM_CTL_MODE	4	/* Change final queue file mode */
#ifdef DELAY_ACTION
#define MAIL_STREAM_CTL_DELAY	5	/* Change final queue file mtime */
#endif

extern MAIL_STREAM *mail_stream_file(const char *, const char *, const char *, int);
extern MAIL_STREAM *mail_stream_service(const char *, const char *);
extern MAIL_STREAM *mail_stream_command(const char *);
extern void mail_stream_cleanup(MAIL_STREAM *);
extern int mail_stream_finish(MAIL_STREAM *, VSTRING *);
extern void mail_stream_ctl(MAIL_STREAM *, int,...);


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
