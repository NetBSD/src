/*	$NetBSD: dir_forest.h,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

#ifndef _DIR_FOREST_H_INCLUDED_
#define _DIR_FOREST_H_INCLUDED_

/*++
/* NAME
/*	dir_forest 3h
/* SUMMARY
/*	file name to directory forest
/* SYNOPSIS
/*	#include <dir_forest.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern char *dir_forest(VSTRING *, const char *, int);

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
