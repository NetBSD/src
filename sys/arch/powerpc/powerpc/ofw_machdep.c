/*	$NetBSD: ofw_machdep.c,v 1.36 2022/12/12 13:26:46 martin Exp $	*/

/*-
 * Copyright (c) 2007, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
__KERNEL_RCSID(0, "$NetBSD: ofw_machdep.c,v 1.36 2022/12/12 13:26:46 martin Exp $");

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

#include <dev/cons.h>
#include <dev/ofw/openfirm.h>

#include <machine/powerpc.h>
#include <machine/autoconf.h>

#include <powerpc/ofw_machdep.h>

#ifdef DEBUG
#define DPRINTF ofprint
#else
#define DPRINTF while(0) printf
#endif

#define	ofpanic(FORMAT, ...)	do {				\
		ofprint(FORMAT __VA_OPT__(,) __VA_ARGS__);	\
		panic(FORMAT __VA_OPT__(,) __VA_ARGS__);	\
	} while (0)

int	ofw_root;
int	ofw_chosen;

bool	ofw_real_mode;

/*
 * Bootstrap console support functions.
 */

int	console_node = -1, console_instance = -1;
int	ofw_stdin, ofw_stdout;
bool	ofwbootcons_suppress;

int	ofw_address_cells;
int	ofw_size_cells;

void ofprint(const char *blah, ...)
{
	va_list va;
	char buf[256];
	int len;

	va_start(va, blah);
	len = vsnprintf(buf, sizeof(buf), blah, va);
	va_end(va);
	OF_write(console_instance, buf, len);
	/* Apple OF only does a newline on \n, so add an explicit CR */
	if ((len > 0) && (buf[len - 1] == '\n'))
		OF_write(console_instance, "\r", 1);
}

static int
ofwbootcons_cngetc(dev_t dev)
{
	unsigned char ch = '\0';
	int l;

	if (ofwbootcons_suppress) {
		return ch;
	}

	while ((l = OF_read(ofw_stdin, &ch, 1)) != 1) {
		if (l != -2 && l != 0) {
			return -1;
		}
	}
	return ch;
}

static void
ofwbootcons_cnputc(dev_t dev, int c)
{
	char ch = c;

	if (ofwbootcons_suppress) {
		return;
	}

	OF_write(ofw_stdout, &ch, 1);
}

static struct consdev consdev_ofwbootcons = {
	.cn_getc = ofwbootcons_cngetc,
	.cn_putc = ofwbootcons_cnputc,
	.cn_pollc = nullcnpollc,
	.cn_dev = NODEV,
	.cn_pri = CN_INTERNAL,
};

static void
ofw_bootstrap_console(void)
{
	int node;

	if (ofw_chosen == -1) {
		goto nocons;
	}

	if (OF_getprop(ofw_chosen, "stdout", &ofw_stdout,
		       sizeof(ofw_stdout)) != sizeof(ofw_stdout))
		goto nocons;

	if (OF_getprop(ofw_chosen, "stdin", &ofw_stdin,
		       sizeof(ofw_stdin)) != sizeof(ofw_stdin))
		goto nocons;
	if (ofw_stdout == 0) {
		/* screen should be console, but it is not open */
		ofw_stdout = OF_open("screen");
	}
	node = OF_instance_to_package(ofw_stdout);
	console_node = node;
	console_instance = ofw_stdout;

	cn_tab = &consdev_ofwbootcons;

	return;
 nocons:
	ofpanic("No /chosen could be found!\n");
	console_node = -1;
}

#define	OFMEM_REGIONS	32
static struct mem_region OFmem[OFMEM_REGIONS + 1], OFavail[OFMEM_REGIONS + 3];

static void
ofw_bootstrap_get_memory(void)
{
	const char *macrisc[] = {"MacRISC", "MacRISC2", "MacRISC4", NULL};
	int hmem, i, cnt, memcnt, regcnt;
	int numregs;
	uint32_t regs[OFMEM_REGIONS * 4]; /* 2 values + 2 for 64bit */

	int acells = ofw_address_cells;
	int scells = ofw_size_cells;

	DPRINTF("calling mem_regions\n");

	/* Get memory */
	memset(regs, 0, sizeof(regs));
	if ((hmem = OF_finddevice("/memory")) == -1)
		goto error;
	regcnt = OF_getprop(hmem, "reg", regs,
	    sizeof(regs[0]) * OFMEM_REGIONS * 4);
	if (regcnt <= 0)
		goto error;

	/* how many mem regions did we get? */
	numregs = regcnt / (sizeof(uint32_t) * (acells + scells));
	DPRINTF("regcnt=%d num=%d acell=%d scell=%d\n",
	    regcnt, numregs, acells, scells);

	/* move the data into OFmem */
	memset(OFmem, 0, sizeof(OFmem));
	for (i = 0, memcnt = 0; i < numregs; i++) {
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
			ofprint("Base addr of %llx or size of %llx too"
			    " large for 32 bit OS. Skipping.", addr, size);
			continue;
		}
#endif
		OFmem[memcnt].start = addr;
		OFmem[memcnt].size = size;
		DPRINTF("mem region %d start=%"PRIx64" size=%"PRIx64"\n",
		    memcnt, addr, size);
		memcnt++;
	}

	DPRINTF("available\n");

	/* now do the same thing again, for the available counts */
	memset(regs, 0, sizeof(regs));
	regcnt = OF_getprop(hmem, "available", regs,
	    sizeof(regs[0]) * OFMEM_REGIONS * 4);
	if (regcnt <= 0)
		goto error;

	DPRINTF("%08x %08x %08x %08x\n", regs[0], regs[1], regs[2], regs[3]);

	/*
	 * according to comments in FreeBSD all Apple OF has 32bit values in
	 * "available", no matter what the cell sizes are
	 */
	if (of_compatible(ofw_root, macrisc)) {
		DPRINTF("this appears to be a mac...\n");
		acells = 1;
		scells = 1;
	}
		
	/* how many mem regions did we get? */
	numregs = regcnt / (sizeof(uint32_t) * (acells + scells));
	DPRINTF("regcnt=%d num=%d acell=%d scell=%d\n",
	    regcnt, numregs, acells, scells);

	DPRINTF("to OF_avail\n");

	/* move the data into OFavail */
	memset(OFavail, 0, sizeof(OFavail));
	for (i = 0, cnt = 0; i < numregs; i++) {
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
			ofprint("Base addr of %llx or size of %llx too"
			    " large for 32 bit OS. Skipping.", addr, size);
			continue;
		}
#endif
		OFavail[cnt].start = addr;
		OFavail[cnt].size = size;
		DPRINTF("avail region %d start=%#"PRIx64" size=%#"PRIx64"\n",
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
		    (OFmem[memcnt-1].start + OFmem[memcnt-1].size)) {

			OFavail[cnt].start =
			    (OFavail[cnt-1].start + OFavail[cnt-1].size +
			    AVAIL_THRESH) & ~AVAIL_THRESH;
			OFavail[cnt].size =
			    OFmem[memcnt-1].size - OFavail[cnt].start;
			ofprint("WARNING: add memory segment %lx - %" PRIxPADDR ","
			    "\nWARNING: which was not recognized by "
			    "the Firmware.\n",
			    (unsigned long)OFavail[cnt].start,
			    (unsigned long)OFavail[cnt].start +
			    OFavail[cnt].size);
			cnt++;
		}
	}

	return;

error:
#if defined (MAMBO)
	ofprint("no memory, assuming 512MB\n");

	OFmem[0].start = 0x0;
	OFmem[0].size = 0x20000000;
	
	OFavail[0].start = 0x3000;
	OFavail[0].size = 0x20000000 - 0x3000;

#else
	ofpanic("no memory?");
#endif
	return;
}

static void
ofw_bootstrap_get_translations(void)
{
	/* 5 cells per: virt(1), size(1), phys(2), mode(1) */
	uint32_t regs[OFW_MAX_TRANSLATIONS * 5];
	uint32_t virt, size, mode;
	uint64_t phys;
	uint32_t *rp;
	int proplen;
	int mmu_ihandle, mmu_phandle;
	int idx;

	if (OF_getprop(ofw_chosen, "mmu", &mmu_ihandle,
		       sizeof(mmu_ihandle)) <= 0) {
		ofprint("No /chosen/mmu\n");
		return;
	}
	mmu_phandle = OF_instance_to_package(mmu_ihandle);

	proplen = OF_getproplen(mmu_phandle, "translations");
	if (proplen <= 0) {
		ofprint("No translations in /chosen/mmu\n");
		return;
	}

	if (proplen > sizeof(regs)) {
		ofpanic("/chosen/mmu translations too large");
	}

	proplen = OF_getprop(mmu_phandle, "translations", regs, sizeof(regs));
	int nregs = proplen / sizeof(regs[0]);

	/* Decode into ofw_translations[]. */
	for (idx = 0, rp = regs; rp < &regs[nregs];) {
		virt = *rp++;
		size = *rp++;
		switch (ofw_address_cells) {
		case 1:
			phys = *rp++;
			break;
		case 2:
			phys = *rp++;
			phys = (phys << 32) | *rp++;
			break;
		default:
			ofpanic("unexpected #address-cells");
		}
		mode = *rp++;
		if (rp > &regs[nregs]) {
			ofpanic("unexpected OFW translations format");
		}

		/* Wouldn't expect this, but... */
		if (size == 0) {
			continue;
		}

		DPRINTF("translation %d virt=%#"PRIx32
		    " phys=%#"PRIx64" size=%#"PRIx32" mode=%#"PRIx32"\n",
		    idx, virt, phys, size, mode);
		
		if (sizeof(paddr_t) < 8 && phys >= 0x100000000ULL) {
			ofpanic("translation phys out of range");
		}

		if (idx == OFW_MAX_TRANSLATIONS) {
			ofpanic("too many OFW translations");
		}

		ofw_translations[idx].virt = virt;
		ofw_translations[idx].size = size;
		ofw_translations[idx].phys = (paddr_t)phys;
		ofw_translations[idx].mode = mode;
		idx++;
	}
}

static bool
ofw_option_truefalse(const char *prop, int proplen)
{
	/* These are all supposed to be strings. */
	switch (prop[0]) {
	case 'y':
	case 'Y':
	case 't':
	case 'T':
	case '1':
		return true;
	}
	return false;
}

/*
 * Called from ofwinit() very early in bootstrap.  We are still
 * running on the stack provided by OpenFirmware and in the same
 * OpenFirmware client environment as the boot loader.  Our calls
 * to OpenFirmware are direct, and not via the trampoline that
 * saves / restores kernel state.
 */
void
ofw_bootstrap(void)
{
	char prop[32];
	int handle, proplen;

	/* Stash the handles for "/" and "/chosen" for convenience later. */
	ofw_root = OF_finddevice("/");
	ofw_chosen = OF_finddevice("/chosen");

	/* Initialize the early bootstrap console. */
	ofw_bootstrap_console();

	/* Check to see if we're running in real-mode. */
	handle = OF_finddevice("/options");
	if (handle != -1) {
		proplen = OF_getprop(handle, "real-mode?", prop, sizeof(prop));
		if (proplen > 0) {
			ofw_real_mode = ofw_option_truefalse(prop, proplen);
		} else {
			ofw_real_mode = false;
		}
	}
	DPRINTF("OpenFirmware running in %s-mode\n",
	    ofw_real_mode ? "real" : "virtual");

	/* Get #address-cells and #size-cells to fetching memory info. */
	if (OF_getprop(ofw_root, "#address-cells", &ofw_address_cells,
		       sizeof(ofw_address_cells)) <= 0)
		ofw_address_cells = 1;

	if (OF_getprop(ofw_root, "#size-cells", &ofw_size_cells,
		       sizeof(ofw_size_cells)) <= 0)
		ofw_size_cells = 1;

	/* Get the system memory configuration. */
	ofw_bootstrap_get_memory();

	/* Get any translations used by OpenFirmware. */
	ofw_bootstrap_get_translations();
}

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
	*memp = OFmem;
	*availp = OFavail;
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
