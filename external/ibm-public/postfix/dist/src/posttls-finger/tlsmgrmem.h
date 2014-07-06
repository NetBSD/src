/*	$NetBSD: tlsmgrmem.h,v 1.1.1.1 2014/07/06 19:27:55 tron Exp $	*/

/*++
/* NAME
/*	tlsmgrmem 3
/* SUMMARY
/*	Memory-based TLS manager interface for tlsfinger(1).
/* SYNOPSIS
/*	#include <tlsmgrmem.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern void tlsmgrmem_disable(void);
extern void tlsmgrmem_status(int *, int *, int *);
extern void tlsmgrmem_flush(void);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/
