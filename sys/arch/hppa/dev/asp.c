/*	$NetBSD: asp.c,v 1.1 2014/02/24 07:23:42 skrll Exp $	*/

/*	$OpenBSD: asp.c,v 1.5 2000/02/09 05:04:22 mickey Exp $	*/

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

/*
 * References:
 *
 * 1. Cobra/Coral I/O Subsystem External Reference Specification
 *    Hewlett-Packard
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: asp.c,v 1.1 2014/02/24 07:23:42 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <sys/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <machine/cpufunc.h>

#include <hppa/hppa/machdep.h>
#include <hppa/dev/cpudevs.h>
#include <hppa/dev/viper.h>

#include <hppa/gsc/gscbusvar.h>

struct asp_hwr {
	uint8_t asp_reset;
	uint8_t asp_resv[31];
	uint8_t asp_version;
	uint8_t asp_resv1[15];
	uint8_t asp_scsidsync;
	uint8_t asp_resv2[15];
	uint8_t asp_error;
};

struct asp_trs {
	uint32_t asp_irr;
	uint32_t asp_imr;
	uint32_t asp_ipr;
	uint32_t asp_icr;
	uint32_t asp_iar;
	uint32_t asp_resv[3];
	uint8_t  asp_cled;
	uint8_t  asp_resv1[3];
	struct {
		u_int		:20,
			asp_spu	: 3,	/* SPU ID board jumper */
#define	ASP_SPUCOBRA	0
#define	ASP_SPUCORAL	1
#define	ASP_SPUBUSH	2
#define	ASP_SPUHARDBALL	3
#define	ASP_SPUSCORPIO	4
#define	ASP_SPUCORAL2	5
			asp_sw	: 1,	/* front switch is normal */
			asp_clk : 1,	/* SCSI clock is doubled */
			asp_lan : 2,	/* LAN iface selector */
#define	ASP_LANINVAL	0
#define	ASP_LANAUI	1
#define	ASP_LANTHIN	2
#define	ASP_LANMISS	3
			asp_lanf: 1,	/* LAN AUI fuse is ok */
			asp_spwr: 1,	/* SCSI power ok */
			asp_scsi: 3;	/* SCSI ctrl ID */
	} _asp_ios;
#define	asp_spu		_asp_ios.asp_spu
#define	asp_sw		_asp_ios.asp_sw
#define	asp_clk		_asp_ios.asp_clk
#define	asp_lan		_asp_ios.asp_lan
#define	asp_lanf	_asp_ios.asp_lanf
#define	asp_spwr	_asp_ios.asp_spwr
#define	asp_scsi	_asp_ios.asp_scsi
};

#define	ASP_BANK_SZ	0x02000000
#define	ASP_REG_INT	0x00800000
#define	ASP_ETHER_ADDR	0x00810000
#define	ASP_REG_MISC	0x0082f000

const struct asp_spus_tag {
	char	name[12];
	int	ledword;
} asp_spus[] = {
	{ "Cobra", 0 },
	{ "Coral", 0 },
	{ "Bushmaster", 0 },
	{ "Hardball", 1 },
	{ "Scorpio", 0 },
	{ "Coral II", 1 },
	{ "#6", 0 },
	{ "#7", 0 }
};

struct asp_softc {
	device_t sc_dev;

	struct hppa_interrupt_register sc_ir;

	volatile struct asp_hwr *sc_hw;
	volatile struct asp_trs *sc_trs;
};

int	aspmatch(device_t, cfdata_t, void *);
void	aspattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(asp, sizeof(struct asp_softc),
    aspmatch, aspattach, NULL, NULL);

/*
 * Before a module is matched, this fixes up its gsc_attach_args.
 */
static void asp_fix_args(void *, struct gsc_attach_args *);
static void
asp_fix_args(void *_sc, struct gsc_attach_args *ga)
{
	hppa_hpa_t module_offset;
	struct asp_softc *sc = _sc;

	/*
	 * Determine this module's interrupt bit.
	 */
	module_offset = ga->ga_hpa - (hppa_hpa_t) sc->sc_trs;
	ga->ga_irq = HPPACF_IRQ_UNDEF;
#define ASP_IRQ(off, irq) if (module_offset == off) ga->ga_irq = irq
	ASP_IRQ(0x22000, 6);	/* com1 */
	ASP_IRQ(0x23000, 5);	/* com0 */
	ASP_IRQ(0x24000, 7);	/* lpt */
	ASP_IRQ(0x25000, 9);	/* osiop */
	ASP_IRQ(0x26000, 8);	/* ie */
	ASP_IRQ(0x30000, 3);	/* siop */
	ASP_IRQ(0x800000, 13);	/* harmony */
#undef ASP_IRQ
}

int
aspmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_BHA ||
	    ca->ca_type.iodc_sv_model != HPPA_BHA_ASP)
		return 0;

	/*
	 * Forcibly mask the HPA down to the start of the ASP
	 * chip address space.
	 */
	ca->ca_hpa &= ~(ASP_BANK_SZ - 1);

	return 1;
}

void
aspattach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct asp_softc *sc = device_private(self);
	struct gsc_attach_args ga;
	struct cpu_info *ci = &cpus[0];
	bus_space_handle_t ioh;
	int s;

	sc->sc_dev = self;

	ca->ca_irq = hppa_intr_allocate_bit(&ci->ci_ir, ca->ca_irq);
	if (ca->ca_irq == HPPACF_IRQ_UNDEF) {
		aprint_error_dev(self, ": can't allocate interrupt");
		return;
	}

	/*
	 * Map the ASP interrupt registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + ASP_REG_INT,
	    sizeof(struct asp_trs), 0, &ioh)) {
		aprint_error(": can't map interrupt registers.\n");
		return;
	}
	sc->sc_trs = (struct asp_trs *)ioh;

	/*
	 * Map the ASP miscellaneous registers.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + ASP_REG_MISC,
			  sizeof(struct asp_hwr), 0, &ioh)) {
		aprint_error(": can't map miscellaneous registers.\n");
		return;
	}
	sc->sc_hw = (struct asp_hwr *)ioh;

	/*
	 * Map the Ethernet address and read it out.
	 */
	if (bus_space_map(ca->ca_iot, ca->ca_hpa + ASP_ETHER_ADDR,
			  sizeof(ga.ga_ether_address), 0, &ioh)) {
		aprint_error(": can't map EEPROM.\n");
		return;
	}
	bus_space_read_region_1(ca->ca_iot, ioh, 0,
		ga.ga_ether_address, sizeof(ga.ga_ether_address));
	bus_space_unmap(ca->ca_iot, ioh, sizeof(ga.ga_ether_address));

#ifdef USELEDS
	machine_ledaddr = &sc->sc_trs->asp_cled;
	machine_ledword = asp_spus[sc->sc_trs->asp_spu].ledword;
#endif

	/* reset ASP */
	/* sc->sc_hw->asp_reset = 1; */
	/* delay(400000); */

	s = splhigh();
	viper_setintrwnd(1 << ca->ca_irq);

	sc->sc_trs->asp_imr = ~0;
	(void)sc->sc_trs->asp_irr;
	sc->sc_trs->asp_imr = 0;

	/* Establish the interrupt register. */
	hppa_interrupt_register_establish(ci, &sc->sc_ir);
	sc->sc_ir.ir_name = device_xname(self);
	sc->sc_ir.ir_mask = &sc->sc_trs->asp_imr;
	sc->sc_ir.ir_req = &sc->sc_trs->asp_irr;

	splx(s);

	aprint_normal(": %s rev %d, lan %d scsi %d\n",
	    asp_spus[sc->sc_trs->asp_spu].name, sc->sc_hw->asp_version,
	    sc->sc_trs->asp_lan, sc->sc_trs->asp_scsi);

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
	ga.ga_fix_args = asp_fix_args;
	ga.ga_fix_args_cookie = sc;
	ga.ga_scsi_target = sc->sc_trs->asp_scsi;
	config_found(self, &ga, gscprint);
}
