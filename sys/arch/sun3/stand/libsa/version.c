/*	$NetBSD: version.c,v 1.6 1998/06/29 20:33:03 gwr Exp $ */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 *
 * 1/98 Common boot programs run on both Sun 3 and 3X
 *      machines, with different default kernel names
 *      on each so shared root images are possible.
 *
 * 6/98 Both netboot and ufsboot now look first for
 *	"netbsd.sun3" or "netbsd.sun3x" and then
 *	"netbsd" so that boot media can support
 *	both by providing two kernels.
 */

#include <sys/types.h>
#include "libsa.h"

char version[] = "$Revision: 1.6 $";
