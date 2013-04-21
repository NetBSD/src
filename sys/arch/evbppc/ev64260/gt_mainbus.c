/*	$NetBSD: gt_mainbus.c,v 1.18 2013/04/21 15:42:11 kiyohara Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt_mainbus.c,v 1.18 2013/04/21 15:42:11 kiyohara Exp $");

#include "opt_ev64260.h"
#include "opt_pci.h"
#include "pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include "opt_pci.h"
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "opt_marvell.h"
#include <dev/marvell/gtreg.h>
#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>
#include <dev/marvell/marvellvar.h>
#include <dev/marvell/gtsdmareg.h>
#include <dev/marvell/gtmpscreg.h>
#ifdef MPSC_CONSOLE
#include <dev/marvell/gtmpscvar.h>
#endif

#include <evbppc/ev64260/ev64260.h>

#include <powerpc/pic/picvar.h>


static int gt_match(device_t, cfdata_t, void *);
static void gt_attach(device_t, device_t, void *);

void gtpci_md_conf_interrupt(void *, int, int, int, int, int *);
int gtpci_md_conf_hook(void *, int, int, int, pcireg_t);

CFATTACH_DECL_NEW(gt, sizeof(struct gt_softc), gt_match, gt_attach, NULL, NULL);

struct gtpci_prot gtpci_prot = {
	GTPCI_GT64260_ACBL_PCISWAP_NOSWAP	|
	GTPCI_GT64260_ACBL_WBURST_8_QW		|
	GTPCI_GT64260_ACBL_RDMULPREFETCH	|
	GTPCI_GT64260_ACBL_RDLINEPREFETCH	|
	GTPCI_GT64260_ACBL_RDPREFETCH		|
	GTPCI_GT64260_ACBL_PREFETCHEN,
	0,
};

int
gt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *mba = aux;

	if (strcmp(mba->mba_name, "gt") != 0)
		return 0;

	return 1;
}

void
gt_attach(device_t parent, device_t self, void *aux)
{
	extern struct powerpc_bus_space ev64260_gt_bs_tag;
	extern struct powerpc_bus_dma_tag ev64260_bus_dma_tag;
	struct mainbus_attach_args *mba = aux;
	struct gt_softc *sc = device_private(self);
	uint32_t cpumstr, cr, r;

	sc->sc_dev = self;
	sc->sc_addr = mba->mba_addr;
	sc->sc_iot = &ev64260_gt_bs_tag;
	sc->sc_dmat = &ev64260_bus_dma_tag;

#ifdef MPSC_CONSOLE
	{
		/* First, unmap already mapped console space. */
		gtmpsc_softc_t *gtmpsc = &gtmpsc_cn_softc;

		bus_space_unmap(gtmpsc->sc_iot, gtmpsc->sc_mpsch, GTMPSC_SIZE);
		bus_space_unmap(gtmpsc->sc_iot, gtmpsc->sc_sdmah, GTSDMA_SIZE);
	}
#endif
	if (bus_space_map(sc->sc_iot, sc->sc_addr, GT_SIZE, 0, &sc->sc_ioh) !=
	    0) {
		aprint_error_dev(self, "registers map failed\n");
		return;
	}
#ifdef MPSC_CONSOLE
	{
		/* Next, remap console space. */
		gtmpsc_softc_t *gtmpsc = &gtmpsc_cn_softc;

		if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
		    GTMPSC_BASE(gtmpsc->sc_unit), GTMPSC_SIZE,
		    &gtmpsc->sc_mpsch)) {
			aprint_error_dev(self, "Cannot map MPSC registers\n");
			return;
		}
		if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
		    GTSDMA_BASE(gtmpsc->sc_unit), GTSDMA_SIZE,
		    &gtmpsc->sc_sdmah)) {
			aprint_error_dev(self, "Cannot map SDMA registers\n");
			return;
		}
	}
#endif

	/*
	 * Set MPSC Routing:
	 *	MR0 --> Serial Port 0
	 *	MR1 --> Serial Port 1
	 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTMPSC_MRR, GTMPSC_MRR_RES);

	/*
	 * RX and TX Clock Routing:
	 *	CRR0 --> BRG0
	 *	CRR1 --> BRG1
	 */
	cr = GTMPSC_CRR(0, GTMPSC_CRR_BRG0) | GTMPSC_CRR(1, GTMPSC_CRR_BRG1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTMPSC_RCRR, cr);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTMPSC_TCRR, cr);

	/*
	 * Setup Multi-Purpose Pins (MPP).
	 *   Change to GPP.
	 *     GPP 21 (DUART channel A intr)
	 *     GPP 22 (DUART channel B intr)
	 *     GPP 26 (RTC INT)
	 *     GPP 27 (PCI 0 INTA)
	 *     GPP 29 (PCI 1 INTA)
	 */
#define PIN2SHIFT(pin)	((pin % 8) * 4)
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GT_MPP_Control2);
	r |= ((0xf << PIN2SHIFT(21)) | (0xf << PIN2SHIFT(22)));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_MPP_Control2, r);
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GT_MPP_Control3);
	r |= ((0xf << PIN2SHIFT(26)));
	r |= ((0xf << PIN2SHIFT(27)) | (0xf << PIN2SHIFT(29)));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_MPP_Control3, r);

	/* Also configure GPP. */
#define GPP_EXTERNAL_INTERRUPS \
	    ((1 << 21) | (1 << 22) | (1 << 26) | (1 << 27) | (1 << 29))
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GT_GPP_IO_Control);
	r &= ~GPP_EXTERNAL_INTERRUPS;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_GPP_IO_Control, r);
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GT_GPP_Level_Control);
	r |= GPP_EXTERNAL_INTERRUPS;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_GPP_Level_Control, r);

	/* clear interrupts */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ICR_CIM_LO, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ICR_CIM_HI, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_GPP_Interrupt_Cause,
	    ~GPP_EXTERNAL_INTERRUPS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_GPP_Interrupt_Mask,
	    GPP_EXTERNAL_INTERRUPS);

	discovery_pic->pic_cookie = sc;
	intr_establish(IRQ_GPP7_0, IST_LEVEL, IPL_HIGH,
	    pic_handle_intr, discovery_gpp_pic[0]);
	intr_establish(IRQ_GPP15_8, IST_LEVEL, IPL_HIGH,
	    pic_handle_intr, discovery_gpp_pic[1]);
	intr_establish(IRQ_GPP23_16, IST_LEVEL, IPL_HIGH,
	    pic_handle_intr, discovery_gpp_pic[2]);
	intr_establish(IRQ_GPP31_24, IST_LEVEL, IPL_HIGH,
	    pic_handle_intr, discovery_gpp_pic[3]);

	cpumstr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GT_CPU_Master_Ctl);
	cpumstr &= ~(GT_CPUMstrCtl_CleanBlock|GT_CPUMstrCtl_FlushBlock);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GT_CPU_Master_Ctl, cpumstr);

	gt_attach_common(sc);
}


void
gtpci_md_conf_interrupt(void *v, int bus, int dev, int pin, int swiz,
			int *iline)
{
#ifdef PCI_NETBSD_CONFIGURE
	struct gtpci_softc *sc = v;

	*iline = (sc->sc_unit == 0 ? 27 : 29);

#define IRQ_GPP_BASE	(discovery_pic->pic_numintrs);

	if (*iline != 0xff)
		*iline += IRQ_GPP_BASE;
#endif /* PCI_NETBSD_CONFIGURE */
}

int
gtpci_md_conf_hook(void *v, int bus, int dev, int func, pcireg_t id)
{
	struct gtpci_softc *sc = v;

	return gtpci_conf_hook(sc->sc_pc, bus, dev, func, id);
}


void *
marvell_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{

	/* pass through */
	return intr_establish(irq, IST_LEVEL, ipl, func, arg);
}
