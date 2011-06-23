/*	$NetBSD: wzero3_machdep.c,v 1.2 2011/06/23 12:40:32 nonaka Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *      This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wzero3_machdep.c,v 1.2 2011/06/23 12:40:32 nonaka Exp $");

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/termios.h>

#include <uvm/uvm.h>

#include <machine/bootconfig.h>
#include <machine/bootinfo.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include "com.h"
#if (NCOM > 0)
#include "opt_com.h"
#include <dev/ic/comvar.h>
#endif	/* NCOM > 0 */
#include "lcd.h"
#include "wzero3lcd.h"

#if (NCOM > 0) && defined(COM_PXA2X0)
#ifndef	CONSPEED
#define	CONSPEED 9600
#endif
#ifndef	CONMODE
#define	CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int comcnspeed = CONSPEED;
int comcnmode = CONMODE;

#if defined(HWUARTCONSOLE)
#define	CONADDR	PXA2X0_HWUART_BASE
#elsif defined(BTUARTCONSOLE)
#define	CONADDR	PXA2X0_BTUART_BASE
#elsif defined(STUARTCONSOLE)
#define	CONADDR	PXA2X0_STUART_BASE
#else
#define	CONADDR	PXA2X0_FFUART_BASE
#endif

bus_addr_t comcnaddr = CONADDR;
#endif	/* NCOM > 0 && COM_PXA2X0 */

const struct pmap_devmap machdep_devmap[] = { { 0, 0, 0, 0, 0, } };

void pxa2x0_machdep_init(void);


static void
ws003sh_cpu_reset(void)
{
	uint32_t rv;

	rv = pxa2x0_memctl_read(MEMCTL_MSC0);
	if ((rv & 0xffff0000) == 0x7ff00000) {
		pxa2x0_memctl_write(MEMCTL_MSC0, (rv & 0xffff) | 0x7ee00000);
	}

	pxa2x0_gpio_set_function(89, GPIO_OUT | GPIO_SET);
	for (;;)
		continue;
}

static struct pxa2x0_gpioconf ws003sh_boarddep_gpioconf[] = {
	/* FFUART */
	{  98, GPIO_ALT_FN_3_OUT },	/* FFRTS */
	{  99, GPIO_ALT_FN_3_OUT },	/* FFTXD */
	/* SSP3 */
	{  34, GPIO_ALT_FN_3_OUT },	/* SSPSCLK3 */
	{  38, GPIO_ALT_FN_1_OUT },	/* SSPTXD3 */
	{  82, GPIO_ALT_FN_1_IN },	/* SSPRXD3 */

	{ -1 }
};

static struct pxa2x0_gpioconf ws007sh_boarddep_gpioconf[] = {
	/* FFUART */
	{  98, GPIO_ALT_FN_3_OUT },	/* FFRTS */
	{  99, GPIO_ALT_FN_3_OUT },	/* FFTXD */
	/* SSP2 */
	{  19, GPIO_ALT_FN_1_OUT },	/* SSPSCLK2 */
	{  86, GPIO_ALT_FN_1_IN },	/* SSPRXD2 */
	{  87, GPIO_ALT_FN_1_OUT },	/* SSPTXD2 */
	/* SSP3 */
	{  38, GPIO_ALT_FN_1_OUT },	/* SSPTXD3 */
	{  52, GPIO_ALT_FN_2_OUT },	/* SSPSCLK3 */
	{  89, GPIO_ALT_FN_1_IN },	/* SSPRXD3 */

	{ -1 }
};

static struct pxa2x0_gpioconf ws011sh_boarddep_gpioconf[] = {
	/* FFUART */
	{  98, GPIO_ALT_FN_3_OUT },	/* FFRTS */
	{  99, GPIO_ALT_FN_3_OUT },	/* FFTXD */
	/* SSP2 */
	{  19, GPIO_ALT_FN_1_OUT },	/* SSPSCLK2 */
	{  86, GPIO_ALT_FN_1_IN },	/* SSPRXD2 */
	{  87, GPIO_ALT_FN_1_OUT },	/* SSPTXD2 */

	{ -1 }
};

static struct pxa2x0_gpioconf *ws003sh_gpioconf[] = {
	pxa27x_com_ffuart_gpioconf,
	pxa27x_pxamci_gpioconf,
	pxa27x_ohci_gpioconf,
	ws003sh_boarddep_gpioconf,
	NULL
};

static struct pxa2x0_gpioconf *ws007sh_gpioconf[] = {
	pxa27x_com_ffuart_gpioconf,
	pxa27x_pxamci_gpioconf,
	pxa27x_ohci_gpioconf,
	ws007sh_boarddep_gpioconf,
	NULL
};

static struct pxa2x0_gpioconf *ws011sh_gpioconf[] = {
	pxa27x_com_ffuart_gpioconf,
	pxa27x_pxamci_gpioconf,
	pxa27x_ohci_gpioconf,
	ws011sh_boarddep_gpioconf,
	NULL
};

void
pxa2x0_machdep_init(void)
{
	extern void (*__cpu_reset)(void);
	extern BootConfig bootconfig;		/* Boot config storage */

	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS003SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS004SH)) {
		pxa2x0_gpio_config(ws003sh_gpioconf);
		__cpu_reset = ws003sh_cpu_reset;
	} else if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS007SH)) {
		pxa2x0_gpio_config(ws007sh_gpioconf);
	} else if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH)) {
		pxa2x0_gpio_config(ws011sh_gpioconf);
	}
	pxa2x0_clkman_config(CKEN_FFUART, 1);
	pxa2x0_clkman_config(CKEN_OST, 1);
	pxa2x0_clkman_config(CKEN_USBHC, 0);
	pxa2x0_clkman_config(CKEN_USBDC, 0);
	pxa2x0_clkman_config(CKEN_AC97, 0);
	pxa2x0_clkman_config(CKEN_SSP, 0);
	pxa2x0_clkman_config(CKEN_HWUART, 0);
	pxa2x0_clkman_config(CKEN_STUART, 0);
	pxa2x0_clkman_config(CKEN_BTUART, 0);
	pxa2x0_clkman_config(CKEN_I2S, 0);
	pxa2x0_clkman_config(CKEN_MMC, 0);
	pxa2x0_clkman_config(CKEN_FICP, 0);
	pxa2x0_clkman_config(CKEN_I2C, 0);
	pxa2x0_clkman_config(CKEN_PWM1, 0);
	if (!platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH)) {
		pxa2x0_clkman_config(CKEN_PWM0, 0); /* WS011SH: DON'T DISABLE */
	}

	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS003SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS004SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS007SH)) {
		bootconfig.dram[0].pages = 16384; /* 64MiB */
	} else
	if (platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS011SH)
	 || platid_match(&platid, &platid_mask_MACH_SHARP_WZERO3_WS020SH)) {
		bootconfig.dram[0].pages = 32768; /* 128MiB */
	}
}

void
consinit(void)
{
	static int consinit_called = 0;

	if (consinit_called != 0)
		return;

	consinit_called = 1;

	if (bootinfo->bi_cnuse == BI_CNUSE_SERIAL) {
#if (NCOM > 0) && defined(COM_PXA2X0)
		comcnattach(&pxa2x0_a4x_bs_tag, comcnaddr, comcnspeed,
		    PXA2X0_COM_FREQ, COM_TYPE_PXA2x0, comcnmode);
#endif
	} else {
#if (NLCD > 0)
#if NWZERO3LCD > 0
		extern void wzero3lcd_cnattach(void);
		wzero3lcd_cnattach();
		return;
#endif
#endif
	}
}
