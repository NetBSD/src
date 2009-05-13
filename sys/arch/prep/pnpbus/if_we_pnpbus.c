/*	$NetBSD: if_we_pnpbus.c,v 1.4.14.1 2009/05/13 17:18:15 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_we_pnpbus.c,v 1.4.14.1 2009/05/13 17:18:15 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/bswap.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/isa_machdep.h>
#include <machine/residual.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/wereg.h>
#include <dev/ic/wevar.h>

#include <prep/pnpbus/pnpbusvar.h>

int	we_pnpbus_probe(struct device *, struct cfdata *, void *);
void	we_pnpbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(we_pnpbus, sizeof(struct we_softc),
    we_pnpbus_probe, we_pnpbus_attach, NULL, NULL);

extern struct cfdriver we_cd;

static const char *we_params(bus_space_tag_t, bus_space_handle_t,
		u_int8_t *, bus_size_t *, u_int8_t *, int *);

#define WE_DEFAULT_IOMEM 0xe4000

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
we_pnpbus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	if (strcmp(pna->pna_devid, "IBM2001") == 0)
		ret = 1;

	if (strcmp(pna->pna_devid, "IBM0010") == 0)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);

	return ret;
}

void
we_pnpbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct we_softc *wsc = (struct we_softc *)self;
	struct dp8390_softc *sc = &wsc->sc_dp8390;
	struct pnpbus_dev_attach_args *pna = aux;
	struct pnpbus_irq *irq;
	bus_space_tag_t nict, asict, memt;
	bus_space_handle_t nich, asich, memh;
	bus_size_t memsize = 0x4000;
	const char *typestr;
	int memfound = 0, i, irqnum;

	nict = asict = pna->pna_iot;
	memt = pna->pna_memt;

	aprint_normal("\n");

	if (pnpbus_io_map(&pna->pna_res, 0, &asict, &asich)) {
		aprint_error("%s: can't map nic i/o space\n",
		    device_xname(self));
		return;
	}

	if (bus_space_subregion(asict, asich, WE_NIC_OFFSET, WE_NIC_NPORTS,
	    &nich)) {
		aprint_error("%s: can't subregion i/o space\n",
		    device_xname(self));
		return;
	}

	typestr = we_params(asict, asich, &wsc->sc_type, &memsize,
	    &wsc->sc_flags, &sc->is790);
	if (typestr == NULL) {
		aprint_error("%s: where did the card go?\n",
		    device_xname(self));
		return;
	}

	/*
	 * Map memory space.  See if any was allocated via PNP, if not, use
	 * the default.
	 */
	if (pnpbus_iomem_map(&pna->pna_res, 0, &memt, &memh) == 0)
		memfound = 1;
	else if (bus_space_map(memt, WE_DEFAULT_IOMEM, memsize, 0, &memh)) {
		aprint_error("%s: can't map shared memory\n",
		    device_xname(self));
		return;
	}

	wsc->sc_asict = asict;
	wsc->sc_asich = asich;

	sc->sc_regt = nict;
	sc->sc_regh = nich;

	sc->sc_buft = memt;
	sc->sc_bufh = memh;

	if (memfound)
		pnpbus_getiomem(&pna->pna_res, 0, &wsc->sc_maddr,
		    &sc->mem_size);
	else {
		wsc->sc_maddr = WE_DEFAULT_IOMEM;
		sc->mem_size = memsize;
	}

	/* Interface is always enabled. */
	sc->sc_enabled = 1;

	if (we_config(self, wsc, typestr))
		return;

	/*
	 * Enable the configured interrupt.
	 */
#if 0
	if (sc->is790)
		bus_space_write_1(asict, asich, WE790_ICR,
		    bus_space_read_1(asict, asich, WE790_ICR) |
		    WE790_ICR_EIL);
	else if (wsc->sc_type & WE_SOFTCONFIG)
		bus_space_write_1(asict, asich, WE_IRR,
		    bus_space_read_1(asict, asich, WE_IRR) | WE_IRR_IEN);
#endif

	/*
	 * Establish interrupt handler.
	 * Loop through all probed IRQs until one looks sane.
	 */
	for (i = 0, irq = SIMPLEQ_FIRST(&pna->pna_res.irq);
 	     i < pna->pna_res.numirq; i++, irq = SIMPLEQ_NEXT(irq, next)) {
		irqnum = ffs(irq->mask) - 1;
		/* some cards think they are level.  force them to edge */
		if (irq->flags & 0x0c)
			irq->flags = 0x01;
		if (!LEGAL_IRQ(irqnum))
			continue;
		if (irqnum < 2)
			continue;
		break;
	}
	wsc->sc_ih = pnpbus_intr_establish(i, IPL_NET, IST_PNP, dp8390_intr, sc,
	    &pna->pna_res);
	if (wsc->sc_ih == NULL)
		aprint_error("%s: can't establish interrupt\n",
		    device_xname(self));
}

static const char *
we_params(bus_space_tag_t asict, bus_space_handle_t asich, u_int8_t *typep, bus_size_t *memsizep, u_int8_t *flagp, int *is790p)
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

		aprint_debug("we_params: type = 0x%x, typestr = %s,"
		    " is16bit = %d, memsize = %d\n", type, typestr, is16bit,
		    memsize);
		for (i = 0; i < 8; i++)
			aprint_debug("     %d -> 0x%x\n", i,
			    bus_space_read_1(asict, asich, i));
	}
#endif

	if (typep != NULL)
		*typep = type;
	if (memsizep != NULL)
		*memsizep = memsize;
	if (flagp != NULL && is16bit)
		*flagp |= WE_16BIT_ENABLE;
	if (is790p != NULL)
		*is790p = is790;
	return (typestr);
}
