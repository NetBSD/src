/*	$NetBSD: fold_addr.h,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

#ifndef _FOLD_ADDR_H_INCLUDED_
#define _FOLD_ADDR_H_INCLUDED_

/*++
/* NAME
/*	fold_addr 3h
/* SUMMARY
/*	address case folding
/* SYNOPSIS
/*	#include <fold_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define FOLD_ADDR_USER	(1<<0)
#define FOLD_ADDR_HOST	(1<<1)

#define FOLD_ADDR_ALL	(FOLD_ADDR_USER | FOLD_ADDR_HOST)

extern char *fold_addr(char *, int);

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
