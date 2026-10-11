/*	$NetBSD: isp1763hc_rr.c,v 1.1 2026/10/11 17:31:04 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Individual Computers RapidRoad USB, on clockport.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isp1763hc_rr.c,v 1.1 2026/10/11 17:31:04 rkujawa Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/ic/isp1763reg.h>
#include <dev/ic/isp1763var.h>

#include <amiga/clockport/clockportvar.h>
#include <amiga/clockport/isp1763hc_rrreg.h>

struct isp1763hc_rr_softc {
	struct isp1763hc_softc	sc_isp;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_ih;
	void			(*sc_intr_disestablish)(void *);
	int			sc_bank;
	bool			sc_attached;
};

#define	RR_SOFTC(sc)	((struct isp1763hc_rr_softc *)(sc))

static int	isp1763hc_rr_match(device_t, cfdata_t, void *);
static void	isp1763hc_rr_attach(device_t, device_t, void *);
static int	isp1763hc_rr_detach(device_t, int);
static bool	isp1763hc_rr_identify(bus_space_tag_t, bus_space_handle_t);
static bool	isp1763hc_rr_shutdown(device_t, int);
static int	isp1763hc_rr_intr(void *);

static void	isp1763hc_rr_bank(struct isp1763hc_rr_softc *, bus_size_t);
static uint8_t	isp1763hc_rr_rd1(struct isp1763hc_softc *, bus_size_t);
static void	isp1763hc_rr_wr1(struct isp1763hc_softc *, bus_size_t,
		    uint8_t);
static uint16_t	isp1763hc_rr_rd2(struct isp1763hc_softc *, bus_size_t);
static void	isp1763hc_rr_wr2(struct isp1763hc_softc *, bus_size_t,
		    uint16_t);
static void	isp1763hc_rr_memrd(struct isp1763hc_softc *, uint16_t, void *,
		    size_t);
static void	isp1763hc_rr_memwr(struct isp1763hc_softc *, uint16_t,
		    const void *, size_t);
static void	isp1763hc_rr_intr_enable(struct isp1763hc_softc *, bool);

CFATTACH_DECL3_NEW(isp1763hc_rr, sizeof(struct isp1763hc_rr_softc),
    isp1763hc_rr_match, isp1763hc_rr_attach, isp1763hc_rr_detach,
    isp1763hc_activate, NULL, isp1763hc_childdet, DVF_DETACH_SHUTDOWN);

static bool
isp1763hc_rr_identify(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	uint32_t id;
	int i;

	(void)bus_space_read_1(iot, ioh, RR_WINDOW);
	delay(10000);

	if ((bus_space_read_1(iot, ioh, RR_BANKA) & RR_BANKA_ID_MASK) !=
	    RR_BANKA_ID)
		return false;

	/* Select bank 0x0e, which holds the id. */
	(void)bus_space_read_1(iot, ioh, RR_BANKB);

	id = 0;
	for (i = 0; i < 4; i++)
		id |= (uint32_t)bus_space_read_1(iot, ioh, RR_WINDOW + i) <<
		    (8 * i);

	return id == ISP1763_CHIPID_VALUE;
}

static int
isp1763hc_rr_match(device_t parent, cfdata_t cf, void *aux)
{
	struct clockport_attach_args *cpa = aux;
	bus_space_handle_t ioh;
	bool found;

	if (bus_space_map(cpa->cp_iot, 0, RR_NREGS, 0, &ioh) != 0)
		return 0;

	if ((cf->cf_flags & RR_FLAGS_FORCE) != 0 &&
	    (bus_space_read_1(cpa->cp_iot, ioh, RR_STATUS) &
	    RR_STATUS_GATE) != 0)
		bus_space_write_1(cpa->cp_iot, ioh, RR_CTRL, 0);

	found = isp1763hc_rr_identify(cpa->cp_iot, ioh);
	bus_space_unmap(cpa->cp_iot, ioh, RR_NREGS);

	return found ? 1 : 0;
}

static void
isp1763hc_rr_attach(device_t parent, device_t self, void *aux)
{
	struct isp1763hc_rr_softc *rsc = device_private(self);
	struct isp1763hc_softc *sc = &rsc->sc_isp;
	struct clockport_attach_args *cpa = aux;

	aprint_naive("\n");
	aprint_normal(": RapidRoad USB\n");

	rsc->sc_iot = cpa->cp_iot;
	if (bus_space_map(rsc->sc_iot, 0, RR_NREGS, 0, &rsc->sc_ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_CTRL, 0);
	rsc->sc_bank = -1;

	sc->sc_dev = self;
	sc->sc_hwmode = ISP1763_HWMODE_BUS8;
	sc->sc_rd1 = isp1763hc_rr_rd1;
	sc->sc_wr1 = isp1763hc_rr_wr1;
	sc->sc_rd2 = isp1763hc_rr_rd2;
	sc->sc_wr2 = isp1763hc_rr_wr2;
	sc->sc_memrd = isp1763hc_rr_memrd;
	sc->sc_memwr = isp1763hc_rr_memwr;
	sc->sc_intr_enable = isp1763hc_rr_intr_enable;

	rsc->sc_intr_disestablish = cpa->cp_intr_disestablish;
	rsc->sc_ih = cpa->cp_intr_establish(isp1763hc_rr_intr, rsc);

	if (isp1763hc_attach(sc) != 0) {
		rsc->sc_intr_disestablish(rsc->sc_ih);
		bus_space_unmap(rsc->sc_iot, rsc->sc_ioh, RR_NREGS);
		return;
	}
	rsc->sc_attached = true;

	if (!pmf_device_register1(self, NULL, NULL, isp1763hc_rr_shutdown))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
isp1763hc_rr_detach(device_t self, int flags)
{
	struct isp1763hc_rr_softc *rsc = device_private(self);
	int error;

	if (!rsc->sc_attached)
		return 0;

	error = isp1763hc_detach(&rsc->sc_isp, flags);
	if (error != 0)
		return error;

	pmf_device_deregister(self);
	rsc->sc_intr_disestablish(rsc->sc_ih);
	bus_space_unmap(rsc->sc_iot, rsc->sc_ioh, RR_NREGS);
	rsc->sc_attached = false;

	return 0;
}

static bool
isp1763hc_rr_shutdown(device_t self, int howto)
{
	struct isp1763hc_rr_softc *rsc = device_private(self);

	isp1763hc_shutdown(&rsc->sc_isp);

	return true;
}

static int
isp1763hc_rr_intr(void *arg)
{
	struct isp1763hc_rr_softc *rsc = arg;
	uint8_t status;

	status = bus_space_read_1(rsc->sc_iot, rsc->sc_ioh, RR_STATUS);
	if ((status & (RR_STATUS_GATE | RR_STATUS_INT)) !=
	    (RR_STATUS_GATE | RR_STATUS_INT))
		return 0;

	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_CTRL, 0);
	isp1763hc_intr(&rsc->sc_isp);

	return 1;
}

static void
isp1763hc_rr_bank(struct isp1763hc_rr_softc *rsc, bus_size_t reg)
{

	if (rsc->sc_bank == RR_BANK(reg))
		return;

	rsc->sc_bank = RR_BANK(reg);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_BANKA, rsc->sc_bank);
}

static uint8_t
isp1763hc_rr_rd1(struct isp1763hc_softc *sc, bus_size_t reg)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	isp1763hc_rr_bank(rsc, reg);

	return bus_space_read_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg));
}

static void
isp1763hc_rr_wr1(struct isp1763hc_softc *sc, bus_size_t reg, uint8_t val)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	isp1763hc_rr_bank(rsc, reg);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg), val);
}

static uint16_t
isp1763hc_rr_rd2(struct isp1763hc_softc *sc, bus_size_t reg)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);
	uint16_t hi;

	isp1763hc_rr_bank(rsc, reg);
	hi = bus_space_read_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg) + 1);

	return (hi << 8) |
	    bus_space_read_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg));
}

static void
isp1763hc_rr_wr2(struct isp1763hc_softc *sc, bus_size_t reg, uint16_t val)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	isp1763hc_rr_bank(rsc, reg);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg), val & 0xff);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_SLOT(reg) + 1,
	    val >> 8);
}

static void
isp1763hc_rr_memrd(struct isp1763hc_softc *sc, uint16_t addr, void *buf,
    size_t len)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_MEMLO, addr & 0xff);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_MEMHI, addr >> 8);
	bus_space_read_multi_1(rsc->sc_iot, rsc->sc_ioh, RR_DATA, buf, len);
}

static void
isp1763hc_rr_memwr(struct isp1763hc_softc *sc, uint16_t addr,
    const void *buf, size_t len)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_MEMLO, addr & 0xff);
	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_MEMHI, addr >> 8);
	bus_space_write_multi_1(rsc->sc_iot, rsc->sc_ioh, RR_DATA, buf, len);
}

static void
isp1763hc_rr_intr_enable(struct isp1763hc_softc *sc, bool on)
{
	struct isp1763hc_rr_softc *rsc = RR_SOFTC(sc);

	bus_space_write_1(rsc->sc_iot, rsc->sc_ioh, RR_CTRL,
	    on ? RR_CTRL_GATE : 0);
}
