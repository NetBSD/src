/*	$NetBSD: ascii_header_text.h,v 1.2 2025/02/25 19:15:45 christos Exp $	*/

#ifndef _ASCII_HEADER_TEXT_H_INCLUDED_
#define _ASCII_HEADER_TEXT_H_INCLUDED_

/*++
/* NAME
/*	ascii_header_text 3h
/* SUMMARY
/*	message header content formatting
/* SYNOPSIS
/*	#include <ascii_header_text.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library
  */
#include <vstring.h>

 /*
  * External API.
  */
#define HDR_TEXT_FLAG_COMMENT	(1<<0)	/* Generate comment content */
#define HDR_TEXT_FLAG_PHRASE	(1<<1)	/* Generate phrase content */
#define HDR_TEXT_FLAG_FOLD	(1<<2)	/* Generate header folding hints */

#define HDR_TEXT_MASK_TARGET \
	(HDR_TEXT_FLAG_COMMENT | HDR_TEXT_FLAG_PHRASE)

extern char *make_ascii_header_text(VSTRING *result, int flags, const char *in);

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
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif

