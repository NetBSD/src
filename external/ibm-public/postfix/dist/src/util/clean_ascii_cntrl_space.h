/*	$NetBSD: clean_ascii_cntrl_space.h,v 1.2.4.2 2025/08/02 05:50:18 perseant Exp $	*/

#ifndef _UNCNTRL_H_INCLUDED_
#define _UNCNTRL_H_INCLUDED_

/*++
/* NAME
/*	clean_ascii_cntrl_space 3h
/* SUMMARY
/*	sane control character removal
/* SYNOPSIS
/*	#include <clean_ascii_cntrl_space.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
extern char *clean_ascii_cntrl_space(VSTRING *result, const char *input, ssize_t len);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
