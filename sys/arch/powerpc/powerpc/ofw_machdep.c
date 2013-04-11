/*	$NetBSD: ofw_machdep.c,v 1.21 2013/04/11 19:53:51 macallan Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ofw_machdep.c,v 1.21 2013/04/11 19:53:51 macallan Exp $");

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

#ifdef DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while(0) printf
#endif

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
	int phandle, i, cnt, regcnt, acells, scells;
	int numregs;
	uint32_t regs[OFMEM_REGIONS * 4]; /* 2 values + 2 for 64bit */

	DPRINTF("calling mem_regions\n");
	/* determine acell size */
	if ((phandle = OF_finddevice("/")) == -1)
		goto error;
	cnt = OF_getprop(phandle, "#address-cells", &acells, sizeof(int));
	if (cnt <= 0)
		acells = 1;

	/* determine scell size */
	if ((phandle = OF_finddevice("/")) == -1)
		goto error;
	cnt = OF_getprop(phandle, "#size-cells", &scells, sizeof(int));
	if (cnt <= 0)
		scells = 1;

	/* Get memory */
	if ((phandle = OF_finddevice("/memory")) == -1)
		goto error;

	memset(regs, 0, sizeof(regs));
	regcnt = OF_getprop(phandle, "reg", regs,
	    sizeof(regs[0]) * OFMEM_REGIONS * 4);
	if (regcnt <= 0)
		goto error;

	/* how many mem regions did we get? */
	numregs = regcnt / (sizeof(uint32_t) * (acells + scells));
	DPRINTF("regcnt=%d num=%d acell=%d scell=%d\n",
	    regcnt, numregs, acells, scells);

	/* move the data into OFmem */
	memset(OFmem, 0, sizeof(OFmem));
	for (i = 0, cnt = 0; i <= numregs; i++) {
		uint64_t addr, size;

		if (acells > 1)
			memcpy(&addr, &regs[i * (acells + scells)],
			    sizeof(int32_t) * acells);
		else
			addr = regs[i * (acells + scells)];

		if (scells > 1)
			memcpy(&size, &regs[i * (acells + scells) + acells],
			    sizeof(int32_t) * scells);
		else
			size = regs[i * (acells + scells) + acells];

		/* skip entry of 0 size */
		if (size == 0)
			continue;
#ifndef _LP64
		if (addr > 0xFFFFFFFF || size > 0xFFFFFFFF ||
			(addr + size) > 0xFFFFFFFF) {
			aprint_error("Base addr of %llx or size of %llx too"
			    " large for 32 bit OS. Skipping.", addr, size);
			continue;
		}
#endif
		OFmem[cnt].start = addr;
		OFmem[cnt].size = size;
		aprint_normal("mem region %d start=%llx size=%llx\n",
		    cnt, addr, size);
		cnt++;
	}

	DPRINTF("available\n");

	/* now do the same thing again, for the available counts */
	memset(regs, 0, sizeof(regs));
	regcnt = OF_getprop(phandle, "available", regs,
	    sizeof(regs[0]) * OFMEM_REGIONS * 4);
	if (regcnt <= 0)
		goto error;

	DPRINTF("%08x %08x %08x %08x\n", regs[0], regs[1], regs[2], regs[3]);

	/*
	 * some(?) G5s have messed up 'available' properties which don't obey
	 * #address-cells. Try to detect this here.
	 * XXX this needs a better test
	 */
	if (((regcnt >> 2) % (acells + scells)) != 0) {
		aprint_normal("messed up 'available' property detected\n");
		acells = 1;
	}
	
	/* how many mem regions did we get? */
	numregs = regcnt / (sizeof(uint32_t) * (acells + scells));
	DPRINTF("regcnt=%d num=%d acell=%d scell=%d\n",
	    regcnt, numregs, acells, scells);

	DPRINTF("to OF_avail\n");

	/* move the data into OFavail */
	memset(OFavail, 0, sizeof(OFavail));
	for (i = 0, cnt = 0; i <= numregs; i++) {
		uint64_t addr, size;

		DPRINTF("%d\n", i);
		if (acells > 1)
			memcpy(&addr, &regs[i * (acells + scells)],
			    sizeof(int32_t) * acells);
		else
			addr = regs[i * (acells + scells)];

		if (scells > 1)
			memcpy(&size, &regs[i * (acells + scells) + acells],
			    sizeof(int32_t) * scells);
		else
			size = regs[i * (acells + scells) + acells];
		/* skip entry of 0 size */
		if (size == 0)
			continue;
#ifndef _LP64
		if (addr > 0xFFFFFFFF || size > 0xFFFFFFFF ||
			(addr + size) > 0xFFFFFFFF) {
			aprint_error("Base addr of %llx or size of %llx too"
			    " large for 32 bit OS. Skipping.", addr, size);
			continue;
		}
#endif
		OFavail[cnt].start = addr;
		OFavail[cnt].size = size;
		aprint_normal("avail region %d start=%llx size=%llx\n",
		    cnt, addr, size);
		cnt++;
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
