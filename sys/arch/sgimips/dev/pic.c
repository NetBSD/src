/*	$NetBSD: pic.c,v 1.15.12.2 2017/12/03 11:36:41 jdolecek Exp $	 */

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
__KERNEL_RCSID(0, "$NetBSD: pic.c,v 1.15.12.2 2017/12/03 11:36:41 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/picreg.h>

#include <sgimips/gio/giovar.h>

#include "locators.h"

struct pic_softc {
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
};

static int      pic_match(device_t, cfdata_t, void *);
static void     pic_attach(device_t, device_t, void *);
static int      pic_print(void *, const char *);
static void	pic_bus_reset(void);
static void	pic_bus_error(vaddr_t, uint32_t, uint32_t);
static void	pic_watchdog_enable(void);
static void	pic_watchdog_disable(void);
static void	pic_watchdog_tickle(void);

CFATTACH_DECL_NEW(pic, 0,
    pic_match, pic_attach, NULL, NULL);

struct pic_attach_args {
	const char	       *iaa_name;

	bus_space_tag_t		iaa_st;
	bus_space_handle_t	iaa_sh;
};

int pic_gio32_arb_config(int, uint32_t);

static struct pic_softc psc;

static int
pic_match(device_t parent, cfdata_t match, void *aux)
{
	/*
	 * PIC exists on IP12 systems. It appears to be the immediate
	 * ancestor of the mc, for mips1 processors.
	 */
	if (mach_type == MACH_SGI_IP12)
		return 1;
	else
		return 0;
}

static void
pic_attach(device_t parent, device_t self, void *aux)
{
	uint32_t reg;
	struct pic_attach_args iaa;
	struct mainbus_attach_args *ma = aux;

	psc.iot = normal_memt;
	if (bus_space_map(psc.iot, ma->ma_addr, 0x20010,
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

	/* display the machine type, board revision */
	printf("pic0: ");

	switch (mach_subtype) {
	case MACH_SGI_IP12_4D_3X:
		printf("Personal Iris 4D/3x");
		break;
	case MACH_SGI_IP12_VIP12:
		printf("VME IP12");
		break;
	case MACH_SGI_IP12_HP1:
		printf("Indigo R3000");
		break;
	case MACH_SGI_IP12_HPLC:
		printf("Hollywood Light");
		break;
	default:
		printf("unknown machine");
		break;
	}
	printf(", board revision %x\n", mach_boardrev);

	printf("pic0: ");

	if (reg & PIC_MODE_NOCACHE)
		printf("cache disabled");
	else
		printf("cache enabled");

	if (reg & PIC_MODE_ISTREAM)
		printf(", instr streaming");

	if (reg & PIC_MODE_STOREPARTIAL)
		printf(", store partial");

	if (reg & PIC_MODE_BUSDRIVE)
		printf(", bus drive");

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

	platform.intr5 = pic_bus_error;

	/*
	 * A GIO bus exists on all IP12's. However, Personal Iris
	 * machines use VME for their expansion bus.
	 */
	iaa.iaa_name = "gio";
	(void)config_found(self, (void *)&iaa, pic_print);

	pic_watchdog_enable();
}


static int
pic_print(void *aux, const char *name)
{
	struct pic_attach_args *iaa = aux;

	if (name)
		aprint_normal("%s at %s", iaa->iaa_name, name);

	return UNCONF;
}

static void
pic_bus_reset(void)
{

	bus_space_write_4(psc.iot, psc.ioh, PIC_PARITY_ERROR, 0);
}

static void
pic_bus_error(vaddr_t pc, uint32_t status, uint32_t ipending)
{

	printf("pic0: bus error\n");
	pic_bus_reset();
}

static void
pic_watchdog_enable(void)
{
	uint32_t reg;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg |= PIC_CPUCTRL_WDOG;
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}

static void
pic_watchdog_disable(void)
{
	uint32_t reg;

	reg = bus_space_read_4(psc.iot, psc.ioh, PIC_CPUCTRL);
	reg &= ~(PIC_CPUCTRL_WDOG);
	bus_space_write_4(psc.iot, psc.ioh, PIC_CPUCTRL, reg);
}

static void
pic_watchdog_tickle(void)
{

	pic_watchdog_disable();
	pic_watchdog_enable();
}

/* intended to be called from gio/gio.c only */
int
pic_gio32_arb_config(int slot, uint32_t flags)
{
	uint32_t reg;

	/* only Indigo machines have GIO expansion slots (XXX HPLC?) */
	if (mach_subtype != MACH_SGI_IP12_HP1 &&
	    mach_subtype != MACH_SGI_IP12_HPLC)
		return EINVAL;

	/* graphics slot is not valid on IP12 */
	if (slot != GIO_SLOT_EXP0 && slot != GIO_SLOT_EXP1)
		return EINVAL;

	reg = bus_space_read_4(psc.iot, psc.ioh, (slot == GIO_SLOT_EXP0) ?
	    PIC_GIO32ARB_SLOT0 : PIC_GIO32ARB_SLOT1);

	if (flags & GIO_ARB_RT)
		reg &= ~PIC_GIO32ARB_SLOT_LONG;

	if (flags & GIO_ARB_LB)
		reg |= PIC_GIO32ARB_SLOT_LONG;

	if (flags & GIO_ARB_MST)
		reg &= ~PIC_GIO32ARB_SLOT_SLAVE;

	if (flags & GIO_ARB_SLV)
		reg |= PIC_GIO32ARB_SLOT_SLAVE;

	bus_space_write_4(psc.iot, psc.ioh, (slot == GIO_SLOT_EXP0) ?
	    PIC_GIO32ARB_SLOT0 : PIC_GIO32ARB_SLOT1, reg);

	return 0;
}
