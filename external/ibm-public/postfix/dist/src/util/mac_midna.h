/*	$NetBSD: mac_midna.h,v 1.2.4.2 2026/11/04 23:14:58 perseant Exp $	*/

#ifndef _MAC_MIDHA_H_INCLUDED_
#define _MAC_MIDHA_H_INCLUDED_

/*++
/* NAME
/*	mac_midna 3h
/* SUMMARY
/*	IDNA-based mac_expand() plugin
/* SYNOPSIS
/*	#include <mac_midna.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern void mac_midna_register(void);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
