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
    char   *id;				/* queue id */
    MAIL_STREAM_FINISH_FN finish;	/* finish code */
    MAIL_STREAM_CLOSE_FN close;		/* close stream */
    char   *class;			/* trigger class */
    char   *service;			/* trigger service */
    int     mode;			/* additional permissions */
};

extern MAIL_STREAM *mail_stream_file(const char *, const char *, const char *, int);
extern MAIL_STREAM *mail_stream_service(const char *, const char *);
extern MAIL_STREAM *mail_stream_command(const char *);
extern void mail_stream_cleanup(MAIL_STREAM *);
extern int mail_stream_finish(MAIL_STREAM *, VSTRING *);


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
