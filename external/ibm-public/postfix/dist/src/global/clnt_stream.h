/*	$NetBSD: clnt_stream.h,v 1.1.1.2 2022/10/08 16:09:07 christos Exp $	*/

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
typedef int (*CLNT_STREAM_HANDSHAKE_FN)(VSTREAM *);

extern CLNT_STREAM *clnt_stream_create(const char *, const char *, int, int,
				               CLNT_STREAM_HANDSHAKE_FN);
extern VSTREAM *clnt_stream_access(CLNT_STREAM *);
extern const char *clnt_stream_path(CLNT_STREAM *);
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
