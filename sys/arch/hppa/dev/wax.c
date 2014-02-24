/*	$NetBSD: wax.c,v 1.1 2014/02/24 07:23:43 skrll Exp $	*/

/*	$OpenBSD: wax.c,v 1.1 1998/11/23 03:04:10 mickey Exp $	*/

/*
 * Copyright (c) 1998-2003 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wax.c,v 1.1 2014/02/24 07:23:43 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hppa/dev/cpudevs.h>

#include <hppa/gsc/gscbusvar.h>

#define	WAX_IOMASK	0xfff00000
#define	WAX_REGS	0xc000

struct wax_regs {
	uint32_t wax_irr;	/* int requset register */
	uint32_t wax_imr;	/* int mask register */
	uint32_t wax_ipr;	/* int pending register */
	uint32_t wax_icr;	/* int control register */
	uint32_t wax_iar;	/* int address register */
};

struct wax_softc {
	device_t sc_dv;
	struct hppa_interrupt_register sc_ir;
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
	ga->ga_irq = HPPACF_IRQ_UNDEF;
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

	return 1;
}

void
waxattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct wax_softc *sc = device_private(self);
	struct gsc_attach_args ga;
	struct cpu_info *ci = &cpus[0];
	bus_space_handle_t ioh;
	int s;

	ca->ca_irq = hppa_intr_allocate_bit(&ci->ci_ir, ca->ca_irq);
	if (ca->ca_irq == HPPACF_IRQ_UNDEF) {
		aprint_error(": can't allocate interrupt\n");
		return;
	}

	sc->sc_dv = self;
	wax_attached = 1;

	aprint_normal("\n");

	/*
	 * Map the WAX interrupt registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa, sizeof(struct wax_regs),
	    0, &ioh)) {
		aprint_error(": can't map interrupt registers\n");
		return;
	}
	sc->sc_regs = (struct wax_regs *)ca->ca_hpa;

	/* interrupts guts */
	s = splhigh();
	sc->sc_regs->wax_iar = ci->ci_hpa | (31 - ca->ca_irq);
	sc->sc_regs->wax_icr = 0;
	sc->sc_regs->wax_imr = ~0U;
	(void)sc->sc_regs->wax_irr;
	sc->sc_regs->wax_imr = 0;
	splx(s);

	/* Establish the interrupt register. */
	hppa_interrupt_register_establish(ci, &sc->sc_ir);
	sc->sc_ir.ir_name = device_xname(self);
	sc->sc_ir.ir_mask = &sc->sc_regs->wax_imr;
	sc->sc_ir.ir_req = &sc->sc_regs->wax_irr;

	/* Attach the GSC bus. */
	ga.ga_ca = *ca;	/* clone from us */
	if (strcmp(device_xname(parent), "mainbus0") == 0) {
		ga.ga_dp.dp_bc[0] = ga.ga_dp.dp_bc[1];
		ga.ga_dp.dp_bc[1] = ga.ga_dp.dp_bc[2];
		ga.ga_dp.dp_bc[2] = ga.ga_dp.dp_bc[3];
		ga.ga_dp.dp_bc[3] = ga.ga_dp.dp_bc[4];
		ga.ga_dp.dp_bc[4] = ga.ga_dp.dp_bc[5];
		ga.ga_dp.dp_bc[5] = ga.ga_dp.dp_mod;
		ga.ga_dp.dp_mod = 0;
	}

	ga.ga_name = "gsc";
	ga.ga_ir = &sc->sc_ir;
	ga.ga_fix_args = wax_fix_args;
	ga.ga_fix_args_cookie = sc;
	ga.ga_scsi_target = 7; /* XXX */
	config_found(self, &ga, gscprint);
}
