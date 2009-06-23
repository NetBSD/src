/*	$NetBSD: dict_ni.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_NI_H_INCLUDED_
#define _DICT_NI_H_INCLUDED_

/*++
/* NAME
/*	dict_ni 3h
/* SUMMARY
/*	dictionary manager interface to NetInfo maps
/* SYNOPSIS
/*	#include <dict_ni.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_NETINFO	"netinfo"

extern DICT *dict_ni_open(const char *, int, int);

/* AUTHOR(S)
/*	Pieter Schoenmakers
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB Eindhoven
/*	The Netherlands
/*--*/

#endif
