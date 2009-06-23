/*	$NetBSD: rec_streamlf.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

#ifndef _REC_STREAMLF_H_INCLUDED_
#define _REC_STREAMLF_H_INCLUDED_

/*++
/* NAME
/*	rec_streamlf 3h
/* SUMMARY
/*	record interface to stream-lf files
/* SYNOPSIS
/*	#include <rec_streamlf.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <rec_type.h>

 /*
  * External interface.
  */
extern int rec_streamlf_get(VSTREAM *, VSTRING *, int);
extern int rec_streamlf_put(VSTREAM *, int, const char *, int);

#define REC_STREAMLF_PUT_BUF(s, t, b) \
	rec_streamlf_put((s), (t), vstring_str(b), VSTRING_LEN(b))

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
