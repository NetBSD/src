#ifndef _LOG_ADHOC_H_INCLUDED_
#define _LOG_ADHOC_H_INCLUDED_

/*++
/* NAME
/*	log_adhoc 3h
/* SUMMARY
/*	ad-hoc delivery event logging
/* SYNOPSIS
/*	#include <log_adhoc.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>
#include <time.h>

 /*
  * Client interface.
  */
extern void PRINTFLIKE(7, 8) log_adhoc(const char *, const char *,
				               const char *, const char *,
				               time_t, const char *,
				               const char *,...);
extern void vlog_adhoc(const char *, const char *,
		               const char *, const char *,
		               time_t, const char *,
		               const char *, va_list);

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
