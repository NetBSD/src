/*	$NetBSD: sbus.c,v 1.12.8.1 2016/10/05 20:55:33 skrll Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

/*
 * PlayStation 2 internal PCMCIA/USB interface unit.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbus.c,v 1.12.8.1 2016/10/05 20:55:33 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bootinfo.h>
#include <machine/autoconf.h>

#include <playstation2/playstation2/interrupt.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/intcvar.h>
#include <playstation2/dev/sbusvar.h>
#include <playstation2/dev/sbusreg.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC void sbus_type2_pcmcia_intr_clear(void);
STATIC void sbus_type2_pcmcia_intr_enable(void);
STATIC void sbus_type2_pcmcia_intr_disable(void);
STATIC void sbus_type2_pcmcia_intr_reinstall(void);
STATIC void sbus_type3_pcmcia_intr_clear(void);
STATIC void sbus_type3_pcmcia_intr_enable(void);
STATIC void sbus_type3_pcmcia_intr_disable(void);
STATIC void sbus_type3_pcmcia_intr_reinstall(void);
STATIC int sbus_spurious_intr(void *);

STATIC void (*sbus_pcmcia_intr_clear)(void);
STATIC void (*sbus_pcmcia_intr_enable)(void);
STATIC void (*sbus_pcmcia_intr_disable)(void);
STATIC void (*sbus_pcmcia_intr_reinstall)(void);

STATIC int (*sbus_pcmcia_intr)(void *) = sbus_spurious_intr;
STATIC void *sbus_pcmcia_context;
STATIC int (*sbus_usb_intr)(void *) = sbus_spurious_intr;
STATIC void *sbus_usb_context;

STATIC void sbus_init(int);
STATIC int sbus_intr(void *);

STATIC int sbus_match(device_t, cfdata_t, void *);
STATIC void sbus_attach(device_t, device_t, void *);
STATIC int sbus_search(device_t, cfdata_t,
		       const int *, void *);
STATIC int sbus_print(void *, const char *);

CFATTACH_DECL_NEW(sbus, sizeof (struct device),
    sbus_match, sbus_attach, NULL, NULL);

extern struct cfdriver sbus_cd;
STATIC int __sbus_attached;

int
sbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, sbus_cd.cd_name) != 0)
		return (0);

	return (!__sbus_attached);
}

void
sbus_attach(device_t parent, device_t self, void *aux)
{
	int type = BOOTINFO_REF(BOOTINFO_PCMCIA_TYPE); 

	printf(": controller type %d\n", type);

	/* Initialize SBUS controller */
	sbus_init(type);

	config_search_ia(sbus_search, self, "sbus", 0);
}

int
sbus_search(device_t parent, cfdata_t cf,
	    const int *ldesc, void *aux)
{
	struct sbus_attach_args sa;

	if (config_match(parent, cf, &sa))
		config_attach(parent, cf, &sa, sbus_print);
	
	return (0);
}

int
sbus_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}

void
sbus_init(int type)
{
	/* install model dependent hook */
#define SET_PCMCIA_INTR_OPS(x)	 					\
	sbus_pcmcia_intr_clear = sbus_type##x##_pcmcia_intr_clear;	\
	sbus_pcmcia_intr_enable = sbus_type##x##_pcmcia_intr_enable;	\
	sbus_pcmcia_intr_disable = sbus_type##x##_pcmcia_intr_disable;	\
	sbus_pcmcia_intr_reinstall = sbus_type##x##_pcmcia_intr_reinstall

	switch (type) {
	default:
		panic("unknown pcmcia controller type = %d", type);
		break;
	case 0:
		/* FALLTHROUGH */
	case 1:
		/* FALLTHROUGH */
	case 2:
		SET_PCMCIA_INTR_OPS(2);
		break;
	case 3:
		SET_PCMCIA_INTR_OPS(3);
		break;
	}
#undef SET_PCMCIA_INTR_OPS
	/* disable interrupt */
	(*sbus_pcmcia_intr_disable)();

	/* clear interrupt */
	(*sbus_pcmcia_intr_clear)();
	_reg_write_4(SBUS_SMFLG_REG, SMFLG_PCMCIA_INT);
	_reg_write_4(SBUS_SMFLG_REG, SMFLG_USB_INT);	

	/* connect to INTC */
	intc_intr_establish(I_CH1_SBUS, IPL_BIO, sbus_intr, 0);
}

void *
sbus_intr_establish(enum sbus_irq irq, int (*ih_func)(void *), void *ih_arg)
{
	switch (irq) {
	default:
		panic("unknown IRQ");
		break;
	case SBUS_IRQ_PCMCIA:
		sbus_pcmcia_intr = ih_func;
		sbus_pcmcia_context = ih_arg;
		(*sbus_pcmcia_intr_enable)();
		break;
	case SBUS_IRQ_USB:
		sbus_usb_intr = ih_func;
		sbus_usb_context = ih_arg;
		break;
	}

	return (void *)irq;
}

void
sbus_intr_disestablish(void *handle)
{
	int irq = (int)handle;

	switch (irq) {
	default:
		panic("unknown IRQ");
		break;
	case SBUS_IRQ_PCMCIA:
		sbus_pcmcia_intr = sbus_spurious_intr;
		(*sbus_pcmcia_intr_disable)();
		break;
	case SBUS_IRQ_USB:
		sbus_usb_intr = sbus_spurious_intr;
		break;
	}
}

int
sbus_intr(void *arg)
{
	u_int32_t stat;

	_playstation2_evcnt.sbus.ev_count++;
	stat = _reg_read_4(SBUS_SMFLG_REG);

	if (stat & SMFLG_PCMCIA_INT) {
		(*sbus_pcmcia_intr_clear)();
		_reg_write_4(SBUS_SMFLG_REG, SMFLG_PCMCIA_INT);
		(*sbus_pcmcia_intr)(sbus_pcmcia_context);
	}

	if (stat & SMFLG_USB_INT) {
		_reg_write_4(SBUS_SMFLG_REG, SMFLG_USB_INT);
		(*sbus_usb_intr)(sbus_usb_context);
	}
	
	(*sbus_pcmcia_intr_reinstall)();

	return (1);
}

int
sbus_spurious_intr(void *arg)
{

	printf("spurious interrupt.\n");

	return (1);
}

/* SCPH-18000 */
void
sbus_type2_pcmcia_intr_clear(void)
{

	if (_reg_read_2(SBUS_PCMCIA_CSC1_REG16) & 0x080)
		_reg_write_2(SBUS_PCMCIA_CSC1_REG16, 0xffff);
}

void
sbus_type2_pcmcia_intr_enable(void)
{

	_reg_write_2(SBUS_PCMCIA_TIMR_REG16, 0);
}

void
sbus_type2_pcmcia_intr_disable(void)
{

	_reg_write_2(SBUS_PCMCIA_TIMR_REG16, 1);
}

void
sbus_type2_pcmcia_intr_reinstall(void)
{
	u_int16_t r = _reg_read_2(SBUS_PCMCIA_TIMR_REG16);

	_reg_write_2(SBUS_PCMCIA_TIMR_REG16, 1);
	_reg_write_2(SBUS_PCMCIA_TIMR_REG16, r);
}

/* SCPH-30000/35000 */
void
sbus_type3_pcmcia_intr_clear(void)
{
	/* nothing */
}

void
sbus_type3_pcmcia_intr_enable(void)
{

	_reg_write_2(SBUS_PCMCIA3_TIMR_REG16, 0);
}

void
sbus_type3_pcmcia_intr_disable(void)
{

	_reg_write_2(SBUS_PCMCIA3_TIMR_REG16, 1);
}

void
sbus_type3_pcmcia_intr_reinstall(void)
{
	u_int16_t r = _reg_read_2(SBUS_PCMCIA3_TIMR_REG16);

	_reg_write_2(SBUS_PCMCIA3_TIMR_REG16, 1);
	_reg_write_2(SBUS_PCMCIA3_TIMR_REG16, r);
}
