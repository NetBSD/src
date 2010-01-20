/*	$NetBSD: tr2.c,v 1.3.18.1 2010/01/20 09:04:33 matt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tr2.c,v 1.3.18.1 2010/01/20 09:04:33 matt Exp $");

#include "fb_sbdio.h"
#include "kbms_sbdio.h"
#include "zsc_sbdio.h"
#include "ewskbd_zsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>		/* Set L2-cache size */

#include <machine/autoconf.h>
#include <machine/sbdvar.h>
#define	_SBD_TR2_PRIVATE
#include <machine/sbd_tr2.h>
#include <machine/z8530var.h>

#include <ews4800mips/ews4800mips/cons_machdep.h>

#include <dev/cons.h>

SBD_DECL(tr2);

/* EWS4800/350 mainbus device list */
static const char *tr2_mainbusdevs[] =
{
	"sbdio",
#ifdef notyet
	"vme",
#endif
	NULL
};

/* EWS4800/350 System board device list */
static const struct sbdiodevdesc tr2_sbdiodevs[] = {
	/* name		     addr1	 addr2 irq   flags */
#if NEWSKBD_ZSC > 0
	{ "zsc",	0x1b010000,         -1,	 0, 0x0003 }, /* Z85C30 */
#else
	{ "zkbms",	0x1b010000,         -1,  0, 0x0001 }, /* Z85C30 */
#endif
	{ "zsc",	0x1b011000,         -1,	 2, 0x0000 }, /* Z85C30 */
	{ "mkclock",	0x1b020000,         -1,	-1, 0x0000 }, /* MK48T08 */
	{ "fdc",	0x1b030000,         -1,	 7, 0x0000 }, /* uPD72065 */
	{ "lpt",	0x1b040000,         -1,	 7, 0x0000 },
	{ "osiop",	0x1b050000,         -1,	 5, 0x0000 }, /* 53C710 */
	{ "iee",	0x1b060000,         -1,	 6, 0x0000 }, /* i82596CA */
	/* XXX: fb should be attached at independent bus */
	{ "fb",		0xf0000000, 0xf5f00000,	-1, 0x0000 },
	{ NULL,		        -1,         -1, -1,     -1 } /* terminate */
};

void
tr2_init(void)
{

	/* EWS4800/350 and EWS4800/350F */
	platform.machine = MACHINE_TR2;
	strcpy(platform.name, "EWS4800/350 (TR2)");

	/* 50MHz external clock, 100MHz internal clock */
	platform.cpu_clock = 100000000;
	curcpu()->ci_cpu_freq = platform.cpu_clock;

	platform.mainbusdevs = tr2_mainbusdevs;
	platform.sbdiodevs = tr2_sbdiodevs;

	ipl_sr_bits = tr2_sr_bits;

	kseg2iobufsize = 0x02000000;	/* 32MB for VME and framebuffer */

	/* Register system-board specific ops. */
	_SBD_OPS_REGISTER_ALL(tr2);
}

int
tr2_ipl_bootdev(void)
{

	return *NVSRAM_BOOTDEV;
}

void
tr2_cache_config(void)
{

	mips_cache_info.mci_sdcache_size = 1024 * 1024;	/* 1MB L2-cache */
}

void
tr2_wbflush(void)
{

	*(volatile uint8_t *)MIPS_KSEG1_START;
}


void
tr2_consinit(void)
{
	extern struct consdev consdev_zs_sbdio;
	extern const struct cdevsw zstty_cdevsw;
	extern int fb_sbdio_cnattach(uint32_t, uint32_t, int);
	extern void kbd_sbdio_cnattach(uint32_t, uint32_t);
	extern void ewskbd_zsc_cnattach(uint32_t, uint32_t, int);

	switch (*NVSRAM_CONSTYPE) {
#if NFB_SBDIO > 0
	case 0x00:
		/* ga */
		cons.type = CONS_FB_KSEG2;
		if (fb_sbdio_cnattach(TR2_GAFB_ADDR, TR2_GACTRL_ADDR, 0x0000))
			break;
#if NEWSKBD_ZSC > 0
		ewskbd_zsc_cnattach(TR2_KBMS_ADDR, TR2_KBMS_ADDR + 4, 4915200);
#else
#if NKBMS_SBDIO > 0
		kbd_sbdio_cnattach(TR2_KBMS_ADDR, TR2_KBMS_ADDR + 4);
#endif
#endif
		return;
#endif
	case 0x01:
#if NZSC_SBDIO > 0
		/* sio 1 (zs channel A) */
		cons.type = CONS_SIO1;
		zs_consaddr = (void *)(TR2_SIO_ADDR + 8);
		cn_tab = &consdev_zs_sbdio;
		cn_tab->cn_pri = CN_REMOTE;
		cn_tab->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
		(*cn_tab->cn_init)(cn_tab);
		return;
	case 0x02:
		/* sio 2 (zs channel B) */
		cons.type = CONS_SIO2;
		zs_consaddr = (void *)(TR2_SIO_ADDR + 0);
		cn_tab = &consdev_zs_sbdio;
		cn_tab->cn_pri = CN_REMOTE;
		cn_tab->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 1);
		(*cn_tab->cn_init)(cn_tab);
		return;
#endif
	default:
		break;
	}
	rom_cons_init();	/* XXX */
}

void
tr2_reboot(void)
{

	*SOFTRESET_REG |= 0x10;
	for (;;)
		;
	/*NOTREACHED */
}

void
tr2_poweroff(void)
{

	for (;;)
		*POWEROFF_REG = 1;
	/* NOTREACHED */
}

void
tr2_mem_init(void *kernstart, void *kernend)
{

	sbd_memcluster_setup(kernstart, kernend);
#if 0
	sbd_memcluster_check();
#endif
}

void
tr2_ether_addr(uint8_t *p)
{
	int i;

	for (i = 0; i < 6; i++)
		p[i] = *(NVSRAM_ETHERADDR + i * 4);
}

void tr2_bell(dev_t dev, u_int pitch, u_int period, u_int volue);

void
tr2_bell(dev_t dev, u_int pitch, u_int period, u_int volue)
{

	*BUZZER_REG = volue > 0 ? 1 : 0;
}
