/*	$NetBSD: mac_midna.h,v 1.2.2.2 2026/05/11 17:14:03 martin Exp $	*/

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
