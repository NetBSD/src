/*	$NetBSD: clnt_stream.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _CLNT_STREAM_H_INCLUDED_
#define _CLNT_STREAM_H_INCLUDED_

/*++
/* NAME
/*	clnt_stream 3h
/* SUMMARY
/*	client socket maintenance
/* SYNOPSIS
/*	#include <clnt_stream.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
typedef struct CLNT_STREAM CLNT_STREAM;

extern CLNT_STREAM *clnt_stream_create(const char *, const char *, int, int);
extern VSTREAM *clnt_stream_access(CLNT_STREAM *);
extern void clnt_stream_recover(CLNT_STREAM *);
extern void clnt_stream_free(CLNT_STREAM *);

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
