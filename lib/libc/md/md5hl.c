/*	$NetBSD: md5hl.c,v 1.3 2002/03/31 12:58:56 bjh21 Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@netbsd.org>, April 29, 1997.
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
