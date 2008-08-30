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
extern int vstring_get(VSTRING *, VSTREAM *);
extern int vstring_get_nonl(VSTRING *, VSTREAM *);
extern int vstring_get_null(VSTRING *, VSTREAM *);
extern int vstring_get_bound(VSTRING *, VSTREAM *, ssize_t);
extern int vstring_get_nonl_bound(VSTRING *, VSTREAM *, ssize_t);
extern int vstring_get_null_bound(VSTRING *, VSTREAM *, ssize_t);

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
/*--*/

#endif
