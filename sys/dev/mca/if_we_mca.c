/*	$NetBSD: if_we_mca.c,v 1.3.2.2 2001/04/09 01:56:48 nathanw Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and Jaromir Dolecek.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 */

/*
 * Device driver for the Western Digital/SMC 8003 and 8013 series,
 * and the SMC Elite Ultra (8216).
 *
 * Currently only tested with WD8003W/A. Other WD8003-based cards
 * should work without problems, the SMC Elite based ones hopefully too.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <dev/mca/mcareg.h>
#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/wereg.h>
#include <dev/ic/wevar.h>

#define WD_8003		0x01
#define WD_ELITE	0x02
#define WD_MEMSIZE	16384	/* all supported cards have 16K Bytes memory */

int we_mca_probe __P((struct device *, struct cfdata *, void *));
void we_mca_attach __P((struct device *, struct device *, void *));
void we_mca_init_hook __P((struct we_softc *));

struct cfattach we_mca_ca = {
	sizeof(struct we_softc), we_mca_probe, we_mca_attach
};

/*
 * The types for some cards may not be correct; hopefully it's close
 * enough.
 */
static const struct we_mca_product {
	u_int32_t we_id;
	const char *we_name;
	int we_flag;
	int we_type;
	const char *we_typestr;
} we_mca_products[] = {
	{ MCA_PRODUCT_WD_8013EP, "EtherCard PLUS Elite/A (8013EP/A)",
		WD_ELITE,	WE_TYPE_WD8013EP, "WD8013EP/A" },
	{ MCA_PRODUCT_WD_8013WP, "EtherCard PLUS Elite 10T/A (8013WP/A)",
		WD_ELITE,	WE_TYPE_WD8013EP, "WD8013WP/A" }, /* XXX */
	{ MCA_PRODUCT_IBM_WD_2,"IBM PS/2 Adapter/A for Ethernet Networks (UTP)",
		WD_ELITE, WE_TYPE_WD8013EP, "WD8013WP/A"}, /* XXX */
	{ MCA_PRODUCT_IBM_WD_T,"IBM PS/2 Adapter/A for Ethernet Networks (BNC)",
		WD_ELITE, WE_TYPE_WD8013EP, "WD8013WP/A"}, /* XXX */
	{ MCA_PRODUCT_WD_8003E,	"WD EtherCard PLUS/A (WD8003E/A or WD8003ET/A)",
		WD_8003,	WE_TYPE_WD8003E, "WD8003E/A or WD8003ET/A" },
	{ MCA_PRODUCT_WD_8003ST,"WD StarCard PLUS/A (WD8003ST/A)",
		WD_8003,	WE_TYPE_WD8003W, "WD8003ST/A" }, /* XXX */
	{ MCA_PRODUCT_WD_8003W,	"WD EtherCard PLUS 10T/A (WD8003W/A)",
		WD_8003,	WE_TYPE_WD8003W, "WD8003W/A" },
	{ MCA_PRODUCT_IBM_WD_O, "IBM PS/2 Adapter/A for Ethernet Networks",
		WD_8003,	WE_TYPE_WD8003W, "IBM PS/2 Adapter/A" },/*XXX*/
	{ 0x0000,			NULL },
};

/* see POS description in we_mca_attach() */
static const int we_mca_irq[] = {
	3, 4, 10, 15,
};
	
/* memory position and shared RAM sizes for WD8013-type of cards */
static const struct {
	u_int8_t id;
	int maddr;
	int memsize;
} we_mca_elite_mem[] = {
	{ 0x10,	0x0C0000, 16384 },
	{ 0x12,	0x0C4000, 16384 },
	{ 0x14,	0x0C8000, 16384 },
	{ 0x16,	0x0CC000, 16384 },
	{ 0x18,	0x0D0000, 16384 },
	{ 0x1A,	0x0D4000, 16384 },
	{ 0x1C,	0x0D8000, 16384 },
	{ 0x1E,	0x0DC000, 16384 },
	{ 0x90,	0xFC0000, 16384 },
	{ 0x94,	0xFC8000, 16384 },
	{ 0x98,	0xFD0000, 16384 },
	{ 0x9C,	0x0C0000, 16384 },
	{ 0x00,	0x0C0000, 8192 },
	{ 0x01,	0x0C2000, 8192 },
	{ 0x10,	0x0C4000, 8192 },
	{ 0x11,	0x0C6000, 8192 },
	{ 0x00, 0x000000, 0    },
};

	
static const struct we_mca_product *we_mca_lookup __P((int));

static const struct we_mca_product *
we_mca_lookup(id)
	int id;
{
	const struct we_mca_product *wep;

	for(wep = we_mca_products; wep->we_name; wep++)
		if (wep->we_id == id)
			return (wep);

	return (NULL);
}

int
we_mca_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mca_attach_args *ma = aux;

	return (we_mca_lookup(ma->ma_id) != NULL);
}

void
we_mca_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct we_softc *wsc = (struct we_softc *)self;
	struct dp8390_softc *sc = &wsc->sc_dp8390;
	struct mca_attach_args *ma = aux;
	const struct we_mca_product *wep;
	bus_space_tag_t nict, asict, memt;
	bus_space_handle_t nich, asich, memh;
	const char *typestr;
	int irq, iobase, maddr;
	int pos2, pos3, pos5;

	pos2 = mca_conf_read(ma->ma_mc, ma->ma_slot, 2);
	pos3 = mca_conf_read(ma->ma_mc, ma->ma_slot, 3);
	pos5 = mca_conf_read(ma->ma_mc, ma->ma_slot, 5);

	/*
	 * POS registers differ much between 8003 and 8013, so they are
	 * divided to two sections.
	 * 
	 * 8003: POS register 2: (adf pos0)
	 * 7 6 5 4 3 2 1 0
	 * 0 0 1  \___/  \__ enable: 0=adapter disabled, 1=adapter enabled
	 *            \_____ Adapter IO Space: 0x200-0x29F + XX*0x20
	 *
	 * 8003: POS register 3: (adf pos1)
	 * 7 6 5 4 3 2 1 0
	 * 1 1 0  \__/ 1 X
	 *           \______ Shared Ram Space (16K Bytes):
	 *                     0xC0000-0xC3FFF + XX * 0x4000
	 *
	 * 8003: POS register 5: (adf pos3)
	 * 7 6 5 4 3 2 1 0
	 *             \_/
	 *               \__ Interrupt Level: 00=3 01=4 10=10 11=15
	 *
	 * -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	 *
	 * 8013: POS register 2: (adf pos0)
	 * 7 6 5 4 3 2 1 0
	 * \____/        \__ enable: 0=adapter disabled, 1=adapter enabled
	 *      \___________ Adapter I/O Space: 0x0800-0x081F + XX * 0x1000
	 *
	 * 8013: POS register 3: (adf pos1)
	 * 7 6 5 4 3 2 1 0
	 *   X 0             Shared Ram Base Address - mask 0x9f
	 *                     see wd_elite_mem[] array for details
	 *
	 * 8013: POS register 5: (adf pos3)
	 * 7 6 5 4 3 2 1 0
	 *         \_/ \_/
	 *           \   \__ Media Select: 00=TwPr (no link) 10=BNC
	 *            \          01=AUI|AUI or 10BaseT
	 *             \____ Interrupt Level: 00=3 01=4 10=10 11=14
	 */

	wep = we_mca_lookup(ma->ma_id);
#ifdef DIAGNOSTIC
	if (wep == NULL) {
		printf("\n%s: where did the card go?\n", sc->sc_dev.dv_xname);
		return;
	}
#endif

	if (wep->we_flag & WD_8003) {
		/* WD8003 based */
		iobase = 0x200 + (((pos2 & 0x0e) >> 1) * 0x020);
		maddr  = 0xC0000 + (((pos3 & 0x1c) >> 2) * 0x04000);
		irq = we_mca_irq[(pos5 & 0x03)];
		sc->mem_size = WD_MEMSIZE;
	} else {
		/* SMC Elite */
		int i, id;

		iobase = 0x800 + (((pos2 & 0xf0) >> 4) * 0x1000);
		irq = we_mca_irq[(pos5 & 0x06) >> 2];

		/* find location of shared mem and it's size */
		id = (pos3 & 0x9f);
		for(i=0; we_mca_elite_mem[i].maddr; i++)
			if (we_mca_elite_mem[i].id == id)
				break;
		if (we_mca_elite_mem[i].maddr == 0) {
			printf("\n%s: cannot find Shared Ram Base Address\n",
				sc->sc_dev.dv_xname);
			return;
		}
				
		maddr = we_mca_elite_mem[i].maddr;
		sc->mem_size = we_mca_elite_mem[i].memsize;
	}

	nict = asict = ma->ma_iot;
	memt = ma->ma_memt;

	printf(" slot %d: %s\n", ma->ma_slot + 1, wep->we_name);

	/* Map the device. */
	if (bus_space_map(asict, iobase, WE_NPORTS, 0, &asich)) {
		printf("%s: can't map nic i/o space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_subregion(asict, asich, WE_NIC_OFFSET, WE_NIC_NPORTS,
	    &nich)) {
		printf("%s: can't subregion i/o space\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	wsc->sc_type = wep->we_type;
	wsc->sc_16bitp = 1;		/* all cards we support are 16bit */
	sc->is790 = 0;
	typestr = wep->we_typestr;

	/*
	 * Map memory space.
	 */
	if (bus_space_map(memt, maddr, sc->mem_size, 0, &memh)) {
		printf("%s: can't map shared memory\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	wsc->sc_asict = asict;
	wsc->sc_asich = asich;

	sc->sc_regt = nict;
	sc->sc_regh = nich;

	sc->sc_buft = memt;
	sc->sc_bufh = memh;

	wsc->sc_maddr = maddr;

	/* Interface is always enabled. */
	sc->sc_enabled = 1;

	wsc->sc_init_hook = we_mca_init_hook;

	if (we_config(self, wsc, typestr))
		return;

	/* Map and establish the interrupt. */
	wsc->sc_ih = mca_intr_establish(ma->ma_mc, irq,
			    IPL_NET, dp8390_intr, sc);
	if (wsc->sc_ih == NULL) {
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	printf("%s: interrupting at irq %d\n", sc->sc_dev.dv_xname, irq);
}

void
we_mca_init_hook(wsc)
	struct we_softc *wsc;
{
	/*
	 * This quirk really needs to be here, at least for WD8003W/A. Without
	 * this, the card doesn't send any interrupts in 16bit mode. The quirk
	 * was taken from Linux smc-mca.c driver.
	 * I do not know why it's necessary. I don't want to know. It works
	 * and that is enough for me.
	 */
	bus_space_write_1(wsc->sc_asict, wsc->sc_asich, WE_LAAR, 0x04);
	wsc->sc_laar_proto |= 0x04;
}
