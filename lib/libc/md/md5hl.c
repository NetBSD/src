/*	$NetBSD: md5hl.c,v 1.4 2003/07/26 19:24:47 salo Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, April 29, 1997.
 * Public domain.
 */

#define	MDALGORITHM	MD5

#include "namespace.h"
#include <md5.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_MD5_H
#include "mdXhl.c"
#endif
