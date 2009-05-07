/*	$NetBSD: sti_sgc.c,v 1.14 2009/05/07 15:17:22 skrll Exp $	*/

/*	$OpenBSD: sti_sgc.c,v 1.21 2003/12/22 23:39:06 mickey Exp $	*/

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
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * These cards has to be known to work so far:
 *	- HPA1991AGrayscale rev 0.02	(705/35) (byte-wide)
 *	- HPA1991AC19       rev 0.02	(715/33) (byte-wide)
 *	- HPA208LC1280      rev 8.04	(712/80) just works
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sti_sgc.c,v 1.14 2009/05/07 15:17:22 skrll Exp $");

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

/* internal EG */
#define	STI_INEG_REV	0x60
#define	STI_INEG_PROM	0xf0011000

int sti_sgc_probe(struct device *, struct cfdata *, void *);
void sti_sgc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sti_gedoens, sizeof(struct sti_softc), sti_sgc_probe, sti_sgc_attach,
    NULL, NULL);

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

	rom = ca->ca_hpa;

	if (ca->ca_type.iodc_sv_model != HPPA_FIO_GSGC) {
		return rom;
	}

	switch (ca->ca_type.iodc_revision) {
	case STI_GOPT1_REV:
	case STI_GOPT2_REV:
	case STI_GOPT3_REV:
	case STI_GOPT4_REV:
	case STI_GOPT5_REV:
	case STI_GOPT6_REV:
	case STI_GOPT7_REV:
		/* these share the onboard's prom */
		pagezero_cookie = hp700_pagezero_map();
		rom = PAGE0->pd_resv2[1];
		hp700_pagezero_unmap(pagezero_cookie);
		break;

	case STI_INEG_REV:
		rom = STI_INEG_PROM;
		break;
	}
	return rom;
}

int
sti_sgc_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;
	bus_space_handle_t romh;
	paddr_t rom;
	u_int32_t id, romend;
	u_char devtype;
	int rv = 0, romunmapped = 0;

	if (ca->ca_type.iodc_type != HPPA_TYPE_FIO)
		return (0);

	/* these need futher checking for the graphics id */
	if (ca->ca_type.iodc_sv_model != HPPA_FIO_GSGC &&
	    ca->ca_type.iodc_sv_model != HPPA_FIO_SGC)
		return 0;

	rom = sti_sgc_getrom(ca);
#ifdef STIDEBUG
	printf ("sti: hpa=%x, rom=%x\n", (uint)ca->ca_hpa, (uint)rom);
#endif

	/* if it does not map, probably part of the lasi space */
	if ((rv = bus_space_map(ca->ca_iot, rom, STI_ROMSIZE, 0, &romh))) {
#ifdef STIDEBUG
		printf ("sti: can't map rom space (%d)\n", rv);
#endif
		if ((rom & HPPA_IOBEGIN) == HPPA_IOBEGIN) {
			romh = rom;
			romunmapped++;
		} else {
			/* in this case nobody has no freaking idea */
			return 0;
		}
	}

	devtype = bus_space_read_1(ca->ca_iot, romh, 3);

#ifdef STIDEBUG
	printf("sti: devtype=%d\n", devtype);
#endif
	rv = 1;
	switch (devtype) {
	case STI_DEVTYPE4:
		id = bus_space_read_4(ca->ca_iot, romh, STI_DEV4_DD_GRID);
		romend = bus_space_read_4(ca->ca_iot, romh, STI_DEV4_DD_ROMEND);
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
		romend = (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_ROMEND 
		    +  3) << 24) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_ROMEND 
		    +  7) << 16) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_ROMEND 
		    + 11) <<  8) |
		    (bus_space_read_1(ca->ca_iot, romh, STI_DEV1_DD_ROMEND 
		    + 15));
		break;
	default:
#ifdef STIDEBUG
		printf("sti: unknown type (%x)\n", devtype);
#endif
		rv = 0;
		romend = 0;
	}

	if (rv &&
	    ca->ca_type.iodc_sv_model == HPPA_FIO_SGC && id == STI_ID_FDDI) {
#ifdef STIDEBUG
		printf("sti: not a graphics device\n");
#endif
		rv = 0;
	}

	if (ca->ca_naddrs >= sizeof(ca->ca_addrs) / sizeof(ca->ca_addrs[0])) {
		printf("sti: address list overflow\n");
		return (0);
	}

	ca->ca_addrs[ca->ca_naddrs].addr = rom;
	ca->ca_addrs[ca->ca_naddrs].size = round_page(romend);
	ca->ca_naddrs++;

	if (!romunmapped)
		bus_space_unmap(ca->ca_iot, romh, STI_ROMSIZE);
	return (rv);
}

void
sti_sgc_attach(struct device *parent, struct device *self, void *aux)
{
	struct sti_softc *sc = (void *)self;
	struct confargs *ca = aux;
	paddr_t rom;
	u_int32_t romlen;
	int rv;
	int pagezero_cookie;

	pagezero_cookie = hp700_pagezero_map();
	sc->memt = sc->iot = ca->ca_iot;
	sc->base = ca->ca_hpa;

	/* we stashed rom addr/len into the last slot during probe */
	rom = ca->ca_addrs[ca->ca_naddrs - 1].addr;
	romlen = ca->ca_addrs[ca->ca_naddrs - 1].size;
	if ((rv = bus_space_map(ca->ca_iot, rom, romlen, 0, &sc->romh))) {
		if ((rom & HPPA_IOBEGIN) == HPPA_IOBEGIN)
			sc->romh = rom;
		else {
			printf (": can't map rom space (%d)\n", rv);
			return;
		}
	}

#ifdef HP7300LC_CPU
	/*
	 * PCXL2: enable accel I/O for this space, see PCX-L2 ERS "ACCEL_IO".
	 * "pcxl2_ers.{ps,pdf}", (section / chapter . rel. page / abs. page)
	 * 8.7.4 / 8-12 / 92, 11.3.14 / 11-14 / 122 and 14.8 / 14-5 / 203.
	 */
	if (strcmp(hppa_cpu_info->hppa_cpu_info_chip_type, "PCX-L2") == 0
	    && ca->ca_hpa >= PCXL2_ACCEL_IO_START
	    && ca->ca_hpa <= PCXL2_ACCEL_IO_END)
		eaio_l2(PCXL2_ACCEL_IO_ADDR2MASK(ca->ca_hpa));
#endif /* HP7300LC_CPU */

	sc->sc_devtype = bus_space_read_1(sc->iot, sc->romh, 3);
	if (ca->ca_hpa == (hppa_hpa_t)PAGE0->mem_cons.pz_hpa)
		sc->sc_flags |= STI_CONSOLE;
	hp700_pagezero_unmap(pagezero_cookie);
	sti_attach_common(sc);
}
