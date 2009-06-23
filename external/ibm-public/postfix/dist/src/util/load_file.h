/*	$NetBSD: load_file.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef LOAD_FILE_H_INCLUDED_
#define LOAD_FILE_H_INCLUDED_

/*++
/* NAME
/*	load_file 3h
/* SUMMARY
/*	load file with some prejudice
/* SYNOPSIS
/*	#include <load_file.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef void (*LOAD_FILE_FN)(VSTREAM *, void *);

extern void load_file(const char *, LOAD_FILE_FN, void *);

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
