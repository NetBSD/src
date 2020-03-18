/*	$NetBSD: mail_addr_find.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _MAIL_ADDR_FIND_H_INCLUDED_
#define _MAIL_ADDR_FIND_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_find 3h
/* SUMMARY
/*	generic address-based lookup
/* SYNOPSIS
/*	#include <mail_addr_find.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <mail_addr_form.h>
#include <maps.h>

 /*
  * External interface.
  */
extern const char *mail_addr_find_opt(MAPS *, const char *, char **,
				              int, int, int, int);

#define MA_FIND_FULL	(1<<0)		/* localpart+ext@domain */
#define MA_FIND_NOEXT	(1<<1)		/* localpart@domain */
#define MA_FIND_LOCALPART_IF_LOCAL \
				(1<<2)	/* localpart (maybe localpart+ext) */
#define MA_FIND_LOCALPART_AT_IF_LOCAL \
				(1<<3)	/* ditto, with @ at end */
#define MA_FIND_AT_DOMAIN	(1<<4)	/* @domain */
#define MA_FIND_DOMAIN	(1<<5)		/* domain */
#define MA_FIND_PDMS	(1<<6)		/* parent matches subdomain */
#define MA_FIND_PDDMDS	(1<<7)		/* parent matches dot-subdomain */
#define MA_FIND_LOCALPART_AT	\
				(1<<8)	/* localpart@ (maybe localpart+ext@) */

#define MA_FIND_DEFAULT	(MA_FIND_FULL | MA_FIND_NOEXT \
				| MA_FIND_LOCALPART_IF_LOCAL \
				| MA_FIND_AT_DOMAIN)

 /* The least-overhead form. */
#define mail_addr_find_int_to_ext(maps, address, extension) \
	mail_addr_find_opt((maps), (address), (extension), \
	    MA_FORM_INTERNAL, MA_FORM_EXTERNAL, \
	    MA_FORM_EXTERNAL, MA_FIND_DEFAULT)

 /* The legacy forms. */
#define MA_FIND_FORM_LEGACY \
	MA_FORM_INTERNAL, MA_FORM_EXTERNAL_FIRST, \
	    MA_FORM_EXTERNAL

#define mail_addr_find_strategy(maps, address, extension, strategy) \
	mail_addr_find_opt((maps), (address), (extension), \
	    MA_FIND_FORM_LEGACY, (strategy))

#define mail_addr_find(maps, address, extension) \
	mail_addr_find_strategy((maps), (address), (extension), \
	    MA_FIND_DEFAULT)

#define mail_addr_find_to_internal(maps, address, extension) \
	mail_addr_find_opt((maps), (address), (extension), \
	    MA_FIND_FORM_LEGACY, MA_FIND_DEFAULT)

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
