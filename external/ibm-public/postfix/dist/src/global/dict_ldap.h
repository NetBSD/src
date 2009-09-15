/*	$NetBSD: dict_ldap.h,v 1.1.1.1.2.2 2009/09/15 06:02:40 snj Exp $	*/

#ifndef _DICT_LDAP_H_INCLUDED_
#define _DICT_LDAP_H_INCLUDED_

/*++
/* NAME
/*	dict_ldap 3h
/* SUMMARY
/*	dictionary manager interface to LDAP maps
/* SYNOPSIS
/*	#include <dict_ldap.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_LDAP	"ldap"

extern DICT *dict_ldap_open(const char *, int, int);

/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10532, USA
/*--*/

#endif
