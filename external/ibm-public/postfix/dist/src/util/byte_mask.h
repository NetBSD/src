/*	$NetBSD: byte_mask.h,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

#ifndef _BYTE_MASK_H_INCLUDED_
#define _BYTE_MASK_H_INCLUDED_

/*++
/* NAME
/*	byte_mask 3h
/* SUMMARY
/*	map names to bit mask
/* SYNOPSIS
/*	#include <byte_mask.h>
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
    int     byte_val;
    int     mask;
} BYTE_MASK;

#define BYTE_MASK_FATAL		(1<<0)
#define BYTE_MASK_ANY_CASE	(1<<1)
#define BYTE_MASK_RETURN	(1<<2)
#define BYTE_MASK_WARN		(1<<6)
#define BYTE_MASK_IGNORE	(1<<7)

#define BYTE_MASK_REQUIRED \
    (BYTE_MASK_FATAL | BYTE_MASK_RETURN | BYTE_MASK_WARN | BYTE_MASK_IGNORE)
#define STR_BYTE_MASK_REQUIRED	(BYTE_MASK_REQUIRED)

#define BYTE_MASK_NONE		0
#define BYTE_MASK_DEFAULT	(BYTE_MASK_FATAL)

#define byte_mask(tag, table, str) \
	byte_mask_opt((tag), (table), (str), BYTE_MASK_DEFAULT)
#define str_byte_mask(tag, table, mask) \
	str_byte_mask_opt(((VSTRING *) 0), (tag), (table), (mask), BYTE_MASK_DEFAULT)

extern int byte_mask_opt(const char *, const BYTE_MASK *, const char *, int);
extern const char *str_byte_mask_opt(VSTRING *, const char *, const BYTE_MASK *, int, int);

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
