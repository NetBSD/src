/*	$NetBSD: md5hl.c,v 1.2.12.1 2002/04/25 04:01:44 nathanw Exp $	*/

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
