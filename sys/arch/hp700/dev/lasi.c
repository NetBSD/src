/*	$NetBSD: lasi.c,v 1.14 2009/05/24 06:53:34 skrll Exp $	*/

/*	$OpenBSD: lasi.c,v 1.4 2001/06/09 03:57:19 mickey Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
__KERNEL_RCSID(0, "$NetBSD: lasi.c,v 1.14 2009/05/24 06:53:34 skrll Exp $");

#undef LASIDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/dev/cpudevs.h>

#include <hp700/gsc/gscbusvar.h>

struct lasi_hwr {
	uint32_t lasi_power;
#define	LASI_BLINK	0x01
#define	LASI_OFF	0x02
	uint32_t lasi_error;
	uint32_t lasi_version;
	uint32_t lasi_reset;
	uint32_t lasi_arbmask;
};

struct lasi_trs {
	uint32_t lasi_irr;	/* int requset register */
	uint32_t lasi_imr;	/* int mask register */
	uint32_t lasi_ipr;	/* int pending register */
	uint32_t lasi_icr;	/* int command? register */
	uint32_t lasi_iar;	/* int acquire? register */
};

#define	LASI_BANK_SZ	0x200000
#define	LASI_REG_INT	0x100000
#define	LASI_REG_MISC	0x10c000

struct lasi_softc {
	device_t sc_dev;
	
	struct hp700_int_reg sc_int_reg;

	struct lasi_hwr volatile *sc_hw;
	struct lasi_trs volatile *sc_trs;
};

int	lasimatch(device_t, cfdata_t, void *);
void	lasiattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lasi, sizeof(struct lasi_softc),
    lasimatch, lasiattach, NULL, NULL);

extern struct cfdriver lasi_cd;

void	lasi_cold_hook(int);

/*
 * Before a module is matched, this fixes up its gsc_attach_args.
 */
static void lasi_fix_args(void *, struct gsc_attach_args *);
static void
lasi_fix_args(void *_sc, struct gsc_attach_args *ga)
{
	struct lasi_softc *sc = _sc;
	hppa_hpa_t module_offset;
	struct pdc_lan_station_id pdc_mac PDC_ALIGNMENT;

	/*
	 * Determine this module's interrupt bit.
	 */
	module_offset = ga->ga_hpa - (hppa_hpa_t) sc->sc_trs;
	ga->ga_irq = HP700CF_IRQ_UNDEF;
#define LASI_IRQ(off, irq) if (module_offset == off) ga->ga_irq = irq
	LASI_IRQ(0x2000, 7);	/* lpt */
	LASI_IRQ(0x4000, 13);	/* harmony */
	LASI_IRQ(0x4040, 16);	/* harmony telephone0 */
	LASI_IRQ(0x4060, 17);	/* harmony telephone1 */
	LASI_IRQ(0x5000, 5);	/* com */
	LASI_IRQ(0x6000, 9);	/* osiop */
	LASI_IRQ(0x7000, 8);	/* ie */
	LASI_IRQ(0x8000, 26);	/* pckbc */
	LASI_IRQ(0xa000, 20);	/* fdc */
#undef LASI_IRQ

	/*
	 * If this is the Ethernet adapter, get its Ethernet address.
	 */
	if (module_offset == 0x7000) {
		if (pdc_call((iodcio_t)pdc, 0, PDC_LAN_STATION_ID,
		     PDC_LAN_STATION_ID_READ, &pdc_mac, ga->ga_hpa) == 0)
			memcpy(ga->ga_ether_address, pdc_mac.addr,
				sizeof(ga->ga_ether_address));
	}
}

int
lasimatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_BHA ||
	    ca->ca_type.iodc_sv_model != HPPA_BHA_LASI)
		return 0;

	/* Make sure we have an IRQ. */
	if (ca->ca_irq == HP700CF_IRQ_UNDEF)
		ca->ca_irq = hp700_intr_allocate_bit(&int_reg_cpu);

	/*
	 * Forcibly mask the HPA down to the start of the LASI
	 * chip address space.
	 */
	ca->ca_hpa &= ~(LASI_BANK_SZ - 1);

	return 1;
}

void
lasiattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct lasi_softc *sc = device_private(self);
	struct gsc_attach_args ga;
	bus_space_handle_t ioh;
	int s, in;

	sc->sc_dev = self;
	/*
	 * Map the LASI interrupt registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + LASI_REG_INT,
			  sizeof(struct lasi_trs), 0, &ioh))
		panic("lasiattach: can't map interrupt registers");
	sc->sc_trs = (struct lasi_trs *)ioh;

	/*
	 * Map the LASI miscellaneous registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + LASI_REG_MISC,
			  sizeof(struct lasi_hwr), 0, &ioh))
		panic("lasiattach: can't map misc registers");
	sc->sc_hw = (struct lasi_hwr *)ioh;

	/* XXX should we reset the chip here? */

	aprint_normal(": rev %d.%d\n", (sc->sc_hw->lasi_version & 0xf0) >> 4,
		sc->sc_hw->lasi_version & 0xf);

	/* interrupts guts */
	s = splhigh();
	sc->sc_trs->lasi_iar = cpu_gethpa(0) | (31 - ca->ca_irq);
	sc->sc_trs->lasi_icr = 0;
	sc->sc_trs->lasi_imr = ~0U;
	in = sc->sc_trs->lasi_irr;
	sc->sc_trs->lasi_imr = 0;
	splx(s);

	/* Establish the interrupt register. */
	hp700_intr_reg_establish(&sc->sc_int_reg);
	sc->sc_int_reg.int_reg_mask = &sc->sc_trs->lasi_imr;
	sc->sc_int_reg.int_reg_req = &sc->sc_trs->lasi_irr;

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
	ga.ga_fix_args = lasi_fix_args;
	ga.ga_fix_args_cookie = sc;
	ga.ga_scsi_target = 7; /* XXX */
	config_found(self, &ga, gscprint);

	/* could be already set by power(4) */
	if (!cold_hook)
		cold_hook = lasi_cold_hook;
}

void
lasi_cold_hook(int on)
{
	struct lasi_softc *sc = device_private(lasi_cd.cd_devs[0]);

	if (!sc)
		return;

	switch (on) {
	case HPPA_COLD_COLD:
		sc->sc_hw->lasi_power = LASI_BLINK;
		break;
	case HPPA_COLD_HOT:
		sc->sc_hw->lasi_power = 0;
		break;
	case HPPA_COLD_OFF:
		sc->sc_hw->lasi_power = LASI_OFF;
		break;
	}
}
