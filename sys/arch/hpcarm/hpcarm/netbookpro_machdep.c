/*	$NetBSD: netbookpro_machdep.c,v 1.3 2023/04/21 15:00:27 skrll Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbookpro_machdep.c,v 1.3 2023/04/21 15:00:27 skrll Exp $");

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/param.h>

#include <uvm/uvm.h>

#include <machine/bootconfig.h>
#include <machine/pmap.h>

#include <arm/xscale/pxa2x0_gpio.h>

#include <hpcarm/dev/nbppconvar.h>

#include <dev/cons.h>
#include <dev/i2c/i2cvar.h>

#include "nbppcon.h"

#include "biconsdev.h"
#if NBICONSDEV > 0
#include <dev/hpc/bicons.h>
cons_decl(bicons);
#endif
#include "wsdisplay.h"
#if NWSDISPLAY == 0
#include <dev/wscons/wskbdvar.h>
#endif

static void netbookpro_reset(void);
void pxa2x0_machdep_init(void);

static int __unused enable_console(void (*)(struct consdev *),
				   void (*)(struct consdev *));
static void disable_consoles(void);
static void cn_nonprobe(struct consdev *);

const struct pmap_devmap machdep_devmap[] = {
	/* Framebuffer */
	DEVMAP_ENTRY(
		0x14000000,
		0x14000000,
		0x00400000
	},
	DEVMAP_ENTRY_END
};

static struct pxa2x0_gpioconf netbookpro_boarddep_gpioconf[] = {
	{  6, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCLK */
	{  8, GPIO_CLR | GPIO_ALT_FN_1_OUT },	/* MMCCS0 */
	{ 52, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPCE1 */
	{ 53, GPIO_CLR | GPIO_ALT_FN_2_OUT },	/* nPCE2 */
	{ -1 }
};

static struct pxa2x0_gpioconf *netbookpro_gpioconf[] = {
	pxa25x_pcic_gpioconf,
	netbookpro_boarddep_gpioconf,
	NULL
};

static void
netbookpro_reset(void)
{
	device_t pcon;

	pcon = device_find_by_xname("nbppcon0");
#if NNBPPCON > 0
	if (pcon != NULL)
		nbppcon_pwr_hwreset(pcon);
#endif

	while (1 /*CONSTCOND*/);

	/* NOTREACHED */
}

void
pxa2x0_machdep_init(void)
{
	extern void (*__cpu_reset)(void);
	extern void (*__sleep_func)(void *);
	extern BootConfig bootconfig;		/* Boot config storage */


	__cpu_reset = netbookpro_reset;
	__sleep_func = NULL;
	bootconfig.dram[0].pages = 32768; /* 128MiB */

	pxa2x0_gpio_config(netbookpro_gpioconf);
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	disable_consoles();

#if NBICONSDEV > 0
	enable_console(biconscninit, biconscnprobe);
	bicons_set_priority(CN_INTERNAL);
	cninit();
#if NWSDISPLAY == 0
	if (cn_tab != NULL) {	/* XXXX */
		cn_tab->cn_getc = wskbd_cngetc;
		cn_tab->cn_pollc = wskbd_cnpollc;
		cn_tab->cn_bell = wskbd_cnbell;
	}
#endif
#endif
}

static int __unused
enable_console(void (*init)(struct consdev *), void (*probe)(struct consdev *))
{
	struct consdev *cp;

	for (cp = constab; cp->cn_probe; cp++) {
		if (cp->cn_init == init) {
			cp->cn_probe = probe;
			return 1;
		}
	}
	return 0;
}

static void
disable_consoles(void)
{
	struct consdev *cp;

	for (cp = constab; cp->cn_probe; cp++)
		cp->cn_probe = cn_nonprobe;
}

static void
cn_nonprobe(struct consdev *cp)
{

	cp->cn_pri = CN_DEAD;
}
