/*	$NetBSD: md4hl.c,v 1.3 2002/03/31 12:58:56 bjh21 Exp $	*/

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
