#ifndef _SENT_H_INCLUDED_
#define _SENT_H_INCLUDED_

/*++
/* NAME
/*	sent 3h
/* SUMMARY
/*	log that message was sent
/* SYNOPSIS
/*	#include <sent.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>
#include <stdarg.h>

 /*
  * External interface.
  */
extern int PRINTFLIKE(5, 6) sent(const char *, const char *, const char *,
				         time_t, const char *,...);
extern int vsent(const char *, const char *, const char *,
		         time_t, const char *, va_list);

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
/**INDENT** Error@17: Unmatched #endif */

#endif
