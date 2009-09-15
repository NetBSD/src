/*	$NetBSD: dict_regexp.h,v 1.1.1.1.2.2 2009/09/15 06:03:56 snj Exp $	*/

#ifndef _DICT_REGEXP_H_INCLUDED_
#define _DICT_REGEXP_H_INCLUDED_

/*++
/* NAME
/*	dict_regexp 3h
/* SUMMARY
/*	dictionary manager interface to REGEXP regular expression library
/* SYNOPSIS
/*	#include <dict_regexp.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_REGEXP	"regexp"

extern DICT *dict_regexp_open(const char *, int, int);

/* AUTHOR(S)
/*	LaMont Jones
/*	lamont@hp.com
/*
/*	Based on PCRE dictionary contributed by	Andrew McNamara
/*	andrewm@connect.com.au
/*	connect.com.au Pty. Ltd.
/*	Level 3, 213 Miller St
/*	North Sydney, NSW, Australia
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
