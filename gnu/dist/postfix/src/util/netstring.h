#ifndef _NETSTRING_H_INCLUDED_
#define _NETSTRING_H_INCLUDED_

/*++
/* NAME
/*	netstring 3h
/* SUMMARY
/*	netstring stream I/O support
/* SYNOPSIS
/*	#include <netstring.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * External interface.
  */
#define NETSTRING_ERR_EOF	1	/* unexpected disconnect */
#define NETSTRING_ERR_TIME	2	/* time out */
#define NETSTRING_ERR_FORMAT	3	/* format error */
#define NETSTRING_ERR_SIZE	4	/* netstring too large */

extern void netstring_except(VSTREAM *, int);
extern void netstring_setup(VSTREAM *, int);
extern int netstring_get_length(VSTREAM *);
extern VSTRING *netstring_get_data(VSTREAM *, VSTRING *, int);
extern void netstring_get_terminator(VSTREAM *);
extern VSTRING *netstring_get(VSTREAM *, VSTRING *, int);
extern void netstring_put(VSTREAM *, const char *, int);
extern void netstring_put_multi(VSTREAM *,...);
extern void netstring_fflush(VSTREAM *);
extern VSTRING *netstring_memcpy(VSTRING *, const char *, int);
extern VSTRING *netstring_memcat(VSTRING *, const char *, int);

#define NETSTRING_PUT_BUF(str, buf) \
	netstring_put((str), vstring_str(buf), VSTRING_LEN(buf))

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
