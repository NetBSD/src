/*	$NetBSD: hd64465.c,v 1.2 2002/03/24 18:21:26 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/boot_flag.h>

#include <sh3/exception.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/debug.h>

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
#define HD64465_NMODULE							\
	(sizeof hd64465_modules / sizeof(struct hd64465_module))

STATIC struct hd64465_intr_entry {
	int (*func)(void *);
	void *arg;
	int priority;
	const u_int16_t bit;
} hd64465_intr_entry[] = {
#define IRQ_ENTRY(x)	[HD64465_IRQ_ ## x] = { 0, 0, 0, HD64465_ ## x }
	IRQ_ENTRY(PS2KB),
	IRQ_ENTRY(PCC0),
	IRQ_ENTRY(PCC1),
	IRQ_ENTRY(AFE),
	IRQ_ENTRY(GPIO),
	IRQ_ENTRY(TMU0),
	IRQ_ENTRY(TMU1),
	IRQ_ENTRY(KBC),
	IRQ_ENTRY(PS2MS),
	IRQ_ENTRY(IRDA),
	IRQ_ENTRY(UART),
	IRQ_ENTRY(PPR),
	IRQ_ENTRY(SCDI),
	IRQ_ENTRY(OHCI),
	IRQ_ENTRY(ADC)
#undef IRQ_ENTRY
};
#define HD64465_IRQ_MAX							\
	(sizeof hd64465_intr_entry / sizeof(struct hd64465_intr_entry))

STATIC struct hd64465 {
	int attached;
	u_int16_t imask;
} hd64465;

STATIC int hd64465_match(struct device *, struct cfdata *, void *);
STATIC void hd64465_attach(struct device *, struct device *, void *);
STATIC int hd64465_print(void *, const char *);
STATIC int hd64465_intr(void *);

struct cfattach hd64465if_ca = {
	sizeof(struct device), hd64465_match, hd64465_attach
};

int
hd64465_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (hd64465.attached++)	
		return (0);	/* only one instance */

	if (strcmp("hd64465if", cf->cf_driver->cd_name))
		return (0);

	if (hd64465_reg_read_2(HD64465_SDIDR) != 0x8122) {
		/* not HD64465 */
		return (0);
	}

	return (1);
}

void
hd64465_attach(struct device *parent, struct device *self, void *aux)
{
	const struct hd64465_module *module;
	struct hd64465_attach_args ha;
	u_int16_t r;
	int i;

	printf("\n");

	r = hd64465_reg_read_2(HD64465_SRR);
	printf("%s: HITACHI HD64465 rev. %d.%d\n", self->dv_xname,
	    (r >> 8) & 0xff, r & 0xff);

	hd64465_intr_disable();

	/* Attach all sub modules */
	for (i = 0, module = hd64465_modules; i < HD64465_NMODULE;
	    i++, module++) {
		if (module->name == 0)
			continue;
		ha.ha_module_id = i;
		config_found(self, &ha, hd64465_print);
	}	

	intc_intr_establish(SH_INTEVT_IRL11, IST_LEVEL, IPL_TTY,
	    hd64465_intr, self);
}

int
hd64465_print(void *aux, const char *pnp)
{
	struct hd64465_attach_args *ha = aux;

	if (pnp)
		printf("%s at %s", hd64465_modules[ha->ha_module_id].name, pnp);

	return (UNCONF);
}

void *
hd64465_intr_establish(enum hd64465_irq irq, int mode, int level,
    int (*func)(void *), void *arg)
{
	struct hd64465_intr_entry *entry = &hd64465_intr_entry[irq];
	u_int16_t r, bit;
	int s;

	s = splhigh();

	entry->func = func;
	entry->arg = arg;
	entry->priority = level;

	bit = entry->bit;

	/* Trigger type */
	r = hd64465_reg_read_2(HD64465_NITR);
	switch (mode) {
	case IST_PULSE:
		/* FALLTHROUGH */
	case IST_EDGE:
		r |= bit;
		break;
	case IST_LEVEL:
		r &= ~bit;
		break;
	}
	hd64465_reg_write_2(HD64465_NITR, r);

	/* Enable interrupt */
	hd64465.imask &= ~bit;
	hd64465_reg_write_2(HD64465_NIMR, hd64465.imask);

	splx(s);

	return (void *)irq;
}

void
hd64465_intr_disestablish(void *handle)
{
	int irq = (int)handle;
	struct hd64465_intr_entry *entry = &hd64465_intr_entry[irq];
	int s;
	
	s = splhigh();	

	/* disable interrupt */
	hd64465.imask |= entry->bit;
	hd64465_reg_write_2(HD64465_NIMR, hd64465.imask);

	entry->func = 0;

	splx(s);
}

int
hd64465_intr(void *arg)
{
	struct hd64465_intr_entry *entry = hd64465_intr_entry;
	u_int16_t cause;
	int i;

	cause = hd64465_reg_read_2(HD64465_NIRR) & ~hd64465.imask;
	hd64465_reg_write_2(HD64465_NIRR, 0);

	for (i = 0; i < HD64465_IRQ_MAX; i++, entry++) {
		if (entry->func == 0)
			continue;
		if (cause & entry->bit)
			(*entry->func)(entry->arg);
	}

	__dbg_heart_beat(HEART_BEAT_BLUE);

	return (0);
}

void
hd64465_intr_disable()
{

	/* Mask all interrupt */
	hd64465.imask = 0xffff;
	hd64465_reg_write_2(HD64465_NIMR, 0xffff);

	/* Edge trigger mode */
	hd64465_reg_write_2(HD64465_NITR, 0xffff);
	/* Clear pending interrupt */
	hd64465_reg_write_2(HD64465_NIRR, 0x0000);
}

void
hd64465_intr_mask()
{

	hd64465_reg_write_2(HD64465_NIMR, 0xffff);
}

void
hd64465_intr_unmask()
{

	hd64465_reg_write_2(HD64465_NIMR, hd64465.imask);
}

/* For the sake of Windows CE reboot clearly. */
void
hd64465_intr_reboot()
{

	/* Enable all interrupt */
	hd64465_reg_write_2(HD64465_NIMR, 0x0000);

	/* Level trigger mode */
	hd64465_reg_write_2(HD64465_NITR, 0x0000);
}
