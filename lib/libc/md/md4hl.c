/*	$NetBSD: md4hl.c,v 1.2.12.1 2002/04/25 04:01:44 nathanw Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@netbsd.org>, April 29, 1997.
 * Public domain.
 */

#define	MDALGORITHM	MD4

#include "namespace.h"
#include <md4.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_MD4_H
#include "mdXhl.c"
#endif
