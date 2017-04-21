/*	$NetBSD: mail_parm_split.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _MAIL_PARM_SPLIT_H_INCLUDED_
#define _MAIL_PARM_SPLIT_H_INCLUDED_

/*++
/* NAME
/*	mail_parm_split 3h
/* SUMMARY
/*	split parameter list value
/* SYNOPSIS
/*	#include <mail_parm_split.h>
/* DESCRIPTION
/* .nf

#endif

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface. For consistency, the separator and grouping character
  * sets are not passed as parameters.
  */
extern ARGV *mail_parm_split(const char *, const char *);

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
