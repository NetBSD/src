/*	$NetBSD: mkclock.c,v 1.1.8.2 2002/06/23 17:41:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * time-of-day clock driver for sparc machines with the MOSTEK MK48Txx.
 */
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/cpu.h>
#include <machine/idprom.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>

/* Location and size of the MK48xx TOD clock, if present */
static bus_space_handle_t	mk_nvram_base;
static bus_size_t		mk_nvram_size;

static int	mk_clk_wenable(todr_chip_handle_t, int);
static int	mk_nvram_wenable(int);

static int	clockmatch_mainbus (struct device *, struct cfdata *, void *);
static int	clockmatch_obio(struct device *, struct cfdata *, void *);
static void	clockattach_mainbus(struct device *, struct device *, void *);
static void	clockattach_obio(struct device *, struct device *, void *);

static void	clockattach(int, bus_space_tag_t, bus_space_handle_t);

struct cfattach clock_mainbus_ca = {
	sizeof(struct device), clockmatch_mainbus, clockattach_mainbus
};

struct cfattach clock_obio_ca = {
	sizeof(struct device), clockmatch_obio, clockattach_obio
};


/* Imported from clock.c: */
extern todr_chip_handle_t todr_handle;
extern int (*eeprom_nvram_wenable)(int);
void establish_hostid(struct idprom *);


/*
 * The OPENPROM calls the clock the "eeprom", so we have to have our
 * own special match function to call it the "clock".
 */
static int
clockmatch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp("eeprom", ma->ma_name) == 0);
}

static int
clockmatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (strcmp("eeprom", uoba->uoba_sbus.sa_name) == 0);

	if (!CPU_ISSUN4) {
		printf("clockmatch_obio: attach args mixed up\n");
		return (0);
	}

	/* Only these sun4s have "clock" (others have "oclock") */
	if (cpuinfo.cpu_type != CPUTYP_4_300 &&
	    cpuinfo.cpu_type != CPUTYP_4_400)
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

/* ARGSUSED */
static void
clockattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
	bus_space_tag_t bt = ma->ma_bustag;
	bus_space_handle_t bh;

	/*
	 * We ignore any existing virtual address as we need to map
	 * this read-only and make it read-write only temporarily,
	 * whenever we read or write the clock chip.  The clock also
	 * contains the ID ``PROM'', and I have already had the pleasure
	 * of reloading the cpu type, Ethernet address, etc, by hand from
	 * the console FORTH interpreter.  I intend not to enjoy it again.
	 */
	if (bus_space_map(bt,
			   ma->ma_paddr,
			   ma->ma_size,
			   BUS_SPACE_MAP_LINEAR,
			   &bh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	clockattach(ma->ma_node, bt, bh);
}

static void
clockattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int node;

	if (uoba->uoba_isobio4 == 0) {
		/* sun4m clock at obio */
		struct sbus_attach_args *sa = &uoba->uoba_sbus;

		node = sa->sa_node;
		bt = sa->sa_bustag;
		if (sbus_bus_map(bt,
			sa->sa_slot, sa->sa_offset, sa->sa_size,
			BUS_SPACE_MAP_LINEAR, &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
	} else {
		/* sun4 clock at obio */
		struct obio4_attach_args *oba = &uoba->uoba_oba4;

		/*
		 * Sun4's only have mk48t02 clocks, so we hard-code
		 * the device address space length to 2048.
		 */
		node = 0;
		bt = oba->oba_bustag;
		if (bus_space_map(bt,
				  oba->oba_paddr,
				  2048,			/* size */
				  BUS_SPACE_MAP_LINEAR,	/* flags */
				  &bh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
	}

	clockattach(node, bt, bh);
}

static void
clockattach(node, bt, bh)
	int node;
	bus_space_tag_t bt;
	bus_space_handle_t bh;
{
	struct idprom *idp;
	char *model;

	if (CPU_ISSUN4)
		model = "mk48t02";	/* Hard-coded sun4 clock */
	else if (node != 0)
		model = PROM_getpropstring(node, "model");
	else
		panic("clockattach: node == 0");

	/* Our TOD clock year 0 represents 1968 */
	todr_handle = mk48txx_attach(bt, bh, model, 1968, NULL, NULL);
	if (todr_handle == NULL)
		panic("Cannot attach %s tod clock", model);

	/*
	 * Store NVRAM base address and size in globals for use
	 * by mk_nvram_wenable().
	 */
	mk_nvram_base = bh;
	if (mk48txx_get_nvram_size(todr_handle, &mk_nvram_size) != 0)
		panic("Cannot get nvram size on %s", model);

	/* Establish clock write-enable method */
	todr_handle->todr_setwen = mk_clk_wenable;

#if defined(SUN4)
	if (CPU_ISSUN4) {
		extern struct idprom sun4_idprom_store;
		idp = &sun4_idprom_store;
		if (cpuinfo.cpu_type == CPUTYP_4_300 ||
		    cpuinfo.cpu_type == CPUTYP_4_400) {
			eeprom_va = bus_space_vaddr(bt, bh);
			eeprom_nvram_wenable = mk_nvram_wenable;
		}
	} else
#endif
	{
	/*
	 * Location of IDPROM relative to the end of the NVRAM area
	 */
#define MK48TXX_IDPROM_OFFSET (mk_nvram_size - 40)

		idp = (struct idprom *)((u_long)bh + MK48TXX_IDPROM_OFFSET);
	}

	establish_hostid(idp);
}

/*
 * Write en/dis-able TOD clock registers.  This is a safety net to
 * save idprom (part of mk48txx TOD clock) from being accidentally
 * stomped on by a buggy code.  We coordinate so that several writers
 * can run simultaneously.
 */
int
mk_clk_wenable(handle, onoff)
	todr_chip_handle_t handle;
	int onoff;
{
	/* XXX - we ignore `handle' here... */
	return (mk_nvram_wenable(onoff));
}

int
mk_nvram_wenable(onoff)
{
	int s;
	vm_prot_t prot;/* nonzero => change prot */
	int npages;
	vaddr_t base;
	static int writers;

	s = splhigh();
	if (onoff)
		prot = writers++ == 0 ? VM_PROT_READ|VM_PROT_WRITE : 0;
	else
		prot = --writers == 0 ? VM_PROT_READ : 0;
	splx(s);

	npages = round_page((vsize_t)mk_nvram_size) << PAGE_SHIFT;
	base = trunc_page((vaddr_t)mk_nvram_base);
	if (prot)
		pmap_changeprot(pmap_kernel(), base, prot, npages);

	return (0);
}
