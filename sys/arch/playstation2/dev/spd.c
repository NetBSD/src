/*	$NetBSD: spd.c,v 1.9.6.2 2014/08/20 00:03:17 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spd.c,v 1.9.6.2 2014/08/20 00:03:17 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bootinfo.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/dev/sbusvar.h>
#include <playstation2/dev/spdvar.h>
#include <playstation2/dev/spdreg.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC int spd_match(struct device *, struct cfdata *, void *);
STATIC void spd_attach(struct device *, struct device *, void *);
STATIC int spd_print(void *, const char *);
STATIC int spd_intr(void *);
STATIC void __spd_eeprom_out(u_int8_t *, int);
STATIC int __spd_eeprom_in(u_int8_t *);

/* SPD device can't attach twice. because PS2 PC-Card slot is only one. */
STATIC struct {
	int (*func)(void *);
	void *arg;
	const char *name;
} __spd_table[2];

CFATTACH_DECL(spd, sizeof(struct device),
    spd_match, spd_attach, NULL, NULL);

#ifdef DEBUG
#define LEGAL_SLOT(slot)	((slot) >= 0 && (slot) < 2)
#endif

int
spd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	
	return ((BOOTINFO_REF(BOOTINFO_DEVCONF) ==
	    BOOTINFO_DEVCONF_SPD_PRESENT));
}

void
spd_attach(struct device *parent, struct device *self, void *aux)
{
	struct spd_attach_args spa;

	printf(": PlayStation 2 HDD Unit\n");

	switch (BOOTINFO_REF(BOOTINFO_PCMCIA_TYPE)) {
	default:
		__spd_table[0].name = "<unknown product>";
		break;
	case 0:
		/* FALLTHROUGH */
	case 1:
		/* FALLTHROUGH */
	case 2:
		__spd_table[SPD_HDD].name = "SCPH-20400";
		__spd_table[SPD_NIC].name = "SCPH-10190";
		break;
	case 3:
		__spd_table[SPD_HDD].name = "SCPH-10260";
		__spd_table[SPD_NIC].name = "SCPH-10260";
		break;
	}

	/* disable all */
	_reg_write_2(SPD_INTR_ENABLE_REG16, 0);
	_reg_write_2(SPD_INTR_CLEAR_REG16, _reg_read_2(SPD_INTR_STATUS_REG16));

	spa.spa_slot = SPD_HDD;
	spa.spa_product_name = __spd_table[SPD_HDD].name;
	config_found(self, &spa, spd_print);

	spa.spa_slot = SPD_NIC;
	spa.spa_product_name = __spd_table[SPD_NIC].name;
	config_found(self, &spa, spd_print);

	sbus_intr_establish(SBUS_IRQ_PCMCIA, spd_intr, 0);
}

int
spd_print(void *aux, const char *pnp)
{
	struct spd_attach_args *spa = aux;

	if (pnp)
		aprint_normal("%s at %s", __spd_table[spa->spa_slot].name, pnp);

	return (UNCONF);
}

int
spd_intr(void *arg)
{
	u_int16_t r;
	
	r = _reg_read_2(SPD_INTR_STATUS_REG16);

 	/* HDD (SCPH-20400) */
	if ((r & SPD_INTR_HDD) != 0)
		if (__spd_table[SPD_HDD].func != NULL)
			(*__spd_table[SPD_HDD].func)(__spd_table[SPD_HDD].arg);

	/* Network (SCPH-10190) */
	if ((r & (SPD_INTR_EMAC3 | SPD_INTR_RXEND | SPD_INTR_TXEND |
	    SPD_INTR_RXDNV | SPD_INTR_TXDNV)) != 0)
		if (__spd_table[SPD_NIC].func)
			(*__spd_table[SPD_NIC].func)(__spd_table[SPD_NIC].arg);

	/* reinstall */
	r = _reg_read_2(SPD_INTR_ENABLE_REG16);
	_reg_write_2(SPD_INTR_ENABLE_REG16, 0);
	_reg_write_2(SPD_INTR_ENABLE_REG16, r);

	return (1);
}

void *
spd_intr_establish(enum spd_slot slot, int (*func)(void *), void *arg)
{

	KDASSERT(LEGAL_SLOT(slot));
	KDASSERT(__spd_table[slot].func == 0);

	__spd_table[slot].func = func;
	__spd_table[slot].arg = arg;

	return ((void *)slot);
}

void
spd_intr_disestablish(void *handle)
{
	int slot = (int)handle;

	KDASSERT(LEGAL_SLOT(slot));

	__spd_table[slot].func = 0;
}

/*
 * EEPROM access
 */
void
spd_eeprom_read(int addr, u_int16_t *data, int n)
{
	int i, j, s;
	u_int8_t r;

	s = _intr_suspend();

	/* set direction */
	_reg_write_1(SPD_IO_DIR_REG8, SPD_IO_CLK | SPD_IO_CS | SPD_IO_IN);

	/* chip select high */
	r = 0;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);
	r |= SPD_IO_CS;
	r &= ~(SPD_IO_IN | SPD_IO_CLK);
	_reg_write_1(SPD_IO_DATA_REG8, r);	
	delay(1);

	/* put start bit */
	__spd_eeprom_out(&r, 1);

	/* put op code (read) */
	__spd_eeprom_out(&r, 1);
	__spd_eeprom_out(&r, 0);

	/* set address */
	for (i = 0; i < 6; i++, addr <<= 1)
		__spd_eeprom_out(&r, addr & 0x20);
	
	/* get data */
	for (i = 0; i < n; i++, data++)
		for (*data = 0, j = 15; j >= 0; j--)
			*data |= (__spd_eeprom_in(&r) << j);

	/* chip select low */
	r &= ~(SPD_IO_CS | SPD_IO_IN | SPD_IO_CLK);
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(2);

	_intr_resume(s);
}

void
__spd_eeprom_out(u_int8_t *rp, int onoff)
{
	u_int8_t r = *rp;

	if (onoff)
		r |= SPD_IO_IN;
	else
		r &= ~SPD_IO_IN;

	r &= ~SPD_IO_CLK;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);
	
	r |= SPD_IO_CLK;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);

	r &= ~SPD_IO_CLK;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);

	*rp = r;
}

int
__spd_eeprom_in(u_int8_t *rp)
{
	int ret;
	u_int8_t r = *rp;

	r &= ~(SPD_IO_IN | SPD_IO_CLK);
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);

	r |= SPD_IO_CLK;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);
	ret = (_reg_read_1(SPD_IO_DATA_REG8) >> 4) & 0x1;

	r &= ~SPD_IO_CLK;
	_reg_write_1(SPD_IO_DATA_REG8, r);
	delay(1);

	*rp = r;

	return (ret);
}

