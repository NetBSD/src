/*	$NetBSD: if_we_isa.c,v 1.1.2.2 2001/03/27 15:32:04 bouyer Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h" 

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h> 
#include <netinet/ip.h>
#include <netinet/if_inarp.h> 
#endif 

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>
#include <machine/bswap.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/wereg.h>
#include <dev/ic/wevar.h>

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define	bus_space_read_region_stream_2	bus_space_read_region_2
#define	bus_space_write_stream_2	bus_space_write_2
#define	bus_space_write_region_stream_2	bus_space_write_region_2
#endif

int	we_isa_probe __P((struct device *, struct cfdata *, void *));
void	we_isa_attach __P((struct device *, struct device *, void *));

struct cfattach we_isa_ca = {
	sizeof(struct we_softc), we_isa_probe, we_isa_attach
};

extern struct cfdriver we_cd;

static const char *we_params __P((bus_space_tag_t, bus_space_handle_t,
		u_int8_t *, bus_size_t *, int *, int *));

static const int we_584_irq[] = {
	9, 3, 5, 7, 10, 11, 15, 4,
};
#define	NWE_584_IRQ	(sizeof(we_584_irq) / sizeof(we_584_irq[0]))

static const int we_790_irq[] = {
	IRQUNK, 9, 3, 5, 7, 10, 11, 15,
};
#define	NWE_790_IRQ	(sizeof(we_790_irq) / sizeof(we_790_irq[0]))

/*
 * Delay needed when switching 16-bit access to shared memory.
 */
#define	WE_DELAY(wsc) delay(3)

/*
 * Enable card RAM, and 16-bit access.
 */
#define	WE_MEM_ENABLE(wsc) \
do { \
	if ((wsc)->sc_16bitp) \
		bus_space_write_1((wsc)->sc_asict, (wsc)->sc_asich, \
		    WE_LAAR, (wsc)->sc_laar_proto | WE_LAAR_M16EN); \
	bus_space_write_1((wsc)->sc_asict, (wsc)->sc_asich, \
	    WE_MSR, wsc->sc_msr_proto | WE_MSR_MENB); \
	WE_DELAY((wsc)); \
} while (0)

/*
 * Disable card RAM, and 16-bit access.
 */
#define	WE_MEM_DISABLE(wsc) \
do { \
	bus_space_write_1((wsc)->sc_asict, (wsc)->sc_asich, \
	    WE_MSR, (wsc)->sc_msr_proto); \
	if ((wsc)->sc_16bitp) \
		bus_space_write_1((wsc)->sc_asict, (wsc)->sc_asich, \
		    WE_LAAR, (wsc)->sc_laar_proto); \
	WE_DELAY((wsc)); \
} while (0)

int
we_isa_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t asict, memt;
	bus_space_handle_t asich, memh;
	bus_size_t memsize;
	int asich_valid, memh_valid;
	int i, is790, rv = 0;
	u_int8_t x, type;

	asict = ia->ia_iot;
	memt = ia->ia_memt;

	asich_valid = memh_valid = 0;

	/* Disallow wildcarded i/o addresses. */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return (0);

	/* Disallow wildcarded mem address. */
	if (ia->ia_maddr == ISACF_IOMEM_DEFAULT)
		return (0);

	/* Attempt to map the device. */
	if (bus_space_map(asict, ia->ia_iobase, WE_NPORTS, 0, &asich))
		goto out;
	asich_valid = 1;

#ifdef TOSH_ETHER
	bus_space_write_1(asict, asich, WE_MSR, WE_MSR_POW);
#endif

	/*
	 * Attempt to do a checksum over the station address PROM.
	 * If it fails, it's probably not a WD/SMC board.  There is
	 * a problem with this, though.  Some clone WD8003E boards
	 * (e.g. Danpex) won't pass the checksum.  In this case,
	 * the checksum byte always seems to be 0.
	 */
	for (x = 0, i = 0; i < 8; i++)
		x += bus_space_read_1(asict, asich, WE_PROM + i);

	if (x != WE_ROM_CHECKSUM_TOTAL) {
		/* Make sure it's an 8003E clone... */
		if (bus_space_read_1(asict, asich, WE_CARD_ID) !=
		    WE_TYPE_WD8003E)
			goto out;

		/* Check the checksum byte. */
		if (bus_space_read_1(asict, asich, WE_PROM + 7) != 0)
			goto out;
	}

	/*
	 * Reset the card to force it into a known state.
	 */
#ifdef TOSH_ETHER
	bus_space_write_1(asict, asich, WE_MSR, WE_MSR_RST | WE_MSR_POW);
#else
	bus_space_write_1(asict, asich, WE_MSR, WE_MSR_RST);
#endif
	delay(100);

	bus_space_write_1(asict, asich, WE_MSR,
	    bus_space_read_1(asict, asich, WE_MSR) & ~WE_MSR_RST);

	/* Wait in case the card is reading it's EEPROM. */
	delay(5000);

	/*
	 * Get parameters.
	 */
	if (we_params(asict, asich, &type, &memsize, NULL, &is790) == NULL)
		goto out;

	/* Allow user to override probed value. */
	if (ia->ia_msize)
		memsize = ia->ia_msize;

	/* Attempt to map the memory space. */
	if (bus_space_map(memt, ia->ia_maddr, memsize, 0, &memh))
		goto out;
	memh_valid = 1;

	/*
	 * If possible, get the assigned interrupt number from the card
	 * and use it.
	 */
	if (is790) {
		u_int8_t hwr;

		/* Assemble together the encoded interrupt number. */
		hwr = bus_space_read_1(asict, asich, WE790_HWR);
		bus_space_write_1(asict, asich, WE790_HWR,
		    hwr | WE790_HWR_SWH);

		x = bus_space_read_1(asict, asich, WE790_GCR);
		i = ((x & WE790_GCR_IR2) >> 4) |
		    ((x & (WE790_GCR_IR1|WE790_GCR_IR0)) >> 2);
		bus_space_write_1(asict, asich, WE790_HWR,
		    hwr & ~WE790_HWR_SWH);

		if (ia->ia_irq != IRQUNK && ia->ia_irq != we_790_irq[i])
			printf("%s%d: overriding IRQ %d to %d\n",
			    we_cd.cd_name, cf->cf_unit, ia->ia_irq,
			    we_790_irq[i]);
		ia->ia_irq = we_790_irq[i];
	} else if (type & WE_SOFTCONFIG) {
		/* Assemble together the encoded interrupt number. */
		i = (bus_space_read_1(asict, asich, WE_ICR) & WE_ICR_IR2) |
		    ((bus_space_read_1(asict, asich, WE_IRR) &
		      (WE_IRR_IR0 | WE_IRR_IR1)) >> 5);

		if (ia->ia_irq != IRQUNK && ia->ia_irq != we_584_irq[i])
			printf("%s%d: overriding IRQ %d to %d\n",
			    we_cd.cd_name, cf->cf_unit, ia->ia_irq,
			    we_584_irq[i]);
		ia->ia_irq = we_584_irq[i];
	}

	/* So, we say we've found it! */
	ia->ia_iosize = WE_NPORTS;
	ia->ia_msize = memsize;
	rv = 1;

 out:
	if (asich_valid)
		bus_space_unmap(asict, asich, WE_NPORTS);
	if (memh_valid)
		bus_space_unmap(memt, memh, memsize);
	return (rv);
}

void
we_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct we_softc *wsc = (struct we_softc *)self;
	struct dp8390_softc *sc = &wsc->sc_dp8390;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t nict, asict, memt;
	bus_space_handle_t nich, asich, memh;
	const char *typestr;

	printf("\n");

	nict = asict = ia->ia_iot;
	memt = ia->ia_memt;

	/* Map the device. */
	if (bus_space_map(asict, ia->ia_iobase, WE_NPORTS, 0, &asich)) {
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

	typestr = we_params(asict, asich, &wsc->sc_type, NULL,
	    &wsc->sc_16bitp, &sc->is790);
	if (typestr == NULL) {
		printf("%s: where did the card go?\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Map memory space.  Note we use the size that might have
	 * been overridden by the user.
	 */
	if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize, 0, &memh)) {
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

	wsc->sc_maddr = ia->ia_maddr;
	sc->mem_size = ia->ia_msize;

	/* Interface is always enabled. */
	sc->sc_enabled = 1;

	if (we_config(self, wsc, typestr))
		return;

	/*
	 * Enable the configured interrupt.
	 */
	if (sc->is790)
		bus_space_write_1(asict, asich, WE790_ICR,
		    bus_space_read_1(asict, asich, WE790_ICR) |
		    WE790_ICR_EIL);
	else if (wsc->sc_type & WE_SOFTCONFIG)
		bus_space_write_1(asict, asich, WE_IRR,
		    bus_space_read_1(asict, asich, WE_IRR) | WE_IRR_IEN);
	else if (ia->ia_irq == IRQUNK) {
		printf("%s: can't wildcard IRQ on a %s\n",
		    sc->sc_dev.dv_xname, typestr);
		return;
	}

	/* Establish interrupt handler. */
	wsc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, dp8390_intr, sc);
	if (wsc->sc_ih == NULL)
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
}

static const char *
we_params(asict, asich, typep, memsizep, is16bitp, is790p)
	bus_space_tag_t asict;
	bus_space_handle_t asich;
	u_int8_t *typep;
	bus_size_t *memsizep;
	int *is16bitp, *is790p;
{
	const char *typestr;
	bus_size_t memsize;
	int is16bit, is790;
	u_int8_t type;

	memsize = 8192;
	is16bit = is790 = 0;

	type = bus_space_read_1(asict, asich, WE_CARD_ID);
	switch (type) {
	case WE_TYPE_WD8003S: 
		typestr = "WD8003S"; 
		break;
	case WE_TYPE_WD8003E:
		typestr = "WD8003E";
		break;
	case WE_TYPE_WD8003EB: 
		typestr = "WD8003EB";
		break;
	case WE_TYPE_WD8003W:
		typestr = "WD8003W";
		break;
	case WE_TYPE_WD8013EBT: 
		typestr = "WD8013EBT";
		memsize = 16384;
		is16bit = 1;
		break;
	case WE_TYPE_WD8013W:
		typestr = "WD8013W";
		memsize = 16384;
		is16bit = 1;
		break;
	case WE_TYPE_WD8013EP:		/* also WD8003EP */
		if (bus_space_read_1(asict, asich, WE_ICR) & WE_ICR_16BIT) {
			is16bit = 1;
			memsize = 16384;
			typestr = "WD8013EP";
		} else
			typestr = "WD8003EP";
		break;
	case WE_TYPE_WD8013WC:
		typestr = "WD8013WC";
		memsize = 16384;
		is16bit = 1;
		break;
	case WE_TYPE_WD8013EBP:
		typestr = "WD8013EBP";
		memsize = 16384;
		is16bit = 1;
		break;
	case WE_TYPE_WD8013EPC:
		typestr = "WD8013EPC";
		memsize = 16384;
		is16bit = 1;
		break;
	case WE_TYPE_SMC8216C:
	case WE_TYPE_SMC8216T:
	    {
		u_int8_t hwr;

		typestr = (type == WE_TYPE_SMC8216C) ?
		    "SMC8216/SMC8216C" : "SMC8216T";

		hwr = bus_space_read_1(asict, asich, WE790_HWR);
		bus_space_write_1(asict, asich, WE790_HWR,
		    hwr | WE790_HWR_SWH);
		switch (bus_space_read_1(asict, asich, WE790_RAR) &
		    WE790_RAR_SZ64) {
		case WE790_RAR_SZ64:
			memsize = 65536;
			break;
		case WE790_RAR_SZ32:
			memsize = 32768;
			break;
		case WE790_RAR_SZ16:
			memsize = 16384;
			break;
		case WE790_RAR_SZ8:
			/* 8216 has 16K shared mem -- 8416 has 8K */
			typestr = (type == WE_TYPE_SMC8216C) ?
			    "SMC8416C/SMC8416BT" : "SMC8416T";
			memsize = 8192;
			break;
		}
		bus_space_write_1(asict, asich, WE790_HWR, hwr);

		is16bit = 1;
		is790 = 1;
		break;
	    }
#ifdef TOSH_ETHER
	case WE_TYPE_TOSHIBA1:
		typestr = "Toshiba1";
		memsize = 32768;
		is16bit = 1;
		break;
	case WE_TYPE_TOSHIBA4:
		typestr = "Toshiba4";
		memsize = 32768;
		is16bit = 1;
		break;
#endif
	default:
		/* Not one we recognize. */
		return (NULL);
	}

	/*
	 * Make some adjustments to initial values depending on what is
	 * found in the ICR.
	 */
	if (is16bit && (type != WE_TYPE_WD8013EBT) &&
#ifdef TOSH_ETHER
	    (type != WE_TYPE_TOSHIBA1 && type != WE_TYPE_TOSHIBA4) &&
#endif
	    (bus_space_read_1(asict, asich, WE_ICR) & WE_ICR_16BIT) == 0) {
		is16bit = 0;
		memsize = 8192;
	}

#ifdef WE_DEBUG
	{
		int i;

		printf("we_params: type = 0x%x, typestr = %s, is16bit = %d, "
		    "memsize = %d\n", type, typestr, is16bit, memsize);
		for (i = 0; i < 8; i++)
			printf("     %d -> 0x%x\n", i,
			    bus_space_read_1(asict, asich, i));
	}
#endif

	if (typep != NULL)
		*typep = type;
	if (memsizep != NULL)
		*memsizep = memsize;
	if (is16bitp != NULL)
		*is16bitp = is16bit;
	if (is790p != NULL)
		*is790p = is790;
	return (typestr);
}
