/*	$NetBSD: mail_addr_crunch.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _MAIL_ADDR_CRUNCH_H_INCLUDED_
#define _MAIL_ADDR_CRUNCH_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_crunch 3h
/* SUMMARY
/*	parse and canonicalize addresses, apply address extension
/* SYNOPSIS
/*	#include <mail_addr_crunch.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * Global library.
  */
#include <mail_addr_form.h>

 /*
  * External interface.
  */
extern ARGV *mail_addr_crunch_opt(const char *, const char *, int, int);

 /*
  * The least-overhead form.
  */
#define mail_addr_crunch_ext_to_int(string, extension) \
	mail_addr_crunch_opt((string), (extension), MA_FORM_EXTERNAL, \
			MA_FORM_INTERNAL)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
