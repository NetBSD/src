/*	$NetBSD: pic.c,v 1.7 2004/04/11 00:44:47 pooka Exp $	 */

/*
 * Copyright (c) 2002 Steve Rumble
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic.c,v 1.7 2004/04/11 00:44:47 pooka Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/picreg.h>

#include "locators.h"

struct pic_softc {
	struct device   sc_dev;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;

};

static int      pic_match(struct device *, struct cfdata *, void *);
static void     pic_attach(struct device *, struct device *, void *);
static int      pic_print(void *, const char *);
void		pic_bus_reset(void);
void		pic_watchdog_enable(void);
void		pic_watchdog_disable(void);
void		pic_watchdog_tickle(void);

CFATTACH_DECL(pic, sizeof(struct pic_softc),
	      pic_match, pic_attach, NULL, NULL);

struct pic_attach_args {
	const char     *iaa_name;

	bus_space_tag_t iaa_st;
	bus_space_handle_t iaa_sh;
};

static struct pic_softc psc;

static int
pic_match(struct device * parent, struct cfdata * match, void *aux)
{
	/*
	 * PIC exists on IP12 systems. It appears to be the immediate
	 * ancestor of the mc, for mips1 processors.
	 */
	if (mach_type == MACH_SGI_IP12)
		return (1);
	else
		return (0);
}

static void
pic_attach(struct device * parent, struct device * self, void *aux)
{
	u_int32_t       reg;
	char            picstr[80] = "";
	struct pic_attach_args iaa;
	struct mainbus_attach_args *ma = aux;

	psc.iot = SGIMIPS_BUS_SPACE_HPC;
	if (bus_space_map(psc.iot, ma->ma_addr, 0,
			  BUS_SPACE_MAP_LINEAR, &psc.ioh))
		panic("pic_attach: could not allocate memory\n");

	platform.bus_reset = pic_bus_reset;
	platform.watchdog_enable = pic_watchdog_enable;
	platform.watchdog_disable = pic_watchdog_disable;
	platform.watchdog_reset = pic_watchdog_tickle;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_SYSID);
	reg = (reg >> PIC_SYSID_REVSHIFT) & PIC_SYSID_REVMASK;
	printf("\npic0: Revision %c", reg + 64);

	/* enable refresh, set big-endian, memory parity, allow slave access */
	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg |= (PIC_CPUCTRL_REFRESH | PIC_CPUCTRL_BIGENDIAN | PIC_CPUCTRL_MPR |
		PIC_CPUCTRL_SLAVE);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);

	/* query the mode register to see what's going on */
	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_MODE);
	printf(": dblk (0x%x), iblk (0x%x)\n", reg & PIC_MODE_DBSIZ,
	       reg & PIC_MODE_IBSIZ);

	if (reg & PIC_MODE_NOCACHE)
		strcat(picstr, "cache disabled");
	else
		strcat(picstr, "cache enabled");

	if (reg & PIC_MODE_ISTREAM)
		strcat(picstr, ", instr streaming");

	if (reg & PIC_MODE_STOREPARTIAL)
		strcat(picstr, ", store partial");

	if (reg & PIC_MODE_BUSDRIVE)
		strcat(picstr, ", bus drive");

	printf("pic0: %s", picstr);

	/* gio32 allow master, real time devices */
	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_GIO32ARB_SLOT0);
	reg &= ~(PIC_GIO32ARB_SLOT_SLAVE | PIC_GIO32ARB_SLOT_LONG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_GIO32ARB_SLOT0, reg);

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_GIO32ARB_SLOT1);
	reg &= ~(PIC_GIO32ARB_SLOT_SLAVE | PIC_GIO32ARB_SLOT_LONG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_GIO32ARB_SLOT1, reg);

	/* default gio32 burst time */
	bus_space_write_4(psc.iot, psc.ioh, PIC_GIO32ARB_BURST,
			  PIC_GIO32ARB_DEFBURST);

	/* default gio32 delay time */
	bus_space_write_4(psc.iot, psc.ioh, PIC_GIO32ARB_DELAY,
			  PIC_GIO32ARB_DEFDELAY);

	printf("\n");

	/* XXX gio only on IP12 Indigo (?). does pic exist anywhere else? */
	iaa.iaa_name = "gio";
	(void) config_found(self, (void *) &iaa, pic_print);

	/* Enable watchdog, reset it */
	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL)
		| (PIC_CPUCTRL_WDOG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}


static int
pic_print(void *aux, const char *name)
{
	struct pic_attach_args *iaa = aux;

	if (name)
		aprint_normal("%s at %s", iaa->iaa_name, name);

	return UNCONF;
}

void
pic_bus_reset(void)
{
	bus_space_write_4(psc.iot, psc.ioh, PIC_PARITY_ERROR, 0);
}

void
pic_watchdog_enable()
{
	uint32_t reg;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg |= PIC_CPUCTRL_WDOG;
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}

void
pic_watchdog_disable()
{
	uint32_t reg;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg &= ~(PIC_CPUCTRL_WDOG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}

void
pic_watchdog_tickle()
{
	uint32_t reg;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg &= ~(PIC_CPUCTRL_WDOG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg |= (PIC_CPUCTRL_WDOG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}
