/*	$NetBSD: mail_addr_form.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _MAIL_ADDR_FORM_H_INCLUDED_
#define _MAIL_ADDR_FORM_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_form 3h
/* SUMMARY
/*	mail address formats
/* SYNOPSIS
/*	#include <mail_addr_form.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define MA_FORM_INTERNAL	1	/* unquoted form */
#define MA_FORM_EXTERNAL	2	/* quoted form */
#define MA_FORM_EXTERNAL_FIRST 3	/* quoted form, then unquoted */
#define MA_FORM_INTERNAL_FIRST 4	/* unquoted form, then quoted */

extern int mail_addr_form_from_string(const char *);
extern const char *mail_addr_form_to_string(int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
