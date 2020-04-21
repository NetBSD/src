/*	$NetBSD: version.c,v 1.5.86.2 2020/04/21 19:37:32 martin Exp $	*/

/*
 * value of $KSH_VERSION (or $SH_VERSION)
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: version.c,v 1.5.86.2 2020/04/21 19:37:32 martin Exp $");
#endif


#include "sh.h"

char ksh_version [] =
	"@(#)PD KSH v5.2.14 99/07/13.2";
