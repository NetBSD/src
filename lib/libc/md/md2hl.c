/*	$NetBSD: md2hl.c,v 1.1.2.3 2002/04/25 04:01:44 nathanw Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@netbsd.org>, April 29, 1997.
 * Public domain.
 */

#define	MDALGORITHM	MD2

#include "namespace.h"
#include <md2.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if !HAVE_MD2_H
#include "mdXhl.c"
#endif
