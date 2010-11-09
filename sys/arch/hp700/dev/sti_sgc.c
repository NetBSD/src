/*	$NetBSD: sti_sgc.c,v 1.20 2010/11/09 12:24:47 skrll Exp $	*/

/*	$OpenBSD: sti_sgc.c,v 1.38 2009/02/06 22:51:04 miod Exp $	*/

/*
 * Copyright (c) 2000-2003 Michael Shalayeff
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
 * These cards has to be known to work so far:
 *	- HPA1991AGrayscale rev 0.02	(705/35) (byte-wide)
 *	- HPA1991AC19       rev 0.02	(715/33) (byte-wide)
 *	- HPA208LC1280      rev 8.04	(712/80) just works
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.20 2010/11/09 12:24:47 skrll Exp $");

#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/hp700/machdep.h>

#ifdef STIDEBUG
#define	DPRINTF(s)	do {	\
	if (stidebug)		\
		printf s;	\
} while(0)

extern int stidebug;
#else
#define	DPRINTF(s)	/* */
#endif

#define	STI_ROMSIZE	(sizeof(struct sti_dd) * 4)
#define	STI_ID_FDDI	0x280b31af	/* Medusa FDDI ROM id */

/* gecko optional graphics */
#define	STI_GOPT1_REV	0x17
#define	STI_GOPT2_REV	0x70
#define	STI_GOPT3_REV	0xd0
#define	STI_GOPT4_REV	0x00
#define	STI_GOPT5_REV	0x20
#define	STI_GOPT6_REV	0x40
#define	STI_GOPT7_REV	0x30

const char sti_sgc_opt[] = {
	STI_GOPT1_REV,
	STI_GOPT2_REV,
	STI_GOPT3_REV,
	STI_GOPT4_REV,
	STI_GOPT5_REV,
	STI_GOPT6_REV,
	STI_GOPT7_REV
};

int sti_sgc_probe(device_t, cfdata_t, void *);
void sti_sgc_attach(device_t, device_t, void *);

void sti_sgc_end_attach(device_t);

extern struct cfdriver sti_cd;

CFATTACH_DECL_NEW(sti_gedoens, sizeof(struct sti_softc), sti_sgc_probe,
    sti_sgc_attach, NULL, NULL);

paddr_t sti_sgc_getrom(struct confargs *);

/*
 * Locate STI ROM.
 * On some machines it may not be part of the HPA space.
 */
paddr_t
sti_sgc_getrom(struct confargs *ca)
{
	paddr_t rom;
	int pagezero_cookie;

	pagezero_cookie = hp700_pagezero_map();
	rom = PAGE0->pd_resv2[1];
	hp700_pagezero_unmap(pagezero_cookie);

	if (ca->ca_type.iodc_sv_model == HPPA_FIO_GSGC) {
		int i;
		for (i = sizeof(sti_sgc_opt); i--; )
			if (sti_sgc_opt[i] == ca->ca_type.iodc_revision)
				break;
		if (i < 0)
			rom = 0;
	}
	
	if (rom < HPPA_IOBEGIN) {
		if (ca->ca_naddrs > 0)
			rom = ca->ca_addrs[0].addr;
		else
			rom = ca->ca_hpa;
	}

	return rom;
}

int
sti_sgc_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	bus_space_handle_t romh;
	paddr_t rom;
	uint32_t id, romend;
	u_char devtype;
	int rv = 0, romunmapped = 0;

	/* due to the graphic nature of this program do probe only one */
	if (cf->cf_unit > sti_cd.cd_ndevs)
		return 0;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO)
		return 0;

	/* these need further checking for the graphics id */
	if (ca->ca_type.iodc_sv_model != HPPA_FIO_GSGC &&
	    ca->ca_type.iodc_sv_model != HPPA_FIO_SGC)
		return 0;

	rom = sti_sgc_getrom(ca);
	DPRINTF(("%s: hpa=%x, rom=%x\n", __func__, (uint)ca->ca_hpa,
	    (uint)rom));

	/* if it does not map, probably part of the lasi space */
	if ((rv = bus_space_map(ca->ca_iot, rom, STI_ROMSIZE, 0, &romh))) {
		DPRINTF(("%s: can't map rom space (%d)\n", __func__, rv));

		if ((rom & HPPA_IOBEGIN) == HPPA_IOBEGIN) {
			romh = rom;
			romunmapped++;
		} else {
			/* in this case nobody has no freaking idea */
			return 0;
		}
	}

	devtype = bus_space_read_1(ca->ca_iot, romh, 3);

	DPRINTF(("%s: devtype=%d\n", __func__, devtype));
	rv = 1;
	switch (devtype) {
	case STI_DEVTYPE4:
		id = bus_space_read_4(ca->ca_iot, romh, STI_DEV4_DD_GRID);
		break;
	case STI_DEVTYPE1:
		id = (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    +  3) << 24) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    +  7) << 16) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    + 11) <<  8) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_GRID
		    + 15));
		break;
	default:
		DPRINTF(("%s: unknown type (%x)\n", __func__, devtype));
		rv = 0;
		romend = 0;
	}

	if (rv &&
	    ca->ca_type.iodc_sv_model == HPPA_FIO_SGC && id == STI_ID_FDDI) {
		DPRINTF(("%s: not a graphics device\n", __func__));
		rv = 0;
	}

	if (ca->ca_naddrs >= sizeof(ca->ca_addrs) / sizeof(ca->ca_addrs[0])) {
		printf("sti: address list overflow\n");
		return 0;
	}

	ca->ca_addrs[ca->ca_naddrs].addr = rom;
	ca->ca_addrs[ca->ca_naddrs].size = sti_rom_size(ca->ca_iot, romh);
	ca->ca_naddrs++;

	if (!romunmapped)
		bus_space_unmap(ca->ca_iot, romh, STI_ROMSIZE);
	return rv;
}

void
sti_sgc_attach(device_t parent, device_t self, void *aux)
{
	struct sti_softc *sc = device_private(self);
	struct confargs *ca = aux;
	bus_space_handle_t romh;
	hppa_hpa_t consaddr;
	int pagezero_cookie;
	paddr_t rom;
	uint32_t romlen;
	int rv;
	int i;

	pagezero_cookie = hp700_pagezero_map();
	consaddr = (hppa_hpa_t)PAGE0->mem_cons.pz_hpa;
	hp700_pagezero_unmap(pagezero_cookie);
	
	sc->sc_dev = self;
	sc->sc_enable_rom = NULL;
	sc->sc_disable_rom = NULL;

	/* we stashed rom addr/len into the last slot during probe */
	rom = ca->ca_addrs[ca->ca_naddrs - 1].addr;
	romlen = ca->ca_addrs[ca->ca_naddrs - 1].size;

	if ((rv = bus_space_map(ca->ca_iot, rom, romlen, 0, &romh))) {
		if ((rom & HPPA_IOBEGIN) == HPPA_IOBEGIN)
			romh = rom;
		else {
			aprint_error(": can't map rom space (%d)\n", rv);
			return;
		}
	}

	sc->bases[0] = romh;
	for (i = 1; i < STI_REGION_MAX; i++)
		sc->bases[i] = ca->ca_hpa;

#ifdef HP7300LC_CPU
	/*
	 * PCXL2: enable accel I/O for this space, see PCX-L2 ERS "ACCEL_IO".
	 * "pcxl2_ers.{ps,pdf}", (section / chapter . rel. page / abs. page)
	 * 8.7.4 / 8-12 / 92, 11.3.14 / 11-14 / 122 and 14.8 / 14-5 / 203.
	 */
	if (hppa_cpu_info->hci_cputype == hpcxl2
	    && ca->ca_hpa >= PCXL2_ACCEL_IO_START
	    && ca->ca_hpa <= PCXL2_ACCEL_IO_END)
		eaio_l2(PCXL2_ACCEL_IO_ADDR2MASK(ca->ca_hpa));
#endif /* HP7300LC_CPU */

	if (ca->ca_hpa == consaddr)
		sc->sc_flags |= STI_CONSOLE;
	if (sti_attach_common(sc, ca->ca_iot, ca->ca_iot, romh,
	   STI_CODEBASE_PA) == 0)
		config_interrupts(self, sti_sgc_end_attach);
}

void
sti_sgc_end_attach(device_t dev)
{
	struct sti_softc *sc = device_private(dev);
	
	sti_end_attach(sc);
}
