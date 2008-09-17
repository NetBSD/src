#ifndef _SMTP_STREAM_H_INCLUDED_
#define _SMTP_STREAM_H_INCLUDED_

/*++
/* NAME
/*	smtp_stream 3h
/* SUMMARY
/*	smtp stream I/O support
/* SYNOPSIS
/*	#include <smtp_stream.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>
#include <setjmp.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * External interface. The following codes are meant for use in longjmp(),
  * so they must all be non-zero.
  */
#define SMTP_ERR_EOF	1		/* unexpected client disconnect */
#define SMTP_ERR_TIME	2		/* time out */
#define SMTP_ERR_QUIET	3		/* silent cleanup (application) */
#define SMTP_ERR_NONE	4		/* non-error case */

extern void smtp_timeout_setup(VSTREAM *, int);
extern void PRINTFLIKE(2, 3) smtp_printf(VSTREAM *, const char *,...);
extern void smtp_flush(VSTREAM *);
extern int smtp_fgetc(VSTREAM *);
extern int smtp_get(VSTRING *, VSTREAM *, ssize_t);
extern void smtp_fputs(const char *, ssize_t len, VSTREAM *);
extern void smtp_fwrite(const char *, ssize_t len, VSTREAM *);
extern void smtp_fputc(int, VSTREAM *);

extern void smtp_vprintf(VSTREAM *, const char *, va_list);

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
