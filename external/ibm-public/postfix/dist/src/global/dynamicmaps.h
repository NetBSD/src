/*	$NetBSD: dynamicmaps.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

#ifndef _DYNAMICMAPS_H_INCLUDED_
#define _DYNAMICMAPS_H_INCLUDED_

/*++
/* NAME
/*	dynamicmaps 3h
/* SUMMARY
/*	load dictionaries dynamically
/* SYNOPSIS
/*	#include <dynamicmaps.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#ifdef USE_DYNAMIC_LIBS

extern void dymap_init(const char *, const char *);

#endif
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
