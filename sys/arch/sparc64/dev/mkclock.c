/*	$NetBSD: mkclock.c,v 1.1.48.1 2008/01/10 23:44:05 bouyer Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mkclock.c,v 1.1.48.1 2008/01/10 23:44:05 bouyer Exp $");

/*    
 * Clock driver for 'mkclock' - Mostek MK48Txx TOD clock.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/cpu.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>
#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

/*
 * clock (eeprom) attaches at the sbus or the ebus (PCI)
 */
static int	mkclock_sbus_match(struct device *, struct cfdata *, void *);
static void	mkclock_sbus_attach(struct device *, struct device *, void *);

static int	mkclock_ebus_match(struct device *, struct cfdata *, void *);
static void	mkclock_ebus_attach(struct device *, struct device *, void *);

static void	mkclock_attach(struct mk48txx_softc *, int);

static int	mkclock_wenable(struct todr_chip_handle *, int);


CFATTACH_DECL(mkclock_sbus, sizeof(struct mk48txx_softc),
    mkclock_sbus_match, mkclock_sbus_attach, NULL, NULL);

CFATTACH_DECL(mkclock_ebus, sizeof(struct mk48txx_softc),
    mkclock_ebus_match, mkclock_ebus_attach, NULL, NULL);

/*
 * The OPENPROM calls the clock the "eeprom", so we have to have our
 * own special match function to call it the "clock".
 */
static int
mkclock_sbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("eeprom", sa->sa_name) == 0);
}

static int
mkclock_ebus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp("eeprom", ea->ea_name) == 0);
}

/*
 * Attach a clock (really `eeprom') to the sbus or ebus.
 *
 * We ignore any existing virtual address as we need to map
 * this read-only and make it read-write only temporarily,
 * whenever we read or write the clock chip.  The clock also
 * contains the ID ``PROM'', and I have already had the pleasure
 * of reloading the CPU type, Ethernet address, etc, by hand from
 * the console FORTH interpreter.  I intend not to enjoy it again.
 *
 * the MK48T02 is 2K.  the MK48T08 is 8K, and the MK48T59 is
 * supposed to be identical to it.
 *
 * This is *UGLY*!  We probably have multiple mappings.  But I do
 * know that this all fits inside an 8K page, so I'll just map in
 * once.
 *
 * What we really need is some way to record the bus attach args
 * so we can call *_bus_map() later with BUS_SPACE_MAP_READONLY
 * or not to write enable/disable the device registers.  This is
 * a non-trivial operation.  
 */

/* ARGSUSED */
static void
mkclock_sbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mk48txx_softc *sc = (void *)self;
	struct sbus_attach_args *sa = aux;
	int sz;

	sc->sc_bst = sa->sa_bustag;

	/* use sa->sa_regs[0].size? */
	sz = 8192;

	if (sbus_bus_map(sc->sc_bst,
			 sa->sa_slot,
			 (sa->sa_offset & ~(PAGE_SIZE - 1)),
			 sz,
			 BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_READONLY,
			 &sc->sc_bsh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}
	mkclock_attach(sc, sa->sa_node);
}


/* ARGSUSED */
static void
mkclock_ebus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mk48txx_softc *sc = (void *)self;
	struct ebus_attach_args *ea = aux;
	int sz;

	sc->sc_bst = ea->ea_bustag;

	/* hard code to 8K? */
	sz = ea->ea_reg[0].size;

	if (bus_space_map(sc->sc_bst,
			 EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			 sz,
			 BUS_SPACE_MAP_LINEAR,
			 &sc->sc_bsh) != 0) {
		printf("%s: can't map register\n", self->dv_xname);
		return;
	}
	mkclock_attach(sc, ea->ea_node);
}


static void
mkclock_attach(struct mk48txx_softc *sc, int node)
{

	sc->sc_model = prom_getpropstring(node, "model");

#ifdef DIAGNOSTIC
	if (sc->sc_model == NULL)
		panic("clockattach: no model property");
#endif

	/* Our TOD clock year 0 is 1968 */
	sc->sc_year0 = 1968;

	/* Save info for the clock wenable call. */
	sc->sc_handle.todr_setwen = mkclock_wenable;

	mk48txx_attach(sc);

	printf("\n");
}

/*
 * Write en/dis-able clock registers.  We coordinate so that several
 * writers can run simultaneously.
 */
static int
mkclock_wenable(struct todr_chip_handle *handle, int onoff)
{
	struct mk48txx_softc *sc;
	vm_prot_t prot;
	vaddr_t va;
	int s, err = 0;
	static int writers;

	s = splhigh();
	if (onoff)
		prot = writers++ == 0 ? VM_PROT_READ|VM_PROT_WRITE : 0;
	else
		prot = --writers == 0 ? VM_PROT_READ : 0;
	splx(s);
	if (prot == VM_PROT_NONE) {
		return 0;
	}
	sc = handle->cookie;
	va = (vaddr_t)bus_space_vaddr(sc->sc_bst, sc->sc_bsh);
	if (va == 0UL) {
		printf("clock_wenable: WARNING -- cannot get va\n");
		return EIO;
	}
	pmap_kprotect(va, prot);
	return (err);
}
