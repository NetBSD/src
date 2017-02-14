/*	$NetBSD: mail_stream.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

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
#include <check_arg.h>

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

/* Legacy type-unchecked API, internal use. */
#define MAIL_STREAM_CTL_END	0	/* Terminator */
#define MAIL_STREAM_CTL_QUEUE	1	/* Change queue */
#define MAIL_STREAM_CTL_CLASS	2	/* Change notification class */
#define MAIL_STREAM_CTL_SERVICE	3	/* Change notification service */
#define MAIL_STREAM_CTL_MODE	4	/* Change final queue file mode */
#ifdef DELAY_ACTION
#define MAIL_STREAM_CTL_DELAY	5	/* Change final queue file mtime */
#endif

/* Type-checked API, external use. */
#define CA_MAIL_STREAM_CTL_END		MAIL_STREAM_CTL_END
#define CA_MAIL_STREAM_CTL_QUEUE(v)	MAIL_STREAM_CTL_QUEUE, CHECK_CPTR(MAIL_STREAM, char, (v))
#define CA_MAIL_STREAM_CTL_CLASS(v)	MAIL_STREAM_CTL_CLASS, CHECK_CPTR(MAIL_STREAM, char, (v))
#define CA_MAIL_STREAM_CTL_SERVICE(v)	MAIL_STREAM_CTL_SERVICE, CHECK_CPTR(MAIL_STREAM, char, (v))
#define CA_MAIL_STREAM_CTL_MODE(v)	MAIL_STREAM_CTL_MODE, CHECK_VAL(MAIL_STREAM, int, (v))
#ifdef DELAY_ACTION
#define CA_MAIL_STREAM_CTL_DELAY(v)	MAIL_STREAM_CTL_DELAY, CHECK_VAL(MAIL_STREAM, int, (v))
#endif

CHECK_VAL_HELPER_DCL(MAIL_STREAM, int);
CHECK_CPTR_HELPER_DCL(MAIL_STREAM, char);

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
