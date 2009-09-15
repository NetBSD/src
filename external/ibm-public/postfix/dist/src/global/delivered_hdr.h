/*	$NetBSD: delivered_hdr.h,v 1.1.1.1.2.2 2009/09/15 06:02:40 snj Exp $	*/

#ifndef _DELIVERED_HDR_H_INCLUDED_
#define _DELIVERED_HDR_H_INCLUDED_

/*++
/* NAME
/*	delivered_hdr 3h
/* SUMMARY
/*	process Delivered-To: headers
/* SYNOPSIS
/*	#include <delivered_hdr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * Global library.
  */
#include <fold_addr.h>

 /*
  * External interface.
  */
typedef struct DELIVERED_HDR_INFO DELIVERED_HDR_INFO;
extern DELIVERED_HDR_INFO *delivered_hdr_init(VSTREAM *, off_t, int);
extern int delivered_hdr_find(DELIVERED_HDR_INFO *, const char *);
extern void delivered_hdr_free(DELIVERED_HDR_INFO *);

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
