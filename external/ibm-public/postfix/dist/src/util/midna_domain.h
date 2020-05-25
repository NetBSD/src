/*	$NetBSD: midna_domain.h,v 1.4 2020/05/25 23:47:14 christos Exp $	*/

#ifndef _MIDNA_H_INCLUDED_
#define _MIDNA_H_INCLUDED_

/*++
/* NAME
/*	midna_domain 3h
/* SUMMARY
/*	ASCII/UTF-8 domain name conversion
/* SYNOPSIS
/*	#include <midna_domain.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern const char *midna_domain_to_ascii(const char *);
extern const char *midna_domain_to_utf8(const char *);
extern const char *midna_domain_suffix_to_ascii(const char *);
extern const char *midna_domain_suffix_to_utf8(const char *);
extern void midna_domain_pre_chroot(void);

extern int midna_domain_cache_size;
extern int midna_domain_transitional;
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Arnt Gulbrandsen
/*
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
