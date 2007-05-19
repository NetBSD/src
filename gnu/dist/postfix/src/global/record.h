/*	$NetBSD: record.h,v 1.1.1.4 2007/05/19 16:28:17 heas Exp $	*/

#ifndef _RECORD_H_INCLUDED_
#define _RECORD_H_INCLUDED_

/*++
/* NAME
/*	record 3h
/* SUMMARY
/*	simple typed record I/O
/* SYNOPSIS
/*	#include <record.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Record type values are positive numbers 0..255. Negative record type
  * values are reserved for diagnostics.
  */
#define REC_TYPE_EOF	-1		/* no record */
#define REC_TYPE_ERROR	-2		/* bad record */

 /*
  * Functional interface.
  */
extern int rec_get_raw(VSTREAM *, VSTRING *, ssize_t, int);
extern int rec_put(VSTREAM *, int, const char *, ssize_t);
extern int rec_put_type(VSTREAM *, int, off_t);
extern int PRINTFLIKE(3, 4) rec_fprintf(VSTREAM *, int, const char *,...);
extern int rec_fputs(VSTREAM *, int, const char *);
extern int rec_goto(VSTREAM *, const char *);
extern int rec_pad(VSTREAM *, int, int);

#define REC_PUT_BUF(v, t, b) rec_put((v), (t), vstring_str(b), VSTRING_LEN(b))

#define REC_FLAG_NONE		(0)
#define REC_FLAG_FOLLOW_PTR	(1<<0)		/* follow PTR records */
#define REC_FLAG_SKIP_DTXT	(1<<1)		/* skip DTXT records */
#define REC_FLAG_SEEK_END	(1<<2)		/* seek EOF after END record */

#define REC_FLAG_DEFAULT \
	(REC_FLAG_FOLLOW_PTR | REC_FLAG_SKIP_DTXT | REC_FLAG_SEEK_END)

#define rec_get(fp, buf, limit) \
	rec_get_raw((fp), (buf), (limit), REC_FLAG_DEFAULT)

#define REC_SPACE_NEED(buflen, reclen) do { \
	    ssize_t _c, _l; \
	    for (_c = 1, _l = (buflen); (_l >>= 7U) != 0; _c++) \
		; \
	    (reclen) = 1 + _c + (buflen); \
	} while (0)

 /*
  * Stuff that needs <stdarg.h>
  */
extern int rec_vfprintf(VSTREAM *, int, const char *, va_list);

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
