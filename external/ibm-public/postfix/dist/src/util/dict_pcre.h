/*	$NetBSD: dict_pcre.h,v 1.1.1.1.2.2 2009/09/15 06:03:56 snj Exp $	*/

#ifndef _DICT_PCRE_H_INCLUDED_
#define _DICT_PCRE_H_INCLUDED_

/*++
/* NAME
/*	dict_pcre 3h
/* SUMMARY
/*	dictionary manager interface to PCRE regular expression library
/* SYNOPSIS
/*	#include <dict_pcre.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_PCRE	"pcre"

extern DICT *dict_pcre_open(const char *, int, int);

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
/*	Andrew McNamara
/*	andrewm@connect.com.au
/*	connect.com.au Pty. Ltd.
/*	Level 3, 213 Miller St
/*	North Sydney, NSW, Australia
/*--*/

#endif
