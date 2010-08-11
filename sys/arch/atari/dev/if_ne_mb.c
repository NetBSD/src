/*	$NetBSD: if_ne_mb.c,v 1.2.6.2 2010/08/11 22:51:45 yamt Exp $	*/

/*
 * Copyright (c) 2010 Izumi Tsutsui.  All rights reserved.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device driver for the EtherNEC,
 * NE2000 in 8bit mode over Atari ROM cartridge slot.
 * http://hardware.atari.org/ether/
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_mb.c,v 1.2.6.2 2010/08/11 22:51:45 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/cpu.h>
#include <machine/iomap.h>

#include <atari/atari/device.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

/*
 * EtherNEC specific address configuration
 */

/* I/O read ops are through /ROM4 area (0xFA0000) */
#define AD_CART_ROM4		(AD_CART + 0x00000)
#define ETHERNEC_READ_PORT	AD_CART_ROM4

/* I/O write ops are through /ROM3 area (0xFB0000) */
#define AD_CART_ROM3		(AD_CART + 0x10000)
#define ETHERNEC_WRITE_PORT	AD_CART_ROM3

/* CPU address lines A13-A9 are connected to ISA A4-A0 */
#define ETHERNEC_PORT_STRIDE	9

/* Using A8-A1 lines to specify write data (no A0 but UDS/LDS on m68k) */
#define ETHERNEC_WR_ADDR_SHIFT	1

/* interrupt polling per HZ */
#define ETHERNEC_TICK		1

static int  ne_mb_probe(device_t, cfdata_t, void *);
static void ne_mb_attach(device_t, device_t, void *);

static void ne_mb_poll(void *);

static bus_space_tag_t ethernec_init_bus_space_tag(bus_space_tag_t);
static void    ethernec_bus_space_unimpl(void);

static int     ethernec_bus_space_peek_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);
static uint8_t ethernec_bus_space_read_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);
static void    ethernec_bus_space_write_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, uint8_t);
static void    ethernec_bus_space_read_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint8_t *, bus_size_t);
static void    ethernec_bus_space_read_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, uint16_t *, bus_size_t);
static void    ethernec_bus_space_write_multi_1(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint8_t *, bus_size_t);
static void    ethernec_bus_space_write_multi_2(bus_space_tag_t,
    bus_space_handle_t, bus_size_t, const uint16_t *, bus_size_t);

struct ne_mb_softc {
	struct ne2000_softc sc_ne2000;		/* MI ne2000 softc */

	struct atari_bus_space sc_bs;
	struct callout sc_poll;
};

CFATTACH_DECL_NEW(ne_mb, sizeof(struct ne_mb_softc),
    ne_mb_probe, ne_mb_attach, NULL, NULL);

static int
ne_mb_probe(device_t parent, cfdata_t cf, void *aux)
{
	static bool ne_matched = false;
	struct atari_bus_space bs;
	bus_space_tag_t iot, asict;
	bus_space_handle_t ioh, iowh, asich;
	int netype, rv;

	rv = 0;

	if (!atari_realconfig)
		goto out;
	if (strcmp("ne", aux) || ne_matched)
		goto out;

	iot = ethernec_init_bus_space_tag(&bs);

	/* map I/O space for read ops */
	if (bus_space_map(iot, ETHERNEC_READ_PORT,
	    NE2000_NPORTS << ETHERNEC_PORT_STRIDE, 0, &ioh) != 0)
		goto out;

	/* map I/O space for write ops */
	if (bus_space_map(iot, ETHERNEC_WRITE_PORT,
	    NE2000_NPORTS << ETHERNEC_PORT_STRIDE, 0, &iowh) != 0)
		goto out1;

	/* XXX abuse stride for offset of write ports from read ones */
	iot->stride =
	    (vaddr_t)bus_space_vaddr(iot, iowh) -
	    (vaddr_t)bus_space_vaddr(iot, ioh);

	/* check if register regions are vaild */
	if (bus_space_peek_1(iot, ioh, 0) == 0)
		goto out2;

	asict = iot;
	if (bus_space_subregion(iot, ioh,
	    NE2000_ASIC_OFFSET << ETHERNEC_PORT_STRIDE,
	    NE2000_ASIC_NPORTS << ETHERNEC_PORT_STRIDE, &asich))
		goto out2;

	/* Look for an NE2000 compatible card */
	netype = ne2000_detect(iot, ioh, asict, asich);
	switch (netype) {
	/* XXX should we reject non RTL8019 variants? */
	case NE2000_TYPE_NE1000:
	case NE2000_TYPE_NE2000:
	case NE2000_TYPE_RTL8019:
		ne_matched = true;
		rv = 1;
		break;
	default:
		break;
	}

 out2:
	bus_space_unmap(iot, iowh, NE2000_NPORTS << ETHERNEC_PORT_STRIDE);
 out1:
	bus_space_unmap(iot, ioh, NE2000_NPORTS << ETHERNEC_PORT_STRIDE);
 out:
	return rv;
}

static void
ne_mb_attach(device_t parent, device_t self, void *aux)
{
	struct ne_mb_softc *sc = device_private(self);
	struct ne2000_softc *nsc = &sc->sc_ne2000;
	struct dp8390_softc *dsc = &nsc->sc_dp8390;
	bus_space_tag_t iot, asict;
	bus_space_handle_t ioh, iowh, asich;
	const char *typestr;
	int netype;

	dsc->sc_dev = self;
	aprint_normal(": EtherNEC on Atari ROM cartridge slot\n");

	iot = ethernec_init_bus_space_tag(&sc->sc_bs);

	/* map I/O space for read ops */
	if (bus_space_map(iot, ETHERNEC_READ_PORT,
	    NE2000_NPORTS << ETHERNEC_PORT_STRIDE, 0, &ioh) != 0)
		goto out;

	/* map I/O space for write ops */
	if (bus_space_map(iot, ETHERNEC_WRITE_PORT,
	    NE2000_NPORTS << ETHERNEC_PORT_STRIDE, 0, &iowh) != 0)
		goto out1;

	/* XXX abuse stride */
	iot->stride =
	    (vaddr_t)bus_space_vaddr(iot, iowh) -
	    (vaddr_t)bus_space_vaddr(iot, ioh);

	asict = iot;
	if (bus_space_subregion(iot, ioh,
	    NE2000_ASIC_OFFSET << ETHERNEC_PORT_STRIDE,
	    NE2000_ASIC_NPORTS << ETHERNEC_PORT_STRIDE, &asich))
		goto out2;

	dsc->sc_regt = iot;
	dsc->sc_regh = ioh;

	nsc->sc_asict = asict;
	nsc->sc_asich = asich;

	/* EtherNEC uses 8-bit data bus */
	nsc->sc_quirk = NE2000_QUIRK_8BIT;

	/*
	 * detect it again, so we can print some information about
	 * the interface.
	 * XXX: Should we accept only RTL8019?
	 */
	netype = ne2000_detect(iot, ioh, asict, asich);
	switch (netype) {
	case NE2000_TYPE_NE1000:
		typestr = "NE1000";
		break;

	case NE2000_TYPE_NE2000:
		typestr = "NE2000";
		break;

	case NE2000_TYPE_RTL8019:
		typestr = "NE2000 (RTL8019)";
		break;

	default:
		aprint_error_dev(self, "where did the card go?!\n");
		goto out2;
	}

	aprint_normal_dev(self, "%s Ethernet (8-bit)\n", typestr);

	/* this interface is always enabled */
	dsc->sc_enabled = 1;

	/* call MI ne2000 attach function */
	ne2000_attach(nsc, NULL);

	/* emulate interrupts by callout(9) */
	aprint_normal_dev(self, "using %d Hz polling\n", hz / ETHERNEC_TICK);
	callout_init(&sc->sc_poll, 0);
	callout_reset(&sc->sc_poll, ETHERNEC_TICK, ne_mb_poll, sc);

	return;

 out2:
	bus_space_unmap(iot, iowh, NE2000_NPORTS << ETHERNEC_PORT_STRIDE);
 out1:
	bus_space_unmap(iot, ioh, NE2000_NPORTS << ETHERNEC_PORT_STRIDE);
 out:
	return;
}

static void
ne_mb_poll(void *arg)
{
	struct ne_mb_softc *sc;
	struct ne2000_softc *nsc;
	struct dp8390_softc *dsc;
	int s;

	sc = arg;
	nsc = &sc->sc_ne2000;
	dsc = &nsc->sc_dp8390;

	s = splnet();
	(void)dp8390_intr(dsc);
	splx(s);

	callout_schedule(&sc->sc_poll, ETHERNEC_TICK);
}

/*
 * bus_space(9) functions for EtherNEC
 *
 *  XXX: should these belong to an independent cartridge slot bus?
 */
static bus_space_tag_t
ethernec_init_bus_space_tag(bus_space_tag_t en_t)
{

	if (en_t == NULL)
		return NULL;

	memset(en_t, 0, sizeof(*en_t));

	/* XXX: implement functions used by MI ne2000 and dp8390 only */
	en_t->abs_p_1   = ethernec_bus_space_peek_1;
	en_t->abs_p_2   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_p_4   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_p_8   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_r_1   = ethernec_bus_space_read_1;
	en_t->abs_r_2   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_r_4   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_r_8   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rs_1  = ethernec_bus_space_read_1;
	en_t->abs_rs_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rs_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rs_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rm_1  = ethernec_bus_space_read_multi_1;
	en_t->abs_rm_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rm_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rm_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rms_1 = ethernec_bus_space_read_multi_1;
	en_t->abs_rms_2 = ethernec_bus_space_read_multi_2;	/* XXX dummy */
	en_t->abs_rms_4 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rms_8 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rr_1  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rr_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rr_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rr_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rrs_1 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rrs_2 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rrs_4 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_rrs_8 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_w_1   = ethernec_bus_space_write_1;
	en_t->abs_w_2   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_w_4   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_w_8   = (void *)ethernec_bus_space_unimpl;
	en_t->abs_ws_1  = ethernec_bus_space_write_1;
	en_t->abs_ws_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_ws_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_ws_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wm_1  = ethernec_bus_space_write_multi_1;
	en_t->abs_wm_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wm_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wm_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wms_1 = ethernec_bus_space_write_multi_1;
	en_t->abs_wms_2 = ethernec_bus_space_write_multi_2;	/* XXX dummy */
	en_t->abs_wms_4 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wms_8 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wr_1  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wr_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wr_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wr_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wrs_1 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wrs_2 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wrs_4 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_wrs_8 = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sm_1  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sm_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sm_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sm_8  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sr_1  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sr_2  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sr_4  = (void *)ethernec_bus_space_unimpl;
	en_t->abs_sr_8  = (void *)ethernec_bus_space_unimpl;

	return en_t;
}

static void
ethernec_bus_space_unimpl(void)
{

	panic("%s: unimplemented EtherNEC bus_space(9) function called",
	    __func__);
}

static int
ethernec_bus_space_peek_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg)
{
	uint8_t *va;

	va = (uint8_t *)(bh + (reg << ETHERNEC_PORT_STRIDE));

	return !badbaddr(va, sizeof(uint8_t));
}

static uint8_t
ethernec_bus_space_read_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg)
{
	volatile uint8_t *ba;

	ba = (volatile uint8_t *)(bh + (reg << ETHERNEC_PORT_STRIDE));

	return *ba;
}

static void
ethernec_bus_space_write_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, uint8_t val)
{
	volatile uint8_t *wport;

	/*
	 * Write ops are done by read against write region (ROM3) address.
	 * 8-bit write data is specified via lower address bits.
	 */
	wport = (volatile uint8_t *)(bh +
	    bt->stride + (reg << ETHERNEC_PORT_STRIDE));
	wport += (u_int)val << ETHERNEC_WR_ADDR_SHIFT;

	(void)*wport;
}

static void
ethernec_bus_space_read_multi_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, uint8_t *a, bus_size_t c)
{
	volatile uint8_t *ba;

	ba = (volatile uint8_t *)(bh + (reg << ETHERNEC_PORT_STRIDE));
	for (; c != 0; c--)
		*a++ = *ba;
}

static void
ethernec_bus_space_read_multi_2(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, uint16_t *a, bus_size_t c)
{

	/* XXX: dummy function for probe ops in ne2000_detect() */
}

static void
ethernec_bus_space_write_multi_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, const uint8_t *a, bus_size_t c)
{
	volatile uint8_t *ba, *wport;
	u_int val;

	ba = (volatile uint8_t *)(bh +
	    bt->stride + (reg << ETHERNEC_PORT_STRIDE));

	for (; c != 0; c--) {
		val = *a++;
		wport = ba + (val << ETHERNEC_WR_ADDR_SHIFT);
		(void)*wport;
	}
}

static void
ethernec_bus_space_write_multi_2(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, const uint16_t *a, bus_size_t c)
{

	/* XXX: dummy function for probe ops in ne2000_detect() */
}
