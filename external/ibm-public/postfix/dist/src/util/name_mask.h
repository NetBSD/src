/*	$NetBSD: name_mask.h,v 1.1.1.2 2011/03/02 19:32:44 tron Exp $	*/

#ifndef _NAME_MASK_H_INCLUDED_
#define _NAME_MASK_H_INCLUDED_

/*++
/* NAME
/*	name_mask 3h
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <name_mask.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
typedef struct {
    const char *name;
    int     mask;
} NAME_MASK;

#define NAME_MASK_FATAL	(1<<0)
#define NAME_MASK_ANY_CASE	(1<<1)
#define NAME_MASK_RETURN	(1<<2)
#define NAME_MASK_COMMA		(1<<3)
#define NAME_MASK_PIPE		(1<<4)
#define NAME_MASK_NUMBER	(1<<5)
#define NAME_MASK_WARN		(1<<6)
#define NAME_MASK_IGNORE	(1<<7)

#define NAME_MASK_REQUIRED \
    (NAME_MASK_FATAL | NAME_MASK_RETURN | NAME_MASK_WARN | NAME_MASK_IGNORE)
#define STR_NAME_MASK_REQUIRED	(NAME_MASK_REQUIRED | NAME_MASK_NUMBER)

#define NAME_MASK_MATCH_REQ	NAME_MASK_FATAL

#define NAME_MASK_NONE		0
#define NAME_MASK_DEFAULT	(NAME_MASK_FATAL)
#define NAME_MASK_DEFAULT_DELIM	", \t\r\n"

#define name_mask_opt(tag, table, str, flags) \
	name_mask_delim_opt((tag), (table), (str), \
			    NAME_MASK_DEFAULT_DELIM, (flags))
#define name_mask(tag, table, str) \
	name_mask_opt((tag), (table), (str), NAME_MASK_DEFAULT)
#define str_name_mask(tag, table, mask) \
	str_name_mask_opt(((VSTRING *) 0), (tag), (table), (mask), NAME_MASK_DEFAULT)

extern int name_mask_delim_opt(const char *, const NAME_MASK *, const char *, const char *, int);
extern const char *str_name_mask_opt(VSTRING *, const char *, const NAME_MASK *, int, int);

 /*
  * "long" API
  */
typedef struct {
    const char *name;
    long    mask;
} LONG_NAME_MASK;

#define long_name_mask_opt(tag, table, str, flags) \
	long_name_mask_delim_opt((tag), (table), (str), NAME_MASK_DEFAULT_DELIM, (flags))
#define long_name_mask(tag, table, str) \
	long_name_mask_opt((tag), (table), (str), NAME_MASK_DEFAULT)
#define str_long_name_mask(tag, table, mask) \
	str_long_name_mask_opt(((VSTRING *) 0), (tag), (table), (mask), NAME_MASK_DEFAULT)

extern long long_name_mask_delim_opt(const char *, const LONG_NAME_MASK *, const char *, const char *, int);
extern const char *str_long_name_mask_opt(VSTRING *, const char *, const LONG_NAME_MASK *, long, int);

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
