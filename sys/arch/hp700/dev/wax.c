/*	$NetBSD: wax.c,v 1.9.4.2 2009/05/16 10:41:12 yamt Exp $	*/

/*	$OpenBSD: wax.c,v 1.1 1998/11/23 03:04:10 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: wax.c,v 1.9.4.2 2009/05/16 10:41:12 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>

#include <hp700/gsc/gscbusvar.h>

#define	WAX_IOMASK	0xfff00000
#define	WAX_REGS	0xc000

struct wax_regs {
	u_int32_t wax_irr;	/* int requset register */
	u_int32_t wax_imr;	/* int mask register */
	u_int32_t wax_ipr;	/* int pending register */
	u_int32_t wax_icr;	/* int control register */
	u_int32_t wax_iar;	/* int address register */
};

struct wax_softc {
	device_t sc_dv;
	struct hp700_int_reg sc_int_reg;
	struct wax_regs volatile *sc_regs;
};

int	waxmatch(device_t, cfdata_t, void *);
void	waxattach(device_t, device_t, void *);


CFATTACH_DECL_NEW(wax, sizeof(struct wax_softc),
    waxmatch, waxattach, NULL, NULL);

static int wax_attached;

/*
 * Before a module is matched, this fixes up its gsc_attach_args.
 */
static void wax_fix_args(void *, struct gsc_attach_args *);
static void
wax_fix_args(void *_sc, struct gsc_attach_args *ga)
{
	struct wax_softc *sc = _sc;
	hppa_hpa_t module_offset;

	/*
	 * Determine this module's interrupt bit.
	 */
	module_offset = ga->ga_hpa - (hppa_hpa_t) sc->sc_regs;
	ga->ga_irq = HP700CF_IRQ_UNDEF;
	if (module_offset == 0x1000)	/* hil */
		ga->ga_irq = 1;
	if (module_offset == 0x2000)	/* com */
		ga->ga_irq = 6;
}

int
waxmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* there will be only one */
	if (wax_attached ||
	    ca->ca_type.iodc_type != HPPA_TYPE_BHA ||
	    ca->ca_type.iodc_sv_model != HPPA_BHA_WAX)
		return 0;

	/* Make sure we have an IRQ. */
	if (ca->ca_irq == HP700CF_IRQ_UNDEF) {
		ca->ca_irq = hp700_intr_allocate_bit(&int_reg_cpu);
	}

	return 1;
}

void
waxattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct wax_softc *sc = device_private(self);
	struct gsc_attach_args ga;
	bus_space_handle_t ioh;
	int s, in;

	if (ca->ca_irq == HP700CF_IRQ_UNDEF) {
		aprint_error(": can't allocate IRQ\n");
		return;
	}

	sc->sc_dv = self;
	wax_attached = 1;

	aprint_normal("\n");

	/*
	 * Map the WAX interrupt registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa, sizeof(struct wax_regs),
	    0, &ioh))
		panic("waxattach: can't map interrupt registers");
	sc->sc_regs = (struct wax_regs *)ca->ca_hpa;

	/* interrupts guts */
	s = splhigh();
	sc->sc_regs->wax_iar = cpu_gethpa(0) | (31 - ca->ca_irq);
	sc->sc_regs->wax_icr = 0;
	sc->sc_regs->wax_imr = ~0U;
	in = sc->sc_regs->wax_irr;
	sc->sc_regs->wax_imr = 0;
	splx(s);

	/* Establish the interrupt register. */
	hp700_intr_reg_establish(&sc->sc_int_reg);
	sc->sc_int_reg.int_reg_mask = &sc->sc_regs->wax_imr;
	sc->sc_int_reg.int_reg_req = &sc->sc_regs->wax_irr;

	/* Attach the GSC bus. */
	ga.ga_ca = *ca;	/* clone from us */
	if (strcmp(parent->dv_xname, "mainbus0") == 0) {
		ga.ga_dp.dp_bc[0] = ga.ga_dp.dp_bc[1];
		ga.ga_dp.dp_bc[1] = ga.ga_dp.dp_bc[2];
		ga.ga_dp.dp_bc[2] = ga.ga_dp.dp_bc[3];
		ga.ga_dp.dp_bc[3] = ga.ga_dp.dp_bc[4];
		ga.ga_dp.dp_bc[4] = ga.ga_dp.dp_bc[5];
		ga.ga_dp.dp_bc[5] = ga.ga_dp.dp_mod;
		ga.ga_dp.dp_mod = 0;
	}

	ga.ga_name = "gsc";
	ga.ga_int_reg = &sc->sc_int_reg;
	ga.ga_fix_args = wax_fix_args;
	ga.ga_fix_args_cookie = sc;
	ga.ga_scsi_target = 7; /* XXX */
	config_found(self, &ga, gscprint);
}
