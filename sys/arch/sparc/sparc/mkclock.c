/*	$NetBSD: mkclock.c,v 1.11 2004/03/17 17:04:59 pk Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mkclock.c,v 1.11 2004/03/17 17:04:59 pk Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/promlib.h>
#include <machine/cpu.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

/* Location and size of the MK48xx TOD clock, if present */
static bus_space_handle_t	mk_nvram_base;
static bus_size_t		mk_nvram_size;

static int	mk_clk_wenable(todr_chip_handle_t, int);
static int	mk_nvram_wenable(int);

static int	clockmatch_mainbus (struct device *, struct cfdata *, void *);
static int	clockmatch_obio(struct device *, struct cfdata *, void *);
static void	clockattach_mainbus(struct device *, struct device *, void *);
static void	clockattach_obio(struct device *, struct device *, void *);

static void	clockattach(struct mk48txx_softc *, int);

CFATTACH_DECL(clock_mainbus, sizeof(struct mk48txx_softc),
    clockmatch_mainbus, clockattach_mainbus, NULL, NULL);

CFATTACH_DECL(clock_obio, sizeof(struct mk48txx_softc),
    clockmatch_obio, clockattach_obio, NULL, NULL);

/* Imported from clock.c: */
extern todr_chip_handle_t todr_handle;
extern int (*eeprom_nvram_wenable)(int);


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
	struct mk48txx_softc *sc = (void *)self;
	struct mainbus_attach_args *ma = aux;

	sc->sc_bst = ma->ma_bustag;

	/*
	 * We ignore any existing virtual address as we need to map
	 * this read-only and make it read-write only temporarily,
	 * whenever we read or write the clock chip.  The clock also
	 * contains the ID ``PROM'', and I have already had the pleasure
	 * of reloading the CPU type, Ethernet address, etc, by hand from
	 * the console FORTH interpreter.  I intend not to enjoy it again.
	 */
	if (bus_space_map(sc->sc_bst,
			   ma->ma_paddr,
			   ma->ma_size,
			   BUS_SPACE_MAP_LINEAR,
			   &sc->sc_bsh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}

	clockattach(sc, ma->ma_node);
}

static void
clockattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mk48txx_softc *sc = (void *)self;
	union obio_attach_args *uoba = aux;
	int node;

	if (uoba->uoba_isobio4 == 0) {
		/* sun4m clock at obio */
		struct sbus_attach_args *sa = &uoba->uoba_sbus;

		node = sa->sa_node;
		sc->sc_bst = sa->sa_bustag;
		if (sbus_bus_map(sc->sc_bst,
			sa->sa_slot, sa->sa_offset, sa->sa_size,
			BUS_SPACE_MAP_LINEAR, &sc->sc_bsh) != 0) {
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
		sc->sc_bst = oba->oba_bustag;
		if (bus_space_map(sc->sc_bst,
				  oba->oba_paddr,
				  2048,			/* size */
				  BUS_SPACE_MAP_LINEAR,	/* flags */
				  &sc->sc_bsh) != 0) {
			printf("%s: can't map register\n", self->dv_xname);
			return;
		}
	}

	clockattach(sc, node);
}

static void
clockattach(sc, node)
	struct mk48txx_softc *sc;
	int node;
{

	if (CPU_ISSUN4)
		sc->sc_model = "mk48t02";	/* Hard-coded sun4 clock */
	else if (node != 0)
		sc->sc_model = prom_getpropstring(node, "model");
	else
		panic("clockattach: node == 0");

	/* Our TOD clock year 0 represents 1968 */
	sc->sc_year0 = 1968;
	mk48txx_attach(sc);

	/* XXX this should be done by todr_attach() */
	todr_handle = &sc->sc_handle;

	printf("\n");

	/*
	 * Store NVRAM base address and size in globals for use
	 * by mk_nvram_wenable().
	 */
	mk_nvram_base = sc->sc_bsh;
	if (mk48txx_get_nvram_size(todr_handle, &mk_nvram_size) != 0)
		panic("Cannot get nvram size on %s", sc->sc_model);

	/* Establish clock write-enable method */
	todr_handle->todr_setwen = mk_clk_wenable;

#if defined(SUN4)
	if (CPU_ISSUN4) {
		if (cpuinfo.cpu_type == CPUTYP_4_300 ||
		    cpuinfo.cpu_type == CPUTYP_4_400) {
			eeprom_va = bus_space_vaddr(sc->sc_bst, sc->sc_bsh);
			eeprom_nvram_wenable = mk_nvram_wenable;
		}
	}
#endif
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
	vaddr_t base;
	vsize_t size;
	static int writers;

	s = splhigh();
	if (onoff)
		prot = writers++ == 0 ? VM_PROT_READ|VM_PROT_WRITE : 0;
	else
		prot = --writers == 0 ? VM_PROT_READ : 0;
	splx(s);

	size = round_page((vsize_t)mk_nvram_size);
	base = trunc_page((vaddr_t)mk_nvram_base);
	if (prot)
		pmap_kprotect(base, size, prot);

	return (0);
}
