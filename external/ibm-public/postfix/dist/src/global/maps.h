/*	$NetBSD: maps.h,v 1.1.1.2 2013/01/02 18:58:59 tron Exp $	*/

#ifndef _MAPS_H_INCLUDED_
#define _MAPS_H_INCLUDED_

/*++
/* NAME
/*	maps 3h
/* SUMMARY
/*	multi-dictionary search
/* SYNOPSIS
/*	#include <maps.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * Dictionary name storage. We're borrowing from the argv(3) module.
  */
typedef struct MAPS {
    char   *title;
    struct ARGV *argv;
    int     error;			/* last request only */
} MAPS;

extern MAPS *maps_create(const char *, const char *, int);
extern const char *maps_find(MAPS *, const char *, int);
extern MAPS *maps_free(MAPS *);

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
