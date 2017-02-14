/*	$NetBSD: load_lib.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _LOAD_LIB_H_INCLUDED_
#define _LOAD_LIB_H_INCLUDED_

/*++
/* NAME
/*	load_lib 3h
/* SUMMARY
/*	library loading wrappers
/* SYNOPSIS
/*	#include "load_lib.h"
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
/* NULL name terminates list */
typedef struct LIB_FN {
    const char *name;
    void  (*fptr)(void);
} LIB_FN;

typedef struct LIB_DP {
    const char *name;
    void  *dptr;
} LIB_DP;

extern void load_library_symbols(const char *, LIB_FN *, LIB_DP *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	LaMont Jones
/*	Hewlett-Packard Company
/*	3404 Harmony Road
/*	Fort Collins, CO 80528, USA
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
