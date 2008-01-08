/*	$NetBSD: tr2a.c,v 1.1.66.1 2008/01/08 22:09:46 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: tr2a.c,v 1.1.66.1 2008/01/08 22:09:46 bouyer Exp $");

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
#define	_SBD_TR2A_PRIVATE
#include <machine/sbd_tr2a.h>
#include <machine/z8530var.h>

#include <ews4800mips/ews4800mips/cons_machdep.h>

#include <dev/cons.h>

SBD_DECL(tr2a);

/* EWS4800/360 bus list */
static const char *tr2a_mainbusdevs[] = {
	"sbdio",
#ifdef notyet
	"apbus",
	"vme",
#endif
	NULL
};

/* EWS4800/360 System board device list */
static const struct sbdiodevdesc tr2a_sbdiodevs[] = {
	/* name		     addr1       addr2 irq  flags */
	{ "lance",	0x1e400000,         -1,  0, 0x0000 }, /* AM79C90 */
	{ "zsc",	0x1e440000,         -1,	 4, 0x0000 }, /* Z85230 */
#if NEWSKBD_ZSC > 0
	{ "zsc",	0x1e480000,         -1,	 9, 0x0001 }, /* Z85230 */
#else
	{ "zkbms",	0x1e480000,         -1,	 9, 0x0000 }, /* Z85230 */
#endif
	{ "mkclock",	0x1e490000,         -1,	-1, 0x0001 }, /* MK48T18 */
	{ "osiop",	0x1e500000,         -1,	 6, 0x0001 }, /* 53C710 */
	{ "osiop",	0x1e510000,         -1,	10, 0x0001 }, /* 53C710 */
/*	0xbe46 MUSE			*/
/*	0xbe42 FDC			*/
/*	0xbe43 LPT			*/
/*	0xbe40-0xbe47 LR?		*/
/*	0xbe80 LR4370-> APbus,VME	*/
	/* 72069 */
	/* XXX: maybe fb should be attached at independent bus */
	{ "fb",		0xf0000000, 0xf5f00000,	-1, 0x0001 },
	{ NULL,		        -1,         -1, -1, 0x0000 } /* terminate */
};

static const struct sbdiodevdesc tr2a_sbdiodevs_nofb[] = {
	/* name		     addr1       addr2 irq  flags */
	{ "lance",	0x1e400000,         -1,  0, 0x0000 }, /* AM79C90 */
	{ "zsc",	0x1e440000,         -1,	 4, 0x0000 }, /* Z85230 */
#if NEWSKBD_ZSC > 0
	{ "zsc",	0x1e480000,         -1,	 9, 0x0001 }, /* Z85230 */
#else
	{ "zkbms",	0x1e480000,         -1,	 9, 0x0000 }, /* Z85230 */
#endif
	{ "mkclock",	0x1e490000,         -1,	-1, 0x0001 }, /* MK48T18 */
	{ "osiop",	0x1e500000,         -1,	 6, 0x0001 }, /* 53C710 */
	{ "osiop",	0x1e510000,         -1,	10, 0x0001 }, /* 53C710 */
	{ NULL,		        -1,         -1, -1, 0x0000 } /* terminate */
};

void
tr2a_init(void)
{
	char model_name[32];
	int have_fb_sbdio;

	have_fb_sbdio = 0;

	platform.machine = MACHINE_TR2A;
	strlcpy(model_name, SBD_INFO->model_name, sizeof(model_name));
	if (model_name[0] == '\0') {
		if (SBD_INFO->model == 0x101f)
			strcpy(model_name, "EWS4800/360AD");
		have_fb_sbdio = 1;
	}

	if (strcmp(model_name, "EWS4800/360ADII") == 0) {
		platform.cpu_clock = 200000000;
		have_fb_sbdio = 1;
	} else if (strcmp(model_name, "EWS4800/360AD") == 0) {
		platform.cpu_clock = 150000000;
		have_fb_sbdio = 1;
	} else if (strcmp(model_name, "EWS4800/360SX") == 0) {
		/* framebuffer is on APbus */
		platform.cpu_clock = 200000000;
	} else if (strcmp(model_name, "EWS4800/360EX") == 0) {
		/* untested */
		platform.cpu_clock = 200000000;
	} else if (strcmp(model_name, "EWS4800/360") == 0) {
		/* untested */
		platform.cpu_clock = 133333333;
	} else {
		printf("UNKNOWN model %s\n", model_name);
		platform.cpu_clock = 133333333;
	}

	curcpu()->ci_cpu_freq = platform.cpu_clock;

	snprintf(platform.name, sizeof(platform.name), "%s (TR2A)", model_name);
	platform.mainbusdevs = tr2a_mainbusdevs;
	platform.sbdiodevs = tr2a_sbdiodevs_nofb;
	if (have_fb_sbdio)
		platform.sbdiodevs = tr2a_sbdiodevs;

	ipl_sr_bits = tr2a_sr_bits;

	kseg2iobufsize = 0x02000000;	/* 32MB for APbus and framebuffer */

	/* Register system-board specific ops. */
	_SBD_OPS_REGISTER_ALL(tr2a);
}

int
tr2a_ipl_bootdev(void)
{

	return *NVSRAM_BOOTDEV;
}

void
tr2a_cache_config(void)
{

	mips_sdcache_size = 1024 * 1024;	/* 1MB L2-cache */
}

void
tr2a_wbflush(void)
{

	*(volatile uint32_t *)0xbe492010;
	*(volatile uint32_t *)0xbe492010;
	*(volatile uint32_t *)0xbe492010;
	*(volatile uint32_t *)0xbe492010;

#if 0	/* notyet */
	if (has_apbus)
		*(volatile uint32_t *)0xbe806000;
#endif
}

void
tr2a_mem_init(void *kernstart, void *kernend)
{

	sbd_memcluster_setup(kernstart, kernend);
#if 0
	sbd_memcluster_check();
#endif
}

void
tr2a_consinit(void)
{
	extern struct consdev consdev_zs_sbdio;
	extern const struct cdevsw zstty_cdevsw;
	extern int fb_sbdio_cnattach(uint32_t, uint32_t, int);
	extern void kbd_sbdio_cnattach(uint32_t, uint32_t);
	extern void ewskbd_zsc_cnattach(uint32_t, uint32_t, int);

	switch (*NVSRAM_CONSTYPE) {
#if NFB_SBDIO > 0
	case 0x80:
		/* on-board FB on 360AD, 360ADII */
		cons.type = CONS_FB_KSEG2;
		if (fb_sbdio_cnattach(TR2A_GAFB_ADDR, TR2A_GAREG_ADDR, 0x0001))
			break;
#if NEWSKBD_ZSC > 0
		ewskbd_zsc_cnattach(TR2A_KBMS_BASE, TR2A_KBMS_BASE + 4,
		    4915200);
#else
#if NKBMS_SBDIO > 0
		kbd_sbdio_cnattach(TR2A_KBMS_BASE, TR2A_KBMS_BASE + 4);
#endif
#endif
		return;
#endif
#if NZSC_SBDIO > 0
	case 0x01:
		/* sio 1 (zs channel A) */
		cons.type = CONS_SIO1;
		zs_consaddr = (void *)(TR2A_SIO_BASE + 8);
		cn_tab = &consdev_zs_sbdio;
		cn_tab->cn_pri = CN_REMOTE;
		cn_tab->cn_dev =
		    makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
		(*cn_tab->cn_init)(cn_tab);
		return;
	case 0x02:
		/* sio 2 (zs channel B) */
		cons.type = CONS_SIO2;
		zs_consaddr = (void *)(TR2A_SIO_BASE + 0);
		cn_tab = &consdev_zs_sbdio;
		cn_tab->cn_pri = CN_REMOTE;
		cn_tab->cn_dev =
		    makedev(cdevsw_lookup_major(&zstty_cdevsw), 1);
		(*cn_tab->cn_init)(cn_tab);
		return;
#endif
	default:
		break;
	}
	rom_cons_init();	/* XXX */
}

void
tr2a_reboot(void)
{

	*SOFTRESET_FLAG |= 0x80000000;
	*SOFTRESET_REG = 0x01;
	*(uint8_t *)0xbfbffffc = 0xff;
	for (;;)
		;
	/*NOTREACHED */
}

void
tr2a_poweroff(void)
{

	for (;;)
		*POWEROFF_REG = 1;
	/* NOTREACHED */
}

void
tr2a_ether_addr(uint8_t *p)
{
	int i;

	for (i = 0; i < 6; i++)
		p[i] = *(uint8_t *)(NVSRAM_ETHERADDR + i * 4);
}
