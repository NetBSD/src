/*	$NetBSD: name_mask.h,v 1.1.1.2.2.1 2006/07/12 15:06:44 tron Exp $	*/

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
  * External interface.
  */
typedef struct {
    const char *name;
    int     mask;
} NAME_MASK;

#define NAME_MASK_MATCH_REQ	(1<<0)
#define NAME_MASK_ANY_CASE	(1<<1)

#define NAME_MASK_NONE		0
#define NAME_MASK_DEFAULT	(NAME_MASK_MATCH_REQ)

#define name_mask(tag, table, str) \
	name_mask_opt((tag), (table), (str), NAME_MASK_DEFAULT)
#define str_name_mask(tag, table, mask) \
	str_name_mask_opt((tag), (table), (mask), NAME_MASK_DEFAULT)

extern int name_mask_opt(const char *, NAME_MASK *, const char *, int);
extern const char *str_name_mask_opt(const char *, NAME_MASK *, int, int);

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
