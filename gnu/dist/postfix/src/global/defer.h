#ifndef _DEFER_H_INCLUDED_
#define _DEFER_H_INCLUDED_

/*++
/* NAME
/*	defer 3h
/* SUMMARY
/*	defer service client interface
/* SYNOPSIS
/*	#include <defer.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>
#include <stdarg.h>

 /*
  * Global library.
  */
#include <bounce.h>

 /*
  * External interface.
  */
extern int PRINTFLIKE(6, 7) defer_append(int, const char *, const char *,
			            const char *, time_t, const char *,...);
extern int vdefer_append(int, const char *, const char *, const char *,
			         time_t, const char *, va_list);
extern int defer_flush(int, const char *, const char *, const char *);

extern int defer_warn(int, const char *, const char *, const char *);

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
