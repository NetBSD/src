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
extern int rec_get(VSTREAM *, VSTRING *, int);
extern int rec_put(VSTREAM *, int, const char *, int);
extern int rec_put_type(VSTREAM *, int, long);
extern int PRINTFLIKE(3, 4) rec_fprintf(VSTREAM *, int, const char *,...);
extern int rec_fputs(VSTREAM *, int, const char *);

#define REC_PUT_BUF(v, t, b) rec_put((v), (t), vstring_str(b), VSTRING_LEN(b))

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
