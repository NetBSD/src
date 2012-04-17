/*	$NetBSD: hd64465.c,v 1.15.2.1 2012/04/17 00:06:25 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: hd64465.c,v 1.15.2.1 2012/04/17 00:06:25 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/boot_flag.h>
#include <sys/bus.h>

#include <machine/intr.h>
#include <machine/debug.h>

#include <sh3/intcreg.h>
#include <hpcsh/dev/hd64465/hd64465var.h>
#include <hpcsh/dev/hd64465/hd64465reg.h>
#include <hpcsh/dev/hd64465/hd64465intcreg.h>

/* HD64465 modules. */
STATIC const struct hd64465_module {
	const char *name;
} hd64465_modules[] = {
	[HD64465_MODULE_PS2IF]		= { "hd64465ps2if" },
	[HD64465_MODULE_PCMCIA]		= { "hd64465pcmcia" },
	[HD64465_MODULE_AFE]		= { "hd64465afe" },
	[HD64465_MODULE_GPIO]		= { "hd64465gpio" },
	[HD64465_MODULE_KBC]		= { "hd64465kbc" },
	[HD64465_MODULE_IRDA]		= { "hd64465irda" },
	[HD64465_MODULE_UART]		= { "hd64465uart" },
	[HD64465_MODULE_PARALEL]	= { "hd64465paralel" },
	[HD64465_MODULE_CODEC]		= { "hd64465codec" },
	[HD64465_MODULE_OHCI]		= { "hd64465ohci" },
	[HD64465_MODULE_ADC]		= { "hd64465adc" }
};

STATIC int hd64465_match(device_t, cfdata_t, void *);
STATIC void hd64465_attach(device_t, device_t, void *);
STATIC int hd64465_print(void *, const char *);
#ifdef DEBUG
STATIC void hd64465_info(void);
#endif

CFATTACH_DECL_NEW(hd64465if, 0,
    hd64465_match, hd64465_attach, NULL, NULL);

int
hd64465_match(device_t parent, cfdata_t cf, void *aux)
{

	if (strcmp("hd64465if", cf->cf_name))
		return (0);

	if (hd64465_reg_read_2(HD64465_SDIDR) != 0x8122) {
		/* not HD64465 */
		return (0);
	}

	return (1);
}

void
hd64465_attach(device_t parent, device_t self, void *aux)
{
	const struct hd64465_module *module;
	struct hd64465_attach_args ha;
	uint16_t r;
	size_t i;

	printf("\n");
#ifdef DEBUG
	if (bootverbose)
		hd64465_info();
#endif

	r = hd64465_reg_read_2(HD64465_SRR);
	printf("%s: HITACHI HD64465 rev. %d.%d\n", self->dv_xname,
	    (r >> 8) & 0xff, r & 0xff);

	/* Mask all interrupt */
	hd64465_reg_write_2(HD64465_NIMR, 0xffff);
	/* Edge trigger mode to clear pending interrupt. */
	hd64465_reg_write_2(HD64465_NITR, 0xffff);
	/* Clear pending interrupt */
	hd64465_reg_write_2(HD64465_NIRR, 0x0000);

	/* Attach all sub modules */
	for (i = 0, module = hd64465_modules;
	    i < __arraycount(hd64465_modules);
	    i++, module++) {
		if (module->name == 0)
			continue;
		ha.ha_module_id = i;
		config_found(self, &ha, hd64465_print);
	}	
}

int
hd64465_print(void *aux, const char *pnp)
{
	struct hd64465_attach_args *ha = aux;

	if (pnp)
		aprint_normal("%s at %s",
		    hd64465_modules[ha->ha_module_id].name, pnp);

	return (UNCONF);
}

void *
hd64465_intr_establish(int irq, int mode, int level,
    int (*func)(void *), void *arg)
{
	uint16_t r;
	int s;

	s = splhigh();
	/* Trigger type */
	r = hd64465_reg_read_2(HD64465_NITR);
	switch (mode) {
	case IST_PULSE:
		/* FALLTHROUGH */
	case IST_EDGE:
		r |= irq;
		break;
	case IST_LEVEL:
		r &= ~irq;
		break;
	}
	hd64465_reg_write_2(HD64465_NITR, r);

	hd6446x_intr_establish(irq, mode, level, func, arg);
	splx(s);

	return (void *)irq;
}

void
hd64465_intr_disestablish(void *handle)
{

	hd6446x_intr_disestablish(handle);
}

/* For the sake of Windows CE reboot clearly. */
void
hd64465_shutdown(void)
{

	/* Enable all interrupt */
	hd64465_reg_write_2(HD64465_NIMR, 0x0000);

	/* Level trigger mode */
	hd64465_reg_write_2(HD64465_NITR, 0x0000);
}

void
hd64465_info(void)
{
	uint16_t r;

	dbg_bit_print_msg(_reg_read_2(SH4_ICR), "SH4_ICR");
	r = hd64465_reg_read_2(HD64465_NIMR);
	dbg_bit_print_msg(r, "NIMR");
	r = hd64465_reg_read_2(HD64465_NIRR);
	dbg_bit_print_msg(r, "NIRR");
	r = hd64465_reg_read_2(HD64465_NITR);
	dbg_bit_print_msg(r, "NITR");
}
