/*	$NetBSD: md4hl.c,v 1.4 2003/07/26 19:24:47 salo Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, April 29, 1997.
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
