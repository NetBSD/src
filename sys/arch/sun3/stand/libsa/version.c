/*	$NetBSD: version.c,v 1.5 1998/02/05 04:57:17 gwr Exp $ */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 *
 * 1/98 Common boot programs run on both Sun 3 and 3X
 *      machines, with different default kernel names
 *      on each so shared root images are possible.
 */

#include <sys/types.h>
#include "libsa.h"

char version[] = "$Revision: 1.5 $";
