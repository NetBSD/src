/*	$NetBSD: mail_addr_map.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _MAIL_ADDR_MAP_H_INCLUDED_
#define _MAIL_ADDR_MAP_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_map 3h
/* SUMMARY
/*	generic address mapping
/* SYNOPSIS
/*	#include <mail_addr_map.h>
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
#include <maps.h>

 /*
  * External interface.
  */
extern ARGV *mail_addr_map_opt(MAPS *, const char *, int, int, int, int);

 /* The least-overhead form. */
#define mail_addr_map_internal(path, address, propagate) \
	mail_addr_map_opt((path), (address), (propagate), \
		  MA_FORM_INTERNAL, MA_FORM_EXTERNAL, MA_FORM_INTERNAL)

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
