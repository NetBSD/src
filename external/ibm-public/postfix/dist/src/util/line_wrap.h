/*	$NetBSD: line_wrap.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _LINE_WRAP_H_INCLUDED_
#define _LINE_WRAP_H_INCLUDED_

/*++
/* NAME
/*	line_wrap 3h
/* SUMMARY
/*	wrap long lines upon output
/* SYNOPSIS
/*	#include <line_wrap.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef void (*LINE_WRAP_FN) (const char *, int, int, void *);
extern void line_wrap(const char *, int, int, LINE_WRAP_FN, void *);

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
