/*	$NetBSD: issetugid.c,v 1.1 2002/04/18 15:31:53 bjh21 Exp $	*/

/*
 * Written by Ben Harris, 2002
 * This file is in the Public Domain
 */

#include "config.h"

#if !HAVE_ISSETUGID
int
issetugid(void)
{

	/*
	 * Assume that anything linked against libnbcompat will be installed
	 * without special privileges.
	 */
	return 0;
}
#endif
