#ifndef _TRACE_H_INCLUDED_
#define _TRACE_H_INCLUDED_

/*++
/* NAME
/*	trace 3h
/* SUMMARY
/*	update user message delivery record
/* SYNOPSIS
/*	#include <trace.h>
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
#include <deliver_request.h>

 /*
  * External interface.
  */
extern int PRINTFLIKE(9, 10) trace_append(int, const char *,
					          const char *, const char *,
					          const char *, time_t,
					          const char *, const char *,
					          const char *,...);
extern int vtrace_append(int, const char *,
			         const char *, const char *,
			         const char *, time_t,
			         const char *, const char *,
			         const char *, va_list);
extern int trace_flush(int, const char *, const char *, const char *, const char *);

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
