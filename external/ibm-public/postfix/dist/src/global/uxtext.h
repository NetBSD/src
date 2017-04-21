/*	$NetBSD: uxtext.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _UXTEXT_H_INCLUDED_
#define _UXTEXT_H_INCLUDED_

/*++
/* NAME
/*	uxtext 3h
/* SUMMARY
/*	quote/unquote text, RFC 6533 style.
/* SYNOPSIS
/*	#include <uxtext.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *uxtext_quote(VSTRING *, const char *, const char *);
extern VSTRING *uxtext_quote_append(VSTRING *, const char *, const char *);
extern VSTRING *uxtext_unquote(VSTRING *, const char *);
extern VSTRING *uxtext_unquote_append(VSTRING *, const char *);

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
/*--*/

#endif
