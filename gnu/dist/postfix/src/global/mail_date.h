#ifndef _MAIL_DATE_H_INCLUDED_
#define _MAIL_DATE_H_INCLUDED_

/*++
/* NAME
/*	mail_date 3h
/* SUMMARY
/*	return formatted time
/* SYNOPSIS
/*	#include <mail_date.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>

 /*
  * External interface
  */
extern const char *mail_date(time_t);

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
