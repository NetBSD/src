/*	$NetBSD: md5hl.c,v 1.5 2003/10/27 00:12:42 lukem Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, April 29, 1997.
 * Public domain.
 */

#define	MDALGORITHM	MD5

#include "namespace.h"
#include <md5.h>

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#if !HAVE_MD5_H
#include "mdXhl.c"
#endif
