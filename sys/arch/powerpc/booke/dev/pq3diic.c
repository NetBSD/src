/*	$NetBSD: pq3diic.c,v 1.1.4.1 2011/06/06 09:06:26 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/tty.h>

#include "ioconf.h"

#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/i2c/motoi2creg.h>
#include <dev/i2c/motoi2cvar.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

struct pq3diic_softc {
	device_t sc_dev;
	void *sc_ih;
	struct motoi2c_softc sc_motoi2c[2];
};

static int pq3diic_match(device_t, cfdata_t, void *);
static void pq3diic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pq3diic, sizeof(struct pq3diic_softc),
    pq3diic_match, pq3diic_attach, NULL, NULL);

static int
pq3diic_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
		return 0;

	return 1;
}

static int
pq3diic_intr(void *arg)
{
	struct pq3diic_softc * const sc = arg;
	int rv = 0;

	rv += motoi2c_intr(&sc->sc_motoi2c[0]);
	rv += motoi2c_intr(&sc->sc_motoi2c[1]);

	return rv;
}

static void
pq3diic_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3diic_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	u_int nports = cnl->cnl_size / I2C_SIZE;
	int error;

	psc->sc_children |= cna->cna_childmask;
	sc->sc_dev = self;

	aprint_normal(": %u port%s\n", nports, nports == 1 ? "" : "s");

	for (u_int port = 0; port <= min(1, nports); port++) {
		struct motoi2c_softc * const msc = &sc->sc_motoi2c[port];
		msc->sc_iot = cna->cna_memt;
		error = bus_space_map(msc->sc_iot,
		    cnl->cnl_addr + port * I2C_SIZE,
		    I2C_SIZE, 0, &msc->sc_ioh);
		if (error) {
			aprint_error_dev(self,
			    "can't map registers for i2c#%uL %d\n",
			    port, error);
		} else {
			motoi2c_attach_common(self, msc, NULL);
		}
	}

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM, IST_ONCHIP,
	    pq3diic_intr, sc);
	if (sc->sc_ih == NULL)
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     cnl->cnl_intrs[0]);
	else
		aprint_normal_dev(self, "interrupting on irq %d\n",
		     cnl->cnl_intrs[0]);
}
