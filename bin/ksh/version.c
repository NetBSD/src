/*	$NetBSD: version.c,v 1.4 2003/06/23 11:39:07 agc Exp $	*/

/*
 * value of $KSH_VERSION (or $SH_VERSION)
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: version.c,v 1.4 2003/06/23 11:39:07 agc Exp $");
#endif


#include "sh.h"

const char ksh_version [] =
	"@(#)PD KSH v5.2.14 99/07/13.2";
