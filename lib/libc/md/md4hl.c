/*	$NetBSD: md4hl.c,v 1.5 2003/10/27 00:12:42 lukem Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, April 29, 1997.
 * Public domain.
 */

#define	MDALGORITHM	MD4

#include "namespace.h"
#include <md4.h>

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#if !HAVE_MD4_H
#include "mdXhl.c"
#endif
