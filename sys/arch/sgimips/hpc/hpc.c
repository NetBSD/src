/*	$NetBSD: hpc.c,v 1.63 2009/12/14 00:46:13 matt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * Copyright (c) 2001 Rafal K. Boni
 * Copyright (c) 2001 Jason R. Thorpe
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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
__KERNEL_RCSID(0, "$NetBSD: hpc.c,v 1.63 2009/12/14 00:46:13 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/callout.h>

#define _SGIMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/ioc/iocreg.h>

#include <dev/ic/smc93cx6var.h>

#include "locators.h"

struct hpc_device {
	const char *hd_name;
	bus_addr_t hd_base;
	bus_addr_t hd_devoff;
	bus_addr_t hd_dmaoff;
	int hd_irq;
	int hd_sysmask;
};

static const struct hpc_device hpc1_devices[] = {
	/* probe order is important for IP20 zsc */

	{ "zsc",        /* Personal Iris/Indigo serial 0/1 duart 1 */
	  HPC_BASE_ADDRESS_0,
	  0x0d10, 0,
	  5,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "zsc",        /* Personal Iris/Indigo kbd/ms duart 0 */
	  HPC_BASE_ADDRESS_0,
	  0x0d00, 0,
	  5,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "sq",		/* Personal Iris/Indigo onboard ethernet */
	  HPC_BASE_ADDRESS_0,
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  3,
	  HPCDEV_IP12 | HPCDEV_IP20 },
	
	{ "sq",		/* E++ GIO adapter slot 0 (Indigo) */
	  HPC_BASE_ADDRESS_1,
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  6,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "sq",		/* E++ GIO adapter slot 0 (Indy) */
	  HPC_BASE_ADDRESS_1,
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  22,
	  HPCDEV_IP24 }, 

	{ "sq",		/* E++ GIO adapter slot 1 (Indigo) */
	  HPC_BASE_ADDRESS_2,
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  6,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "sq",		/* E++ GIO adapter slot 1 (Indy/Challenge S) */
	  HPC_BASE_ADDRESS_2,
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  23,
	  HPCDEV_IP24 },

	{ "wdsc",	/* Personal Iris/Indigo onboard SCSI */
	  HPC_BASE_ADDRESS_0,
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  2,    /* XXX 1 = IRQ_LOCAL0 + 2 */    
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "wdsc",	/* GIO32 SCSI adapter slot 0 (Indigo) */
	  HPC_BASE_ADDRESS_1,
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  6,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "wdsc",	/* GIO32 SCSI adapter slot 0 (Indy) */
	  HPC_BASE_ADDRESS_1,
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  22,
	  HPCDEV_IP24 }, 

	{ "wdsc",	/* GIO32 SCSI adapter slot 1 (Indigo) */
	  HPC_BASE_ADDRESS_2,
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  6,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "wdsc",	/* GIO32 SCSI adapter slot 1 (Indy/Challenge S) */
	  HPC_BASE_ADDRESS_2,
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  23,
	  HPCDEV_IP24 },

	{ NULL,
	  0,
	  0, 0,
	  0,
	  0
	}
};

static const struct hpc_device hpc3_devices[] = {
	{ "zsc",	/* serial 0/1 duart 0 */
	  HPC_BASE_ADDRESS_0,
	  /* XXX Magic numbers */
	  HPC3_PBUS_CH6_DEVREGS + IOC_SERIAL_REGS, 0,
	  29,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "pckbc",	/* Indigo2/Indy ps2 keyboard/mouse controller */
	  HPC_BASE_ADDRESS_0,
	  HPC3_PBUS_CH6_DEVREGS + IOC_KB_REGS, 0,
	  28,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "sq",		/* Indigo2/Indy/Challenge S/Challenge M onboard enet */
	  HPC_BASE_ADDRESS_0,
	  HPC3_ENET_DEVREGS, HPC3_ENET_REGS,
	  3,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "sq",		/* Challenge S IOPLUS secondary ethernet */
	  HPC_BASE_ADDRESS_1,
	  HPC3_ENET_DEVREGS, HPC3_ENET_REGS,
	  0,
	  HPCDEV_IP24 },

	{ "wdsc",	/* Indigo2/Indy/Challenge S/Challenge M onboard SCSI */
	  HPC_BASE_ADDRESS_0,
	  HPC3_SCSI0_DEVREGS, HPC3_SCSI0_REGS,
	  1,	/* XXX 1 = IRQ_LOCAL0 + 1 */
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "wdsc",	/* Indigo2/Challenge M secondary onboard SCSI */
	  HPC_BASE_ADDRESS_0,
	  HPC3_SCSI1_DEVREGS, HPC3_SCSI1_REGS,
	  2,	/* XXX 2 = IRQ_LOCAL0 + 2 */
	  HPCDEV_IP22 },

	{ "haltwo",	/* Indigo2/Indy onboard audio */
	  HPC_BASE_ADDRESS_0,
	  HPC3_PBUS_CH0_DEVREGS, HPC3_PBUS_DMAREGS,
	  8 + 4, /* XXX IRQ_LOCAL1 + 4 */
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "pi1ppc",	/* Indigo2/Indy/Challenge S/Challenge M onboard pport */
	  HPC_BASE_ADDRESS_0,
	  HPC3_PBUS_CH6_DEVREGS + IOC_PLP_REGS, 0,
	  -1,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "panel",	/* Indy front panel */
	  HPC_BASE_ADDRESS_0,
	  HPC3_PBUS_CH6_DEVREGS + IOC_PANEL, 0,
	  9,
	  HPCDEV_IP24 },

	{ NULL,
	  0,
	  0, 0,
	  0,
	  0
	}
};

struct hpc_softc {
	struct device 		sc_dev;

	bus_addr_t		sc_base;

	bus_space_tag_t		sc_ct;
	bus_space_handle_t	sc_ch;
};

static struct hpc_values hpc1_values = {
	.revision =		1,
	.scsi0_regs =		HPC1_SCSI0_REGS,
	.scsi0_regs_size =	HPC1_SCSI0_REGS_SIZE,
	.scsi0_cbp =		HPC1_SCSI0_CBP,
	.scsi0_ndbp = 		HPC1_SCSI0_NDBP,
	.scsi0_bc =		HPC1_SCSI0_BC,
	.scsi0_ctl =		HPC1_SCSI0_CTL,
	.scsi0_gio =		HPC1_SCSI0_GIO,
	.scsi0_dev =		HPC1_SCSI0_DEV,
	.scsi0_dmacfg =		HPC1_SCSI0_DMACFG,
	.scsi0_piocfg =		HPC1_SCSI0_PIOCFG,
	.scsi1_regs =		0,
	.scsi1_regs_size =	0,
	.scsi1_cbp =		0,
	.scsi1_ndbp =		0,
	.scsi1_bc =		0,
	.scsi1_ctl =		0,
	.scsi1_gio =		0,
	.scsi1_dev =		0,
	.scsi1_dmacfg =		0,
	.scsi1_piocfg =		0,
	.enet_regs =		HPC1_ENET_REGS,
	.enet_regs_size =	HPC1_ENET_REGS_SIZE,
	.enet_intdelay =	HPC1_ENET_INTDELAY,
	.enet_intdelayval =	HPC1_ENET_INTDELAY_OFF,
	.enetr_cbp =		HPC1_ENETR_CBP,
	.enetr_ndbp =		HPC1_ENETR_NDBP,
	.enetr_bc =		HPC1_ENETR_BC,
	.enetr_ctl =		HPC1_ENETR_CTL,
	.enetr_ctl_active =	HPC1_ENETR_CTL_ACTIVE,
	.enetr_reset =		HPC1_ENETR_RESET,
	.enetr_dmacfg =		0,
	.enetr_piocfg =		0,
	.enetx_cbp =		HPC1_ENETX_CBP,
	.enetx_ndbp =		HPC1_ENETX_NDBP,
	.enetx_bc =		HPC1_ENETX_BC,
	.enetx_ctl =		HPC1_ENETX_CTL,
	.enetx_ctl_active =	HPC1_ENETX_CTL_ACTIVE,
	.enetx_dev =		0,
	.enetr_fifo =		HPC1_ENETR_FIFO,
	.enetr_fifo_size =	HPC1_ENETR_FIFO_SIZE,
	.enetx_fifo =		HPC1_ENETX_FIFO,
	.enetx_fifo_size =	HPC1_ENETX_FIFO_SIZE,
	.scsi0_devregs_size =	HPC1_SCSI0_DEVREGS_SIZE,
	.scsi1_devregs_size =	0,
	.enet_devregs =		HPC1_ENET_DEVREGS,
	.enet_devregs_size =	HPC1_ENET_DEVREGS_SIZE,
	.pbus_fifo =		0,	
	.pbus_fifo_size =	0,
	.pbus_bbram =		0,
#define MAX_SCSI_XFER   (512*1024)
	.scsi_max_xfer =	MAX_SCSI_XFER,
	.scsi_dma_segs =       (MAX_SCSI_XFER / 4096),
	.scsi_dma_segs_size =	4096,
	.scsi_dma_datain_cmd = (HPC1_SCSI_DMACTL_ACTIVE | HPC1_SCSI_DMACTL_DIR),
	.scsi_dma_dataout_cmd =	HPC1_SCSI_DMACTL_ACTIVE,
	.scsi_dmactl_flush =	HPC1_SCSI_DMACTL_FLUSH,
	.scsi_dmactl_active =	HPC1_SCSI_DMACTL_ACTIVE,
	.scsi_dmactl_reset =	HPC1_SCSI_DMACTL_RESET
};

static struct hpc_values hpc3_values = {
	.revision =		3,
	.scsi0_regs =		HPC3_SCSI0_REGS,
	.scsi0_regs_size =	HPC3_SCSI0_REGS_SIZE,
	.scsi0_cbp =		HPC3_SCSI0_CBP,
	.scsi0_ndbp =		HPC3_SCSI0_NDBP,
	.scsi0_bc =		HPC3_SCSI0_BC,
	.scsi0_ctl =		HPC3_SCSI0_CTL,
	.scsi0_gio =		HPC3_SCSI0_GIO,
	.scsi0_dev =		HPC3_SCSI0_DEV,
	.scsi0_dmacfg =		HPC3_SCSI0_DMACFG,
	.scsi0_piocfg =		HPC3_SCSI0_PIOCFG,
	.scsi1_regs =		HPC3_SCSI1_REGS,
	.scsi1_regs_size =	HPC3_SCSI1_REGS_SIZE,
	.scsi1_cbp =		HPC3_SCSI1_CBP,
	.scsi1_ndbp =		HPC3_SCSI1_NDBP,
	.scsi1_bc =		HPC3_SCSI1_BC,
	.scsi1_ctl =		HPC3_SCSI1_CTL,
	.scsi1_gio =		HPC3_SCSI1_GIO,
	.scsi1_dev =		HPC3_SCSI1_DEV,
	.scsi1_dmacfg =		HPC3_SCSI1_DMACFG,
	.scsi1_piocfg =		HPC3_SCSI1_PIOCFG,
	.enet_regs =		HPC3_ENET_REGS,
	.enet_regs_size =	HPC3_ENET_REGS_SIZE,
	.enet_intdelay =	0,
	.enet_intdelayval =	0,
	.enetr_cbp =		HPC3_ENETR_CBP,
	.enetr_ndbp =		HPC3_ENETR_NDBP,
	.enetr_bc =		HPC3_ENETR_BC,
	.enetr_ctl =		HPC3_ENETR_CTL,
	.enetr_ctl_active =	HPC3_ENETR_CTL_ACTIVE,
	.enetr_reset =		HPC3_ENETR_RESET,
	.enetr_dmacfg =		HPC3_ENETR_DMACFG,
	.enetr_piocfg =		HPC3_ENETR_PIOCFG,
	.enetx_cbp =		HPC3_ENETX_CBP,
	.enetx_ndbp =		HPC3_ENETX_NDBP,
	.enetx_bc =		HPC3_ENETX_BC,
	.enetx_ctl =		HPC3_ENETX_CTL,
	.enetx_ctl_active =	HPC3_ENETX_CTL_ACTIVE,
	.enetx_dev =		HPC3_ENETX_DEV,
	.enetr_fifo =		HPC3_ENETR_FIFO,
	.enetr_fifo_size =	HPC3_ENETR_FIFO_SIZE,
	.enetx_fifo =		HPC3_ENETX_FIFO,
	.enetx_fifo_size =	HPC3_ENETX_FIFO_SIZE,
	.scsi0_devregs_size =	HPC3_SCSI0_DEVREGS_SIZE,
	.scsi1_devregs_size =	HPC3_SCSI1_DEVREGS_SIZE,
	.enet_devregs =		HPC3_ENET_DEVREGS,
	.enet_devregs_size =	HPC3_ENET_DEVREGS_SIZE,
	.pbus_fifo =		HPC3_PBUS_FIFO,
	.pbus_fifo_size =	HPC3_PBUS_FIFO_SIZE,
	.pbus_bbram =		HPC3_PBUS_BBRAM,
	.scsi_max_xfer =	MAX_SCSI_XFER,
	.scsi_dma_segs =       (MAX_SCSI_XFER / 8192),
	.scsi_dma_segs_size =	8192,
	.scsi_dma_datain_cmd =	HPC3_SCSI_DMACTL_ACTIVE,
	.scsi_dma_dataout_cmd =(HPC3_SCSI_DMACTL_ACTIVE | HPC3_SCSI_DMACTL_DIR),
	.scsi_dmactl_flush =	HPC3_SCSI_DMACTL_FLUSH,
	.scsi_dmactl_active =	HPC3_SCSI_DMACTL_ACTIVE,
	.scsi_dmactl_reset =	HPC3_SCSI_DMACTL_RESET
};


static int powerintr_established;

static int	hpc_match(struct device *, struct cfdata *, void *);
static void	hpc_attach(struct device *, struct device *, void *);
static int	hpc_print(void *, const char *);

static int	hpc_revision(struct hpc_softc *, struct gio_attach_args *);

static int	hpc_submatch(struct device *, struct cfdata *,
		     const int *, void *);

//static int	hpc_power_intr(void *);

#if defined(BLINK)
static callout_t hpc_blink_ch;
static void	hpc_blink(void *);
#endif

static int	hpc_read_eeprom(int, bus_space_tag_t, bus_space_handle_t,
		    uint8_t *, size_t);

CFATTACH_DECL(hpc, sizeof(struct hpc_softc),
    hpc_match, hpc_attach, NULL, NULL);

static int
hpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct gio_attach_args* ga = aux;

	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20 ||
	    mach_type == MACH_SGI_IP22) {
		/* Make sure it's actually there and readable */
		if (!platform.badaddr((void*)MIPS_PHYS_TO_KSEG1(ga->ga_addr),
		    sizeof(u_int32_t)))
			return 1;
	}

	return 0;
}

static void
hpc_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpc_softc *sc = (struct hpc_softc *)self;
	struct gio_attach_args* ga = aux;
	struct hpc_attach_args ha;
	const struct hpc_device *hd;
	uint32_t hpctype;
	int isonboard;
	int isioplus;
	int sysmask;

#ifdef BLINK
	callout_init(&hpc_blink_ch, 0);
#endif

	switch (mach_type) {
	case MACH_SGI_IP12:
		sysmask = HPCDEV_IP12;
		break;

	case MACH_SGI_IP20:
		sysmask = HPCDEV_IP20;
		break;

	case MACH_SGI_IP22:
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
			sysmask = HPCDEV_IP22;
		else
			sysmask = HPCDEV_IP24;
		break;

	default:
		panic("hpc_attach: can't handle HPC on an IP%d", mach_type);
	};

	if ((hpctype = hpc_revision(sc, ga)) == 0)
		panic("hpc_attach: could not identify HPC revision\n");

	/* force big-endian mode */
	if (hpctype == 15)
		*(uint32_t *)MIPS_PHYS_TO_KSEG1(ga->ga_addr+HPC1_BIGENDIAN) = 0;

	/*
	 * All machines have only one HPC on the mainboard itself. ''Extra''
	 * HPCs require bus arbiter and other magic to run happily.
	 */
	isonboard = (ga->ga_addr == HPC_BASE_ADDRESS_0);
	isioplus = (ga->ga_addr == HPC_BASE_ADDRESS_1 && hpctype == 3 &&
	    sysmask == HPCDEV_IP24);
	
	printf(": SGI HPC%d%s (%s)\n", (hpctype ==  3) ? 3 : 1,
	    (hpctype == 15) ? ".5" : "", (isonboard) ? "onboard" :
	    (isioplus) ? "IOPLUS mezzanine" : "GIO slot");

	/*
	 * Configure the bus arbiter appropriately.
	 *
	 * In the case of Challenge S, we must tell the IOPLUS board which
	 * DMA channel to use (we steal it from one of the slots). SGI permits
	 * an HPC1.5 in slot 1, in which case IOPLUS must use EXP0, or any
	 * other DMA-capable board in slot 0, which leaves us to use EXP1. Of
	 * course, this means that only one GIO board may use DMA.
	 *
	 * Note that this never happens on Indigo2.
	 */
	if (isioplus) {
		int arb_slot;

		if (platform.badaddr(
		    (void *)MIPS_PHYS_TO_KSEG1(HPC_BASE_ADDRESS_2), 4))
			arb_slot = GIO_SLOT_EXP1;
		else
			arb_slot = GIO_SLOT_EXP0;

		if (gio_arb_config(arb_slot, GIO_ARB_LB | GIO_ARB_MST |
		    GIO_ARB_64BIT | GIO_ARB_HPC2_64BIT)) {
			printf("%s: failed to configure GIO bus arbiter\n",
			    sc->sc_dev.dv_xname);
			return;
		}

		printf("%s: using EXP%d's DMA channel\n", sc->sc_dev.dv_xname,
		    (arb_slot == GIO_SLOT_EXP0) ? 0 : 1);

		bus_space_write_4(ga->ga_iot, ga->ga_ioh,
		    HPC3_PBUS_CFGPIO_REGS, 0x0003ffff);

		if (arb_slot == GIO_SLOT_EXP0)
			bus_space_write_4(ga->ga_iot, ga->ga_ioh,
			    HPC3_PBUS_CH0_DEVREGS, 0x20202020);
		else
			bus_space_write_4(ga->ga_iot, ga->ga_ioh,
			    HPC3_PBUS_CH0_DEVREGS, 0x30303030);
	} else if (!isonboard) {
		int arb_slot;

		arb_slot = (ga->ga_addr == HPC_BASE_ADDRESS_1) ?
		    GIO_SLOT_EXP0 : GIO_SLOT_EXP1;

		if (gio_arb_config(arb_slot, GIO_ARB_RT | GIO_ARB_MST)) {
			printf("%s: failed to configure GIO bus arbiter\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->sc_ct = SGIMIPS_BUS_SPACE_HPC;
	sc->sc_ch = ga->ga_ioh;

	sc->sc_base = ga->ga_addr;

	hpc_read_eeprom(hpctype, SGIMIPS_BUS_SPACE_HPC,
	    MIPS_PHYS_TO_KSEG1(sc->sc_base), ha.hpc_eeprom,
	    sizeof(ha.hpc_eeprom));

	hd = (hpctype == 3) ? hpc3_devices : hpc1_devices;
	for (; hd->hd_name != NULL; hd++) {
		if (!(hd->hd_sysmask & sysmask) || hd->hd_base != sc->sc_base)
			continue;

		ha.ha_name = hd->hd_name;
		ha.ha_devoff = hd->hd_devoff;
		ha.ha_dmaoff = hd->hd_dmaoff;
		ha.ha_irq = hd->hd_irq;

		/* XXX This is disgusting. */
		ha.ha_st = SGIMIPS_BUS_SPACE_HPC;
		ha.ha_sh = MIPS_PHYS_TO_KSEG1(sc->sc_base);
		ha.ha_dmat = &sgimips_default_bus_dma_tag;
		if (hpctype == 3)
			ha.hpc_regs = &hpc3_values;
		else
			ha.hpc_regs = &hpc1_values;
		ha.hpc_regs->revision = hpctype;

		/* XXXgross! avoid complaining in E++ and GIO32 SCSI cases */
		if (hpctype != 3 && sc->sc_base != HPC_BASE_ADDRESS_0) {
			(void)config_found_sm_loc(self, "hpc", NULL, &ha,
			    NULL, hpc_submatch);
		} else {
			(void)config_found_sm_loc(self, "hpc", NULL, &ha,
			    hpc_print, hpc_submatch);
		}
	}

	/*
	 * XXX: Only attach the powerfail interrupt once, since the
	 * interrupt code doesn't let you share interrupt just yet.
	 *
	 * Since the powerfail interrupt is hardcoded to read from
	 * a specific register anyway (XXX#2!), we don't care when
	 * it gets attached, as long as it only happens once.
	 */
	if (mach_type == MACH_SGI_IP22 && !powerintr_established) {
//		cpu_intr_establish(9, IPL_NONE, hpc_power_intr, sc);
		powerintr_established++;
	}

#if defined(BLINK)
	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20)
		hpc_blink(sc);
#endif
}

/*
 * HPC revision detection isn't as simple as it should be. Devices probe
 * differently depending on their slots, but luckily there is only one
 * instance in which we have to decide the major revision (HPC1 vs HPC3).
 *
 * The HPC is found in the following configurations:
 *	o Personal Iris 4D/3x:
 *		One on-board HPC1 or HPC1.5.
 *
 *	o Indigo R3k/R4k:
 * 		One on-board HPC1 or HPC1.5.
 * 		Up to two additional HPC1.5's in GIO slots 0 and 1.
 *
 *	o Indy:
 * 		One on-board HPC3.
 *		Up to two additional HPC1.5's in GIO slots 0 and 1.
 *
 *	o Challenge S
 * 		One on-board HPC3.
 * 		Up to one additional HPC3 on the IOPLUS board (if installed).
 *		Up to one additional HPC1.5 in slot 1 of the IOPLUS board.
 *
 *	o Indigo2, Challenge M
 *		One on-board HPC3.
 *
 * All we really have to worry about is the IP22 case.
 */
static int
hpc_revision(struct hpc_softc *sc, struct gio_attach_args *ga)
{

	/* No hardware ever supported the last hpc base address. */
	if (ga->ga_addr == HPC_BASE_ADDRESS_3)
		return (0);

	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20) {
		u_int32_t reg;

		if (!platform.badaddr((void *)MIPS_PHYS_TO_KSEG1(ga->ga_addr +
		    HPC1_BIGENDIAN), 4)) {
			reg = *(uint32_t *)MIPS_PHYS_TO_KSEG1(ga->ga_addr +
			    HPC1_BIGENDIAN);

			if (((reg >> HPC1_REVSHIFT) & HPC1_REVMASK) ==
			    HPC1_REV15)
				return (15);
			else
				return (1);
		}

		return (1);
	}

	/*
	 * If IP22, probe slot 0 to determine if HPC1.5 or HPC3. Slot 1 must
	 * be HPC1.5.
	 */
	if (mach_type == MACH_SGI_IP22) {
		if (ga->ga_addr == HPC_BASE_ADDRESS_0)
			return (3);

		if (ga->ga_addr == HPC_BASE_ADDRESS_2)
			return (15);

		/*
		 * Probe for it. We use one of the PBUS registers. Note
		 * that this probe succeeds with my E++ adapter in slot 1
		 * (bad), but it appears to always do the right thing in
		 * slot 0 (good!) and we're only worried about that one
		 * anyhow.
		 */
		if (platform.badaddr((void *)MIPS_PHYS_TO_KSEG1(ga->ga_addr +
		    HPC3_PBUS_CH7_BP), 4))
			return (15);
		else
			return (3);
	}

	return (0);
}

static int
hpc_submatch(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (cf->cf_loc[HPCCF_OFFSET] != HPCCF_OFFSET_DEFAULT &&
	    (bus_addr_t) cf->cf_loc[HPCCF_OFFSET] != ha->ha_devoff)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
hpc_print(void *aux, const char *pnp)
{
	struct hpc_attach_args *ha = aux;

	if (pnp)
		printf("%s at %s", ha->ha_name, pnp);

	printf(" offset %#" PRIxVADDR, (vaddr_t)ha->ha_devoff);

	return (UNCONF);
}

#if 0
static int
hpc_power_intr(void *arg)
{
	u_int32_t pwr_reg;

	pwr_reg = *((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850));
	*((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9850)) = pwr_reg;

	printf("hpc_power_intr: panel reg = %08x\n", pwr_reg);

	if (pwr_reg & 2)
		cpu_reboot(RB_HALT, NULL);

	return 1;
}
#endif

#if defined(BLINK)
static void
hpc_blink(void *self)
{
	struct hpc_softc *sc = (struct hpc_softc *) self;
	register int	s;
	int	value;

	s = splhigh();

	value = *(volatile uint8_t *)MIPS_PHYS_TO_KSEG1(HPC_BASE_ADDRESS_0 +
	    HPC1_AUX_REGS);
	value ^= HPC1_AUX_CONSLED;
	*(volatile u_int8_t *)MIPS_PHYS_TO_KSEG1(HPC_BASE_ADDRESS_0 +
	    HPC1_AUX_REGS) = value;
	splx(s);

	/*
	 * Blink rate is:
	 *      full cycle every second if completely idle (loadav = 0)
	 *      full cycle every 2 seconds if loadav = 1
	 *      full cycle every 3 seconds if loadav = 2
	 * etc.
	 */
	s = (((averunnable.ldavg[0] + FSCALE) * hz) >> (FSHIFT + 1));
	callout_reset(&hpc_blink_ch, s, hpc_blink, sc);
}
#endif

/*
 * Read the eeprom associated with one of the HPC's.
 *
 * NB: An eeprom is not always present, but the HPC should be able to
 *     handle this gracefully. Any consumers should validate the data to
 *     ensure it's reasonable.
 */
static int
hpc_read_eeprom(int hpctype, bus_space_tag_t t, bus_space_handle_t h,
    uint8_t *buf, size_t len)
{
	struct seeprom_descriptor sd;
	bus_space_handle_t bsh;
	bus_space_tag_t tag;
	bus_size_t offset;

	if (!len || len & 0x1)
		return (1);

	offset = (hpctype == 3) ? HPC3_EEPROM_DATA : HPC1_AUX_REGS;

	tag = SGIMIPS_BUS_SPACE_NORMAL;
	if (bus_space_subregion(t, h, offset, 1, &bsh) != 0)
		return (1);

	sd.sd_chip = C56_66;
	sd.sd_tag = tag;
	sd.sd_bsh = bsh;
	sd.sd_regsize = 1;
	sd.sd_control_offset = 0;
	sd.sd_status_offset = 0;
	sd.sd_dataout_offset = 0;
	sd.sd_DI = 0x10;	/* EEPROM -> CPU */
	sd.sd_DO = 0x08;	/* CPU -> EEPROM */
	sd.sd_CK = 0x04;
	sd.sd_CS = 0x02;
	sd.sd_MS = 0;
	sd.sd_RDY = 0;

	if (read_seeprom(&sd, (uint16_t *)buf, 0, len / 2) != 1)
		return (1);

	bus_space_unmap(t, bsh, 1);

	return (0);
}
