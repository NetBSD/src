/*	$NetBSD: if_sn_obio.c,v 1.25.32.1 2007/07/11 20:00:33 mjf Exp $	*/

/*
 * Copyright (C) 1997 Allen Briggs
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
 *      This product includes software developed by Allen Briggs
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
__KERNEL_RCSID(0, "$NetBSD: if_sn_obio.c,v 1.25.32.1 2007/07/11 20:00:33 mjf Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/viareg.h>

#include <dev/ic/dp83932reg.h>
#include <dev/ic/dp83932var.h>

#include <mac68k/obio/obiovar.h>
#include <mac68k/dev/if_snvar.h>

#define SONIC_REG_BASE	0x50F0A000
#define SONIC_PROM_BASE	0x50F08000
#define SONIC_SLOTNO	9

static int	sn_obio_match(struct device *, struct cfdata *, void *);
static void	sn_obio_attach(struct device *, struct device *, void *);
static int	sn_obio_getaddr(struct sonic_softc *, uint8_t *);
static int	sn_obio_getaddr_kludge(struct sonic_softc *, uint8_t *);

CFATTACH_DECL(sn_obio, sizeof(struct sonic_softc),
    sn_obio_match, sn_obio_attach, NULL, NULL);

static int
sn_obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_space_handle_t bsh;
	int found = 0;

	if (!mac68k_machine.sonic)
		return 0;

	if (bus_space_map(oa->oa_tag,
	    SONIC_REG_BASE, SONIC_NREGS * 4, 0, &bsh))
		return 0;

	if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0, 4))
		found = 1;

	bus_space_unmap(oa->oa_tag, bsh, SONIC_NREGS * 4);

	return found;
}

/*
 * Install interface into kernel networking data structures
 */
static void
sn_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	struct sonic_softc	*sc = (void *)self;
	uint8_t myaddr[ETHER_ADDR_LEN];
	int i;

	sc->sc_st = oa->oa_tag;
	sc->sc_dmat = oa->oa_dmat;

	if (bus_space_map(sc->sc_st,
	    SONIC_REG_BASE, SONIC_NREGS * 4, 0, &sc->sc_sh)) {
		printf(": failed to map space for SONIC regs.\n");
		return;
	}

	/* regs are addressed as words, big-endian. */
	for (i = 0; i < SONIC_NREGS; i++) {
		sc->sc_regmap[i] = (bus_size_t)((i * 4) + 2);
	}

	sc->sc_bigendian = 1;

	sc->sc_dcr = DCR_BMS | DCR_RFT1 | DCR_TFT0;
	sc->sc_dcr2 = 0;

	switch (current_mac_model->machineid) {
	case MACH_MACC610:
	case MACH_MACC650:
	case MACH_MACQ610:
	case MACH_MACQ650:
	case MACH_MACQ700:
	case MACH_MACQ800:
	case MACH_MACQ900:
	case MACH_MACQ950:
		sc->sc_dcr |= DCR_EXBUS;
		sc->sc_32bit = 1;
		break;

	case MACH_MACLC575:
	case MACH_MACP580:
	case MACH_MACQ630:
		/* Apple Comm Slot cards; assume they are 32 bit */
		sc->sc_dcr |= DCR_EXBUS | DCR_USR1 | DCR_USR0;
		sc->sc_32bit = 1;
		break;

	case MACH_MACPB500:
		sc->sc_dcr |= DCR_SBUS | DCR_LBR;
		sc->sc_32bit = 0;	/* 16 bit interface */
		break;

	default:
		printf(": unsupported machine type\n");
		return;
	}

	if (sn_obio_getaddr(sc, myaddr) &&
	    sn_obio_getaddr_kludge(sc, myaddr)) { /* XXX kludge for PB */
		printf(": failed to get MAC address.\n");
		bus_space_unmap(sc->sc_st, sc->sc_sh, SONIC_NREGS * 4);
		return;
	}

	printf(": integrated SONIC Ethernet adapter\n");

	if (mac68k_machine.aux_interrupts) {
		intr_establish(sonic_intr, (void *)sc, 3);
	} else {
		add_nubus_intr(SONIC_SLOTNO, (void (*)(void *))sonic_intr,
		    (void *)sc);
	}

	sonic_attach(sc, myaddr);
}

static int
sn_obio_getaddr(struct sonic_softc *sc, uint8_t *lladdr)
{
	bus_space_handle_t bsh;

	if (bus_space_map(sc->sc_st, SONIC_PROM_BASE, PAGE_SIZE, 0, &bsh)) {
		printf(": failed to map space to read SONIC address.\n%s",
		    sc->sc_dev.dv_xname);
		return (-1);
	}

	if (!mac68k_bus_space_probe(sc->sc_st, bsh, 0, 1)) {
		bus_space_unmap(sc->sc_st, bsh, PAGE_SIZE);
		return (-1);
	}

	sn_get_enaddr(sc->sc_st, bsh, 0, lladdr);

	bus_space_unmap(sc->sc_st, bsh, PAGE_SIZE);

	return 0;
}

/*
 * Assume that the SONIC was initialized in MacOS.  This should go away
 * when we can properly get the MAC address on the PBs.
 */
static int
sn_obio_getaddr_kludge(struct sonic_softc *sc, u_int8_t *lladdr)
{
	int i, ors = 0;

	/* Shut down NIC */
	CSR_WRITE(sc, SONIC_CR, CR_RST);

	/* For some reason, Apple fills top first. */
	CSR_WRITE(sc, SONIC_CEP, 15);

	i = CSR_READ(sc, SONIC_CAP2);

	ors |= i;
	lladdr[5] = i >> 8;
	lladdr[4] = i;

	i = CSR_READ(sc, SONIC_CAP1);

	ors |= i;
	lladdr[3] = i >> 8;
	lladdr[2] = i;

	i = CSR_READ(sc, SONIC_CAP0);

	ors |= i;
	lladdr[1] = i >> 8;
	lladdr[0] = i;

	CSR_WRITE(sc, SONIC_CR, 0);

	if (ors == 0)
		return -1;

	return 0;
}
