/*	$NetBSD: version.c,v 1.7 1998/07/01 23:12:42 gwr Exp $ */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 *
 * 1/98 Common boot programs run on both Sun 3 and 3X
 *      machines, with different default kernel names
 *      on each so shared root images are possible.
 *
 * 6/98 Both netboot and ufsboot now look for any of:
 *	{ "netbsd", "netbsd.old", "netbsd.$arch" }
 *	where arch=sun3 or arch=sun3x based on the
 *	running machine type.  Boot media can support
 *	both sun3 and sun3x by providing two kernels.
 */

#include <sys/types.h>
#include "libsa.h"

char version[] = "$Revision: 1.7 $";
