/*	$NetBSD: hpc.c,v 1.28 2004/04/11 10:29:20 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: hpc.c,v 1.28 2004/04/11 10:29:20 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/callout.h>

#include <machine/machtype.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>
#include <sgimips/ioc/iocreg.h>

#include "locators.h"

const struct hpc_device {
	const char *hd_name;
	bus_addr_t hd_devoff;
	bus_addr_t hd_dmaoff;
	int hd_irq;
	int hd_sysmask;
} hpc_devices[] = {
	{ "zsc",
	  /* XXX Magic numbers */
	  HPC_PBUS_CH6_DEVREGS + IOC_SERIAL_REGS, 0,
	  29,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	/* probe order is important for IP20 zsc */

	{ "zsc",        /* serial 0/1 duart 1 */
	  0x0d10, 0,
	  5,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "zsc",        /* serial 0/1 duart 0 */
	  0x0d00, 0,
	  5,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "pckbc",
	  HPC_PBUS_CH6_DEVREGS + IOC_KB_REGS, 0,
	  28,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "sq",
	  HPC_ENET_DEVREGS, HPC_ENET_REGS,
	  3,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "sq",
	  HPC1_ENET_DEVREGS, HPC1_ENET_REGS,
	  3,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "wdsc",
	  HPC_SCSI0_DEVREGS, HPC_SCSI0_REGS,
	  1,	/* XXX 1 = IRQ_LOCAL0 + 1 */
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "wdsc",
	  HPC_SCSI1_DEVREGS, HPC_SCSI1_REGS,
	  2,	/* XXX 2 = IRQ_LOCAL0 + 2 */
	  HPCDEV_IP22 },

	{ "wdsc",
	  HPC1_SCSI0_DEVREGS, HPC1_SCSI0_REGS,
	  2,    /* XXX 1 = IRQ_LOCAL0 + 2 */    
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "dpclock",
	  HPC1_PBUS_BBRAM, 0,
	  -1,
	  HPCDEV_IP12 | HPCDEV_IP20 },

	{ "dsclock",
	  HPC_PBUS_BBRAM, 0,
	  -1,
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ "haltwo",
	  HPC_PBUS_CH0_DEVREGS, HPC_PBUS_DMAREGS,
	  8 + 4, /* XXX IRQ_LOCAL1 + 4 */
	  HPCDEV_IP22 | HPCDEV_IP24 },

	{ NULL,
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
	.scsi1_regs =		HPC1_SCSI1_REGS,
	.scsi1_regs_size =	HPC1_SCSI1_REGS_SIZE,
	.scsi1_cbp =		HPC1_SCSI1_CBP,
	.scsi1_ndbp =		HPC1_SCSI1_NDBP,
	.scsi1_bc =		HPC1_SCSI1_BC,
	.scsi1_ctl =		HPC1_SCSI1_CTL,
	.scsi1_gio =		HPC1_SCSI1_GIO,
	.scsi1_dev =		HPC1_SCSI1_DEV,
	.scsi1_dmacfg =		HPC1_SCSI1_DMACFG,
	.scsi1_piocfg =		HPC1_SCSI1_PIOCFG,
	.dmactl_dir =		HPC1_DMACTL_DIR,
	.dmactl_flush =		HPC1_DMACTL_FLUSH,
	.dmactl_active =	HPC1_DMACTL_ACTIVE,
	.dmactl_reset =		HPC1_DMACTL_RESET,
	.enet_regs =		HPC1_ENET_REGS,
	.enet_regs_size =	HPC1_ENET_REGS_SIZE,
	.enet_intdelay =	HPC1_ENET_INTDELAY,
	.enet_intdelayval =	HPC1_ENET_INTDELAYVAL,
	.enetr_cbp =		HPC1_ENETR_CBP,
	.enetr_ndbp =		HPC1_ENETR_NDBP,
	.enetr_bc =		HPC1_ENETR_BC,
	.enetr_ctl =		HPC1_ENETR_CTL,
	.enetr_ctl_active =	HPC1_ENETR_CTL_ACTIVE,
	.enetr_reset =		HPC1_ENETR_RESET,
	.enetr_dmacfg =		0,
	.enetr_piocfg =		HPC1_ENETR_PIOCFG,
	.enetx_cbp =		HPC1_ENETX_CBP,
	.enetx_ndbp =		HPC1_ENETX_NDBP,
	.enetx_bc =		HPC1_ENETX_BC,
	.enetx_ctl =		HPC1_ENETX_CTL,
	.enetx_ctl_active =	HPC1_ENETX_CTL_ACTIVE,
	.enetx_dev =		HPC1_ENETX_DEV,
	.enetr_fifo =		HPC1_ENETR_FIFO,
	.enetr_fifo_size =	HPC1_ENETR_FIFO_SIZE,
	.enetx_fifo =		HPC1_ENETX_FIFO,
	.enetx_fifo_size =	HPC1_ENETX_FIFO_SIZE,
	.scsi0_devregs_size =	HPC1_SCSI0_DEVREGS_SIZE,
	.scsi1_devregs_size =	HPC1_SCSI0_DEVREGS_SIZE,
	.enet_devregs =		HPC1_ENET_DEVREGS,
	.enet_devregs_size =	HPC1_ENET_DEVREGS_SIZE,
	.pbus_fifo =		HPC1_PBUS_FIFO,
	.pbus_fifo_size =	HPC1_PBUS_FIFO_SIZE,
	.pbus_bbram =		HPC1_PBUS_BBRAM,
#define MAX_SCSI_XFER   (512*1024)
	.scsi_max_xfer =	MAX_SCSI_XFER,
	.scsi_dma_segs =	(MAX_SCSI_XFER / 4096),
	.scsi_dma_segs_size =	4096,
	.clk_freq =		100,
	.dma_datain_cmd =	(HPC1_DMACTL_ACTIVE | HPC1_DMACTL_DIR),
	.dma_dataout_cmd =	HPC1_DMACTL_ACTIVE,
	.scsi_dmactl_flush =	HPC1_DMACTL_FLUSH,
	.scsi_dmactl_active =	HPC1_DMACTL_ACTIVE,
	.scsi_dmactl_reset =	HPC1_DMACTL_RESET
};

static struct hpc_values hpc3_values = {
	.revision		3,
	.scsi0_regs =		HPC_SCSI0_REGS,
	.scsi0_regs_size =	HPC_SCSI0_REGS_SIZE,
	.scsi0_cbp =		HPC_SCSI0_CBP,
	.scsi0_ndbp =		HPC_SCSI0_NDBP,
	.scsi0_bc =		HPC_SCSI0_BC,
	.scsi0_ctl =		HPC_SCSI0_CTL,
	.scsi0_gio =		HPC_SCSI0_GIO,
	.scsi0_dev =		HPC_SCSI0_DEV,
	.scsi0_dmacfg =		HPC_SCSI0_DMACFG,
	.scsi0_piocfg =		HPC_SCSI0_PIOCFG,
	.scsi1_regs =		HPC_SCSI1_REGS,
	.scsi1_regs_size =	HPC_SCSI1_REGS_SIZE,
	.scsi1_cbp =		HPC_SCSI1_CBP,
	.scsi1_ndbp =		HPC_SCSI1_NDBP,
	.scsi1_bc =		HPC_SCSI1_BC,
	.scsi1_ctl =		HPC_SCSI1_CTL,
	.scsi1_gio =		HPC_SCSI1_GIO,
	.scsi1_dev =		HPC_SCSI1_DEV,
	.scsi1_dmacfg =		HPC_SCSI1_DMACFG,
	.scsi1_piocfg =		HPC_SCSI1_PIOCFG,
	.dmactl_dir =		HPC_DMACTL_DIR,
	.dmactl_flush =		HPC_DMACTL_FLUSH,
	.dmactl_active =	HPC_DMACTL_ACTIVE,
	.dmactl_reset =		HPC_DMACTL_RESET,
	.enet_regs =		HPC_ENET_REGS,
	.enet_regs_size =	HPC_ENET_REGS_SIZE,
	.enet_intdelay =	0,
	.enet_intdelayval =	0,
	.enetr_cbp =		HPC_ENETR_CBP,
	.enetr_ndbp =		HPC_ENETR_NDBP,
	.enetr_bc =		HPC_ENETR_BC,
	.enetr_ctl =		HPC_ENETR_CTL,
	.enetr_ctl_active =	ENETR_CTL_ACTIVE,
	.enetr_reset =		HPC_ENETR_RESET,
	.enetr_dmacfg =		HPC_ENETR_DMACFG,
	.enetr_piocfg =		HPC_ENETR_PIOCFG,
	.enetx_cbp =		HPC_ENETX_CBP,
	.enetx_ndbp =		HPC_ENETX_NDBP,
	.enetx_bc =		HPC_ENETX_BC,
	.enetx_ctl =		HPC_ENETX_CTL,
	.enetx_ctl_active =	ENETX_CTL_ACTIVE,
	.enetx_dev =		HPC_ENETX_DEV,
	.enetr_fifo =		HPC_ENETR_FIFO,
	.enetr_fifo_size =	HPC_ENETR_FIFO_SIZE,
	.enetx_fifo =		HPC_ENETX_FIFO,
	.enetx_fifo_size =	HPC_ENETX_FIFO_SIZE,
	.scsi0_devregs_size =	HPC_SCSI0_DEVREGS_SIZE,
	.scsi1_devregs_size =	HPC_SCSI1_DEVREGS_SIZE,
	.enet_devregs =		HPC_ENET_DEVREGS,
	.enet_devregs_size =	HPC_ENET_DEVREGS_SIZE,
	.pbus_fifo =		HPC_PBUS_FIFO,
	.pbus_fifo_size =	HPC_PBUS_FIFO_SIZE,
	.pbus_bbram =		HPC_PBUS_BBRAM,
	.scsi_max_xfer =	MAX_SCSI_XFER,
	.scsi_dma_segs =	(MAX_SCSI_XFER / 8192),
	.scsi_dma_segs_size =	8192,
	.clk_freq =		100,
	.dma_datain_cmd =	HPC_DMACTL_ACTIVE,
	.dma_dataout_cmd =	(HPC_DMACTL_ACTIVE | HPC_DMACTL_DIR),
	.scsi_dmactl_flush =	HPC_DMACTL_FLUSH,
	.scsi_dmactl_active =	HPC_DMACTL_ACTIVE,
	.scsi_dmactl_reset =	HPC_DMACTL_RESET
};


extern int mach_type;		/* IPxx type */
extern int mach_subtype;	/* subtype: eg., Guiness/Fullhouse for IP22 */
extern int mach_boardrev;	/* machine board revision, in case it matters */

extern struct sgimips_bus_dma_tag sgimips_default_bus_dma_tag;

static int powerintr_established;

int	hpc_match(struct device *, struct cfdata *, void *);
void	hpc_attach(struct device *, struct device *, void *);
int	hpc_print(void *, const char *);

int	hpc_submatch(struct device *, struct cfdata *, void *);

int	hpc_power_intr(void *);

#if defined(BLINK)
static struct callout hpc_blink_ch = CALLOUT_INITIALIZER;
static void	hpc_blink(void *);
#endif

CFATTACH_DECL(hpc, sizeof(struct hpc_softc),
    hpc_match, hpc_attach, NULL, NULL);

int
hpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct gio_attach_args* ga = aux;

	/* Make sure it's actually there and readable */
	if (badaddr((void*)MIPS_PHYS_TO_KSEG1(ga->ga_addr), sizeof(u_int32_t)))
		return 0;

	return 1;
}

void
hpc_attach(struct device *parent, struct device *self, void *aux)
{
	struct hpc_softc *sc = (struct hpc_softc *)self;
	struct gio_attach_args* ga = aux;
	struct hpc_attach_args ha;
	const struct hpc_device *hd;
	uint32_t hpctype;
	int sysmask;

	switch (mach_type) {
	case MACH_SGI_IP12:
		hpctype = 1;
		sysmask = HPCDEV_IP12;
		break;

	case MACH_SGI_IP20:
		hpctype = 15;
		sysmask = HPCDEV_IP20;
		break;

	case MACH_SGI_IP22:
		hpctype = 3;
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
			sysmask = HPCDEV_IP22;
		else
			sysmask = HPCDEV_IP24;
		break;

	default:
		panic("hpc_attach: can't handle HPC on an IP%d", mach_type);
	};

	/*
	 * Verify HPC1 or HPC1.5
	 *
	 * For some reason the endian register isn't mapped on all
	 * machines (HPC1 machines?).
	 */
	if (hpctype != 3 &&
	    !badaddr((void *)MIPS_PHYS_TO_KSEG1(ga->ga_addr+HPC1_BIGENDIAN),4)){
		hpctype = *(uint32_t *)
		    MIPS_PHYS_TO_KSEG1(ga->ga_addr + HPC1_BIGENDIAN);

		if (((hpctype >> HPC1_REVSHIFT) & HPC1_REVMASK) == HPC1_REV15)
			hpctype = 15;
		else
			hpctype = 1;

		/* force big-endian mode */
		*(uint32_t *)MIPS_PHYS_TO_KSEG1(ga->ga_addr + HPC1_BIGENDIAN)
		    = hpctype & 0xe0;
	}
	
	printf(": SGI HPC%d%s\n", (hpctype ==  3) ? 3 : 1,
				  (hpctype == 15) ? ".5" : "");

	sc->sc_ct = 1;
	sc->sc_ch = ga->ga_ioh;

	sc->sc_base = ga->ga_addr;

	for (hd = hpc_devices; hd->hd_name != NULL; hd++) {
		if (!(hd->hd_sysmask & sysmask))
			continue;

		ha.ha_name = hd->hd_name;
		ha.ha_devoff = hd->hd_devoff;
		ha.ha_dmaoff = hd->hd_dmaoff;
		ha.ha_irq = hd->hd_irq;

		/* XXX This is disgusting. */
		ha.ha_st = 1;
		ha.ha_sh = MIPS_PHYS_TO_KSEG1(sc->sc_base);
		ha.ha_dmat = &sgimips_default_bus_dma_tag;
		if (hpctype == 3)
			ha.hpc_regs = &hpc3_values;
		else
			ha.hpc_regs = &hpc1_values;
		ha.hpc_regs->revision = hpctype;

		(void) config_found_sm(self, &ha, hpc_print, hpc_submatch);
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
		cpu_intr_establish(9, IPL_NONE, hpc_power_intr, sc);
		powerintr_established++;
	}

#if defined(BLINK)
	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20)
		hpc_blink(sc);
#endif
}

int
hpc_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (cf->cf_loc[HPCCF_OFFSET] != HPCCF_OFFSET_DEFAULT &&
	    (bus_addr_t) cf->cf_loc[HPCCF_OFFSET] != ha->ha_devoff)
		return (0);

	return (config_match(parent, cf, aux));
}

int
hpc_print(void *aux, const char *pnp)
{
	struct hpc_attach_args *ha = aux;

	if (pnp)
		printf("%s at %s", ha->ha_name, pnp);

	printf(" offset 0x%lx", ha->ha_devoff);

	return (UNCONF);
}

int
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

#if defined(BLINK)
static void
hpc_blink(void *self)
{
	struct hpc_softc *sc = (struct hpc_softc *) self;
	register int	s;
	int	value;

	s = splhigh();

	value = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(HPC1_AUX_REGS);
	value ^= HPC1_AUX_CONSLED;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(HPC1_AUX_REGS) = value;
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

