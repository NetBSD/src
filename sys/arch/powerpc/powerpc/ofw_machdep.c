/*	$NetBSD: ofw_machdep.c,v 1.14 2003/07/15 02:54:48 lukem Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_machdep.c,v 1.14 2003/07/15 02:54:48 lukem Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <machine/powerpc.h>

#define	OFMEM_REGIONS	32
static struct mem_region OFmem[OFMEM_REGIONS + 1], OFavail[OFMEM_REGIONS + 3];

/*
 * This is called during initppc, before the system is really initialized.
 * It shall provide the total and the available regions of RAM.
 * Both lists must have a zero-size entry as terminator.
 * The available regions need not take the kernel into account, but needs
 * to provide space for two additional entry beyond the terminating one.
 */
void
mem_regions(struct mem_region **memp, struct mem_region **availp)
{
	int phandle, i, cnt;

	/*
	 * Get memory.
	 */
	if ((phandle = OF_finddevice("/memory")) == -1)
		goto error;

	memset(OFmem, 0, sizeof OFmem);
	cnt = OF_getprop(phandle, "reg",
		OFmem, sizeof OFmem[0] * OFMEM_REGIONS);
	if (cnt <= 0)
		goto error;

	/* Remove zero sized entry in the returned data. */
	cnt /= sizeof OFmem[0];
	for (i = 0; i < cnt; )
		if (OFmem[i].size == 0) {
			memmove(&OFmem[i], &OFmem[i + 1],
				(cnt - i) * sizeof OFmem[0]);
			cnt--;
		} else
			i++;

	memset(OFavail, 0, sizeof OFavail);
	cnt = OF_getprop(phandle, "available",
		OFavail, sizeof OFavail[0] * OFMEM_REGIONS);
	if (cnt <= 0)
		goto error;

	cnt /= sizeof OFavail[0];
	for (i = 0; i < cnt; )
		if (OFavail[i].size == 0) {
			memmove(&OFavail[i], &OFavail[i + 1],
				(cnt - i) * sizeof OFavail[0]);
			cnt--;
		} else
			i++;

	*memp = OFmem;
	*availp = OFavail;
	return;

error:
	panic("no memory?");
}

void
ppc_exit(void)
{
	OF_exit();
}

void
ppc_boot(char *str)
{
	OF_boot(str);
}
