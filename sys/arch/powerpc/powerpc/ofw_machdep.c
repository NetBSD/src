/*	$NetBSD: ofw_machdep.c,v 1.19 2012/02/01 09:54:03 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofw_machdep.c,v 1.19 2012/02/01 09:54:03 matt Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/systm.h>

#include <dev/ofw/openfirm.h>

#include <machine/powerpc.h>
#include <machine/autoconf.h>

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
	int phandle, i, cnt, regcnt;
	struct mem_region_avail {
		paddr_t start;
		paddr_t size;
	} OFavail_G5[OFMEM_REGIONS + 3] __attribute((unused));

	/*
	 * Get memory.
	 */
	if ((phandle = OF_finddevice("/memory")) == -1)
		goto error;

	memset(OFmem, 0, sizeof OFmem);
	regcnt = OF_getprop(phandle, "reg",
		OFmem, sizeof OFmem[0] * OFMEM_REGIONS);
	if (regcnt <= 0)
		goto error;

	/* Remove zero sized entry in the returned data. */
	regcnt /= sizeof OFmem[0];
	for (i = 0; i < regcnt; )
		if (OFmem[i].size == 0) {
			memmove(&OFmem[i], &OFmem[i + 1],
				(regcnt - i) * sizeof OFmem[0]);
			regcnt--;
		} else
			i++;

#if defined (PMAC_G5)
	/* XXXSL: the G5 implementation of OFW is defines the /memory reg/available
	 * properties differently. Try to fix it up here with minimal damage to the
	 * rest of the code
 	 */
	{
		int count;
		memset(OFavail_G5, 0, sizeof OFavail_G5);
		count = OF_getprop(phandle, "available",
			OFavail_G5, sizeof OFavail_G5[0] * OFMEM_REGIONS);

		if (count <= 0)
			goto error;

		count /= sizeof OFavail_G5[0];
		cnt = count * sizeof(OFavail[0]);

		for (i = 0; i < count; i++ )
		{
			OFavail[i].start_hi = 0;
			OFavail[i].start = OFavail_G5[i].start;
			OFavail[i].size = OFavail_G5[i].size;
		}
	}
#else
	memset(OFavail, 0, sizeof OFavail);
	cnt = OF_getprop(phandle, "available",
		OFavail, sizeof OFavail[0] * OFMEM_REGIONS);
#endif
	if (cnt <= 0)
		goto error;

	cnt /= sizeof OFavail[0];
	for (i = 0; i < cnt; ) {
		if (OFavail[i].size == 0) {
			memmove(&OFavail[i], &OFavail[i + 1],
				(cnt - i) * sizeof OFavail[0]);
			cnt--;
		} else
			i++;
	}

	if (strncmp(model_name, "Pegasos", 7) == 0) {
		/*
		 * Some versions of SmartFirmware, only recognize the first
		 * 256MB segment as available. Work around it and add an
		 * extra entry to OFavail[] to account for this.
		 */
#define AVAIL_THRESH (0x10000000-1)
		if (((OFavail[cnt-1].start + OFavail[cnt-1].size +
		    AVAIL_THRESH) & ~AVAIL_THRESH) <
		    (OFmem[regcnt-1].start + OFmem[regcnt-1].size)) {

			OFavail[cnt].start =
			    (OFavail[cnt-1].start + OFavail[cnt-1].size +
			    AVAIL_THRESH) & ~AVAIL_THRESH;
			OFavail[cnt].size =
			    OFmem[regcnt-1].size - OFavail[cnt].start;
			aprint_normal("WARNING: add memory segment %lx - %lx,"
			    "\nWARNING: which was not recognized by "
			    "the Firmware.\n",
			    (unsigned long)OFavail[cnt].start,
			    (unsigned long)OFavail[cnt].start +
			    OFavail[cnt].size);
			cnt++;
		}
	}

	*memp = OFmem;
	*availp = OFavail;
	return;

error:
#if defined (MAMBO)
	printf("no memory, assuming 512MB\n");

	OFmem[0].start = 0x0;
	OFmem[0].size = 0x20000000;
	
	OFavail[0].start = 0x3000;
	OFavail[0].size = 0x20000000 - 0x3000;

	*memp = OFmem;
	*availp = OFavail;
#else
	panic("no memory?");
#endif
	return;
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
