/*	$NetBSD: vstring_vstream.h,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

#ifndef _VSTRING_VSTREAM_H_INCLUDED_
#define _VSTRING_VSTREAM_H_INCLUDED_

/*++
/* NAME
/*	vstring_vstream 3h
/* SUMMARY
/*	auto-resizing string library
/* SYNOPSIS
/*	#include <vstring_vstream.h>
/* DESCRIPTION

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
#define VSTRING_GET_FLAG_NONE	(0)
#define VSTRING_GET_FLAG_APPEND	(1<<1)	/* append instead of overwrite */

extern int WARN_UNUSED_RESULT vstring_get_flags(VSTRING *, VSTREAM *, int);
extern int WARN_UNUSED_RESULT vstring_get_flags_nonl(VSTRING *, VSTREAM *, int);
extern int WARN_UNUSED_RESULT vstring_get_flags_null(VSTRING *, VSTREAM *, int);
extern int WARN_UNUSED_RESULT vstring_get_flags_bound(VSTRING *, VSTREAM *, int, ssize_t);
extern int WARN_UNUSED_RESULT vstring_get_flags_nonl_bound(VSTRING *, VSTREAM *, int, ssize_t);
extern int WARN_UNUSED_RESULT vstring_get_flags_null_bound(VSTRING *, VSTREAM *, int, ssize_t);

 /*
  * Convenience aliases for most use cases.
  */
#define vstring_get(string, stream) \
	vstring_get_flags((string), (stream), VSTRING_GET_FLAG_NONE)
#define vstring_get_nonl(string, stream) \
	vstring_get_flags_nonl((string), (stream), VSTRING_GET_FLAG_NONE)
#define vstring_get_null(string, stream) \
	vstring_get_flags_null((string), (stream), VSTRING_GET_FLAG_NONE)

#define vstring_get_bound(string, stream, size) \
	vstring_get_flags_bound((string), (stream), VSTRING_GET_FLAG_NONE, size)
#define vstring_get_nonl_bound(string, stream, size) \
	vstring_get_flags_nonl_bound((string), (stream), \
	    VSTRING_GET_FLAG_NONE, size)
#define vstring_get_null_bound(string, stream, size) \
	vstring_get_flags_null_bound((string), (stream), \
	    VSTRING_GET_FLAG_NONE, size)

 /*
  * Backwards compatibility for code that still uses the vstring_fgets()
  * interface. Unfortunately we can't change the macro name to upper case.
  */
#define vstring_fgets(s, p) \
	(vstring_get((s), (p)) == VSTREAM_EOF ? 0 : (s))
#define vstring_fgets_nonl(s, p) \
	(vstring_get_nonl((s), (p)) == VSTREAM_EOF ? 0 : (s))
#define vstring_fgets_null(s, p) \
	(vstring_get_null((s), (p)) == VSTREAM_EOF ? 0 : (s))
#define vstring_fgets_bound(s, p, l) \
	(vstring_get_bound((s), (p), (l)) == VSTREAM_EOF ? 0 : (s))
#define vstring_fgets_nonl_bound(s, p, l) \
	(vstring_get_nonl_bound((s), (p), (l)) == VSTREAM_EOF ? 0 : (s))

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
