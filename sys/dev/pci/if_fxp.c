/*	$NetBSD: if_fxp.c,v 1.33.2.2 2000/06/01 17:15:27 he Exp $	*/

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
 * Copyright (c) 1995, David Greenman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: if_fxp.c,v 1.47 1998/01/08 23:42:29 eivind Exp
 */

/*
 * Intel EtherExpress Pro/100B PCI Fast Ethernet driver
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_fxpreg.h>
#include <dev/pci/if_fxpvar.h>

/*
 * Constants for PCI (ACPI) power management manipulation.
 */

#define PCI_PMCSR_STATE_MASK	0x03
#define PCI_PMCSR_STATE_D0      0x00
#define PCI_PMCSR_STATE_D1      0x01
#define PCI_PMCSR_STATE_D2      0x02
#define PCI_PMCSR_STATE_D3      0x03

/*
 * NOTE!  On the Alpha, we have an alignment constraint.  The
 * card DMAs the packet immediately following the RFA.  However,
 * the first thing in the packet is a 14-byte Ethernet header.
 * This means that the packet is misaligned.  To compensate,
 * we actually offset the RFA 2 bytes into the cluster.  This
 * alignes the packet after the Ethernet header at a 32-bit
 * boundary.  HOWEVER!  This means that the RFA is misaligned!
 */
#define	RFA_ALIGNMENT_FUDGE	2

/*
 * Template for default configuration parameters.
 * See struct fxp_cb_config for the bit definitions.
 */
static u_int8_t fxp_cb_config_template[] = {
	0x0, 0x0,		/* cb_status */
	0x80, 0x2,		/* cb_command */
	0xff, 0xff, 0xff, 0xff,	/* link_addr */
	0x16,	/*  0 */
	0x8,	/*  1 */
	0x0,	/*  2 */
	0x0,	/*  3 */
	0x0,	/*  4 */
	0x80,	/*  5 */
	0xb2,	/*  6 */
	0x3,	/*  7 */
	0x1,	/*  8 */
	0x0,	/*  9 */
	0x26,	/* 10 */
	0x0,	/* 11 */
	0x60,	/* 12 */
	0x0,	/* 13 */
	0xf2,	/* 14 */
	0x48,	/* 15 */
	0x0,	/* 16 */
	0x40,	/* 17 */
	0xf3,	/* 18 */
	0x0,	/* 19 */
	0x3f,	/* 20 */
	0x5	/* 21 */
};

static void fxp_mii_initmedia __P((struct fxp_softc *));
static int fxp_mii_mediachange	__P((struct ifnet *));
static void fxp_mii_mediastatus	__P((struct ifnet *, struct ifmediareq *));

static void fxp_80c24_initmedia __P((struct fxp_softc *));
static int fxp_80c24_mediachange __P((struct ifnet *));
static void fxp_80c24_mediastatus __P((struct ifnet *, struct ifmediareq *));

static inline void fxp_scb_wait	__P((struct fxp_softc *));
static inline void fxp_pci_confreg_restore __P((struct fxp_softc *));
static int fxp_intr		__P((void *));
static void fxp_start		__P((struct ifnet *));
static int fxp_ioctl		__P((struct ifnet *, u_long, caddr_t));
static void fxp_init		__P((void *));
static void fxp_stop		__P((struct fxp_softc *));
static void fxp_watchdog	__P((struct ifnet *));
static int fxp_add_rfabuf	__P((struct fxp_softc *, struct fxp_rxdesc *));
static int fxp_mdi_read		__P((struct device *, int, int));
static void fxp_statchg		__P((struct device *));
static void fxp_mdi_write	__P((struct device *, int, int, int));
static void fxp_read_eeprom	__P((struct fxp_softc *, u_int16_t *,
				    int, int));
static void fxp_get_info	__P((struct fxp_softc *, u_int8_t *));
void fxp_tick			__P((void *));
static void fxp_mc_setup	__P((struct fxp_softc *));

struct fxp_phytype {
	int	fp_phy;		/* type of PHY, -1 for MII at the end. */
	void	(*fp_init) __P((struct fxp_softc *));
} fxp_phytype_table[] = {
	{ FXP_PHY_80C24,		fxp_80c24_initmedia },
	{ -1,				fxp_mii_initmedia },
};

/*
 * Set initial transmit threshold at 64 (512 bytes). This is
 * increased by 64 (512 bytes) at a time, to maximum of 192
 * (1536 bytes), if an underrun occurs.
 */
static int tx_threshold = 64;

/*
 * Wait for the previous command to be accepted (but not necessarily
 * completed).
 */
static inline void
fxp_scb_wait(sc)
	struct fxp_softc *sc;
{
	int i = 10000;

	while (CSR_READ_1(sc, FXP_CSR_SCB_COMMAND) && --i)
		DELAY(1);
	if (i == 0)
		printf("%s: WARNING: SCB timed out!\n", sc->sc_dev.dv_xname);
}

/*
 * Restore PCI configuration registers that may have been clobbered.
 * This is necessary due to bugs on the Sony VAIO Z505-series on-board
 * ethernet, after an APM suspend/resume.  Ideally this function would
 * be called from a power-hook after APM resume, but no such hook
 * exists at this time, so instead we call it when the driver detects
 * something awry.
 */

static inline void
fxp_pci_confreg_restore(sc)
        struct fxp_softc *sc;
{
	pcireg_t reg;
	int flag;

	/*
	 * Check to see if the command register is blank -- if so, then
	 * we'll assume that all the clobberable-registers have been
	 * clobbered.
	 */
	flag = ((reg = pci_conf_read(sc->sc_pc, sc->sc_tag,
	    PCI_COMMAND_STATUS_REG)) & 0xffff) == 0;

	/*
	 * In general, the above metric is accurate. Unfortunately,
	 * it is inaccurate across a hibernation. Ideally APM/ACPI
	 * code should take note of hibernation events and execute
	 * a hibernation wakeup hook, but that isn't here at this time.
	 */
	flag |= 1;

	if (flag) {
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_COMMAND_STATUS_REG,
		    (reg & 0xffff0000) |
		    (sc->sc_regs[PCI_COMMAND_STATUS_REG>>2] & 0xffff));
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_BHLC_REG,
		    sc->sc_regs[PCI_BHLC_REG>>2]);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_MAPREG_START+0x0,
		    sc->sc_regs[(PCI_MAPREG_START+0x0)>>2]);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_MAPREG_START+0x4,
		    sc->sc_regs[(PCI_MAPREG_START+0x4)>>2]);
		pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_MAPREG_START+0x8,
		    sc->sc_regs[(PCI_MAPREG_START+0x8)>>2]);
	}
}

static int fxp_match __P((struct device *, struct cfdata *, void *));
static void fxp_attach __P((struct device *, struct device *, void *));

static void	fxp_shutdown __P((void *));

struct cfattach fxp_ca = {
	sizeof(struct fxp_softc), fxp_match, fxp_attach
};

/*
 * Check if a device is an 82557.
 */
static int
fxp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INTEL_82557:
		return (1);
	}

	return (0);
}

static void
fxp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fxp_softc *sc = (struct fxp_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	u_int8_t enaddr[6];
	struct ifnet *ifp;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_dma_segment_t seg;
	int ioh_valid, memh_valid;
	bus_addr_t addr;
	bus_size_t size;
	int flags, rseg, i, error, attach_stage;
	struct fxp_phytype *fp;
	int pci_pwrmgmt_cap_reg, pci_pwrmgmt_csr_reg;

	/*
	 * Map control/status registers.
	 */
	ioh_valid = (pci_mapreg_map(pa, FXP_PCI_IOBA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);

	/*
	 * Version 2.1 of the PCI spec, page 196, "Address Maps":
	 *
	 *	Prefetchable
	 *
	 *	Set to one if there are no side effects on reads, the
	 *	device returns all bytes regardless of the byte enables,
	 *	and host bridges can merge processor writes into this
	 *	range without causing errors.  Bit must be set to zero
	 *	otherwise.
	 *
	 * The 82557 incorrectly sets the "prefetchable" bit, resulting
	 * in errors on systems which will do merged reads and writes.
	 * These errors manifest themselves as all-bits-set when reading
	 * from the EEPROM or other < 4 byte registers.
	 *
	 * We must work around this problem by always forcing the mapping
	 * for memory space to be uncacheable.  On systems which cannot
	 * create an uncacheable mapping (because the firmware mapped it
	 * into only cacheable/prefetchable space due to the "prefetchable"
	 * bit), we can fall back onto i/o mapped access.
	 */
	memh_valid = 0;
	memt = pa->pa_memt;
	if (((pa->pa_flags & PCI_FLAGS_MEM_ENABLED) != 0) &&
	    pci_mapreg_info(pa->pa_pc, pa->pa_tag, FXP_PCI_MMBA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT,
	    &addr, &size, &flags) == 0) {
		flags &= ~BUS_SPACE_MAP_CACHEABLE;
		if (bus_space_map(memt, addr, size, flags, &memh) == 0)
			memh_valid = 1;
	}

	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else if (ioh_valid) {
		sc->sc_st = iot;
		sc->sc_sh = ioh;
	} else {
		printf(": unable to map device registers\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	printf(": Intel EtherExpress Pro 10+/100B Ethernet\n");

	/* Make sure bus-mastering is enabled. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/*
	 * Under some circumstances (such as APM suspend/resume
	 * cycles, and across ACPI power state changes), the
	 * i82257-family can lose the contents of critical PCI
	 * configuration registers, causing the card to be
	 * non-responsive and useless.  This occurs on the Sony VAIO
	 * Z505-series, among others.  Preserve them here so they can
	 * be later restored (by fxp_pci_confreg_restore()).
	 */
	sc->sc_pc = pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_regs[PCI_COMMAND_STATUS_REG>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	sc->sc_regs[PCI_BHLC_REG>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_BHLC_REG);
	sc->sc_regs[(PCI_MAPREG_START+0x0)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x0);
	sc->sc_regs[(PCI_MAPREG_START+0x4)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x4);
	sc->sc_regs[(PCI_MAPREG_START+0x8)>>2] =
	    pci_conf_read(pc, pa->pa_tag, PCI_MAPREG_START+0x8);

	/*
	 * Work around BIOS ACPI bugs where the chip is inadvertantly
	 * left in ACPI D3 (lowest power state).  First confirm the device
	 * supports ACPI power management, then move it to the D0 (fully
	 * functional) state if it is not already there.
	 */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT,
	    &pci_pwrmgmt_cap_reg, 0)) {
		pcireg_t reg;

		pci_pwrmgmt_csr_reg = pci_pwrmgmt_cap_reg + 4;
		reg = pci_conf_read(pc, pa->pa_tag, pci_pwrmgmt_csr_reg);
		if ((reg & PCI_PMCSR_STATE_MASK) != PCI_PMCSR_STATE_D0) {
		    pci_conf_write(pc, pa->pa_tag, pci_pwrmgmt_csr_reg,
			(reg & ~PCI_PMCSR_STATE_MASK) |
			PCI_PMCSR_STATE_D0);
		}
	}
	/* Restore PCI configuration registers. */
	fxp_pci_confreg_restore(sc);

	/*
	 * Allocate our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, fxp_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	attach_stage = 0;

	/*
	 * Allocate the control data, and create and load the DMA
	 * map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct fxp_control_data), NBPG, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 1;

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct fxp_control_data), (caddr_t *)&sc->control_data,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: can't map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}
	bzero(sc->control_data, sizeof(struct fxp_control_data));

	attach_stage = 2;

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct fxp_control_data), 1,
	    sizeof(struct fxp_control_data), 0, BUS_DMA_NOWAIT,
	    &sc->sc_dmamap)) != 0) {
		printf("%s: can't create control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 3;

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
	    sc->control_data, sizeof(struct fxp_control_data), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 4;

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < FXP_NTXCB; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    FXP_NTXSEG, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_tx_dmamaps[i])) != 0) {
			printf("%s: can't create tx DMA map %d, error = %d\n",
			    sc->sc_dev.dv_xname, i, error);
			goto fail;
		}
	}

	attach_stage = 5;

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < FXP_NRFABUFS; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &sc->sc_rx_dmamaps[i])) != 0) {
			printf("%s: can't create rx DMA map %d, error = %d\n",
			    sc->sc_dev.dv_xname, i, error);
			goto fail;
		}
	}

	attach_stage = 6;

	/*
	 * Pre-allocate the receive buffer descriptors and the buffers
	 * themselves.
	 */
	sc->sc_rxdescs = malloc(sizeof(struct fxp_rxdesc) * FXP_NRFABUFS,
	    M_DEVBUF, M_NOWAIT);
	if (sc->sc_rxdescs == NULL) {
		printf("%s: can't allocate rx buffer descriptors\n",
		    sc->sc_dev.dv_xname);
		goto fail;
	}
	bzero(sc->sc_rxdescs, sizeof(struct fxp_rxdesc) * FXP_NRFABUFS);

	attach_stage = 7;

	for (i = 0; i < FXP_NRFABUFS; i++) {
		sc->sc_rxdescs[i].fr_dmamap = sc->sc_rx_dmamaps[i];
		if (fxp_add_rfabuf(sc, &sc->sc_rxdescs[i]) != 0) {
			printf("%s: can't allocate or map rx buffers\n",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
	}

	attach_stage = 8;

	/* Initialize MAC address and media structures. */
	fxp_get_info(sc, enaddr);

	printf("%s: Ethernet address %s%s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr), sc->phy_10Mbps_only ? ", 10Mbps" : "");

	ifp = &sc->sc_ethercom.ec_if;

	/*
	 * Get info about our media interface, and initialize it.  Note
	 * the table terminates itself with a phy of -1, indicating
	 * that we're using MII.
	 */
	for (fp = fxp_phytype_table; fp->fp_phy != -1; fp++)
		if (fp->fp_phy == sc->phy_primary_device)
			break;
	(*fp->fp_init)(sc);

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = fxp_ioctl;
	ifp->if_start = fxp_start;
	ifp->if_watchdog = fxp_watchdog;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	/*
	 * Let the system queue as many packets as we have TX descriptors.
	 */
	ifp->if_snd.ifq_maxlen = FXP_NTXCB;
	ether_ifattach(ifp, enaddr);
#if NBPFILTER > 0
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#endif

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_NET, 0);
#endif

	/*
	 * Add shutdown hook so that DMA is disabled prior to reboot. Not
	 * doing do could allow DMA to corrupt kernel memory during the
	 * reboot before the driver initializes.
	 */
	shutdownhook_establish(fxp_shutdown, sc);
	return;

 fail:
	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall though.
	 */
	switch (attach_stage) {
	case 8:
	    {
		struct fxp_rxdesc *rxd;

		for (i = 0; i < FXP_NRFABUFS; i++) {
			rxd = &sc->sc_rxdescs[i];
			if (rxd->fr_mbhead != NULL) {
				bus_dmamap_unload(sc->sc_dmat, rxd->fr_dmamap);
				m_freem(rxd->fr_mbhead);
			}
		}
	    }
		/* FALLTHROUGH */

	case 7:
		free(sc->sc_rxdescs, M_DEVBUF);
		/* FALLTHROUGH */

	case 6:
		for (i = 0; i < FXP_NRFABUFS; i++)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_rx_dmamaps[i]);
		/* FALLTHROUGH */

	case 5:
		for (i = 0; i < FXP_NTXCB; i++)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_tx_dmamaps[i]);
		/* FALLTHROUGH */

	case 4:
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
		/* FALLTHROUGH */

	case 3:
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmamap);
		/* FALLTHROUGH */

	case 2:
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->control_data,
		    sizeof(struct fxp_control_data));
		/* FALLTHROUGH */

	case 1:
		bus_dmamem_free(sc->sc_dmat, &seg, rseg);
		break;
	}
}

void
fxp_mii_initmedia(sc)
	struct fxp_softc *sc;
{

	sc->sc_mii.mii_ifp = &sc->sc_ethercom.ec_if;
	sc->sc_mii.mii_readreg = fxp_mdi_read;
	sc->sc_mii.mii_writereg = fxp_mdi_write;
	sc->sc_mii.mii_statchg = fxp_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, fxp_mii_mediachange,
	    fxp_mii_mediastatus);
	mii_phy_probe(&sc->sc_dev, &sc->sc_mii, 0xffffffff);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
}

void
fxp_80c24_initmedia(sc)
	struct fxp_softc *sc;
{

	/*
	 * The Seeq 80c24 AutoDUPLEX(tm) Ethernet Interface Adapter
	 * doesn't have a programming interface of any sort.  The
	 * media is sensed automatically based on how the link partner
	 * is configured.  This is, in essence, manual configuration.
	 */
	printf("%s: Seeq 80c24 AutoDUPLEX media interface present\n",
	    sc->sc_dev.dv_xname);
	ifmedia_init(&sc->sc_mii.mii_media, 0, fxp_80c24_mediachange,
	    fxp_80c24_mediastatus);
	ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
}

/*
 * Device shutdown routine. Called at system shutdown after sync. The
 * main purpose of this routine is to shut off receiver DMA so that
 * kernel memory doesn't get clobbered during warmboot.
 */
static void
fxp_shutdown(sc)
	void *sc;
{

	fxp_stop((struct fxp_softc *) sc);
}

/*
 * Initialize the interface media.
 */
static void
fxp_get_info(sc, enaddr)
	struct fxp_softc *sc;
	u_int8_t *enaddr;
{
	u_int16_t data, myea[3];

	/*
	 * Reset to a stable state.
	 */
	CSR_WRITE_4(sc, FXP_CSR_PORT, FXP_PORT_SELECTIVE_RESET);
	DELAY(10);

	/*
	 * Get info about the primary PHY
	 */
	fxp_read_eeprom(sc, &data, 6, 1);
	sc->phy_primary_addr = data & 0xff;
	sc->phy_primary_device = (data >> 8) & 0x3f;
	sc->phy_10Mbps_only = data >> 15;

	/*
	 * Read MAC address.
	 */
	fxp_read_eeprom(sc, myea, 0, 3);
	bcopy(myea, enaddr, ETHER_ADDR_LEN);
}

/*
 * Read from the serial EEPROM. Basically, you manually shift in
 * the read opcode (one bit at a time) and then shift in the address,
 * and then you shift out the data (all of this one bit at a time).
 * The word size is 16 bits, so you have to provide the address for
 * every 16 bits of data.
 */
static void
fxp_read_eeprom(sc, data, offset, words)
	struct fxp_softc *sc;
	u_int16_t *data;
	int offset;
	int words;
{
	u_int16_t reg;
	int i, x;

	for (i = 0; i < words; i++) {
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		/*
		 * Shift in read opcode.
		 */
		for (x = 3; x > 0; x--) {
			if (FXP_EEPROM_OPC_READ & (1 << (x - 1))) {
				reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
			} else {
				reg = FXP_EEPROM_EECS;
			}
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			DELAY(1);
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}
		/*
		 * Shift in address.
		 */
		for (x = 6; x > 0; x--) {
			if ((i + offset) & (1 << (x - 1))) {
				reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
			} else {
				reg = FXP_EEPROM_EECS;
			}
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			DELAY(1);
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}
		reg = FXP_EEPROM_EECS;
		data[i] = 0;
		/*
		 * Shift out data.
		 */
		for (x = 16; x > 0; x--) {
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
			    reg | FXP_EEPROM_EESK);
			DELAY(1);
			if (CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
			    FXP_EEPROM_EEDO)
				data[i] |= (1 << (x - 1));
			CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
			DELAY(1);
		}
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
		DELAY(1);
	}
}

/*
 * Start packet transmission on the interface.
 */
static void
fxp_start(ifp)
	struct ifnet *ifp;
{
	struct fxp_softc *sc = ifp->if_softc;
	struct fxp_cb_tx *txp;
	bus_dmamap_t dmamap;
	int old_queued;

	/*
	 * See if we need to suspend xmit until the multicast filter
	 * has been reprogrammed (which can only be done at the head
	 * of the command chain).
	 */
	if (sc->need_mcsetup || (old_queued = sc->tx_queued) >= FXP_NTXCB) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	/*
	 * We're finished if there is nothing more to add to the list or if
	 * we're all filled up with buffers to transmit.
	 */
	while (ifp->if_snd.ifq_head != NULL && sc->tx_queued < FXP_NTXCB) {
		struct mbuf *mb_head;
		int segment, error;

		/*
		 * Grab a packet to transmit.
		 */
		IF_DEQUEUE(&ifp->if_snd, mb_head);

		/*
		 * Get pointer to next available tx desc.
		 */
		txp = sc->cbl_last->cb_soft.next;
		dmamap = txp->cb_soft.dmamap;

		/*
		 * Go through each of the mbufs in the chain and initialize
		 * the transmit buffer descriptors with the physical address
		 * and size of the mbuf.
		 */
 tbdinit:
		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
		    mb_head, BUS_DMA_NOWAIT);
		switch (error) {
		case 0:
			/* Success. */
			break;

		case EFBIG:
		    {
			struct mbuf *mn;

			/*
			 * We ran out of segments.  We have to recopy this
			 * mbuf chain first.  Bail out if we can't get the
			 * new buffers.
			 */
			printf("%s: too many segments, ", sc->sc_dev.dv_xname);

			MGETHDR(mn, M_DONTWAIT, MT_DATA);
			if (mn == NULL) {
				m_freem(mb_head);
				printf("aborting\n");
				goto out;
			}
			if (mb_head->m_pkthdr.len > MHLEN) {
				MCLGET(mn, M_DONTWAIT);
				if ((mn->m_flags & M_EXT) == 0) {
					m_freem(mn);
					m_freem(mb_head);
					printf("aborting\n");
					goto out;
				}
			}
			m_copydata(mb_head, 0, mb_head->m_pkthdr.len,
			    mtod(mn, caddr_t));
			mn->m_pkthdr.len = mn->m_len = mb_head->m_pkthdr.len;
			m_freem(mb_head);
			mb_head = mn;
			printf("retrying\n");
			goto tbdinit;
		    }

		default:
			/*
			 * Some other problem; report it.
			 */
			printf("%s: can't load mbuf chain, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			m_freem(mb_head);
			goto out;
		}

		for (segment = 0; segment < dmamap->dm_nsegs; segment++) {
			txp->tbd[segment].tb_addr =
			    dmamap->dm_segs[segment].ds_addr;
			txp->tbd[segment].tb_size =
			    dmamap->dm_segs[segment].ds_len;
		}

		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		txp->tbd_number = dmamap->dm_nsegs;
		txp->cb_soft.mb_head = mb_head;
		txp->cb_status = 0;
		txp->cb_command =
		    FXP_CB_COMMAND_XMIT | FXP_CB_COMMAND_SF | FXP_CB_COMMAND_S;
		txp->tx_threshold = tx_threshold;
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    FXP_TXDESCOFF(sc, txp), FXP_TXDESCSIZE,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	
		/*
		 * Advance the end of list forward.
		 */
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    FXP_TXDESCOFF(sc, sc->cbl_last), FXP_TXDESCSIZE,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		sc->cbl_last->cb_command &= ~FXP_CB_COMMAND_S;
		sc->cbl_last = txp;
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    FXP_TXDESCOFF(sc, sc->cbl_last), FXP_TXDESCSIZE,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); 

		/*
		 * Advance the beginning of the list forward if there are
		 * no other packets queued (when nothing is queued, cbl_first
		 * sits on the last TxCB that was sent out).
		 */
		if (sc->tx_queued == 0)
			sc->cbl_first = txp;

		sc->tx_queued++;

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, mb_head);
#endif
	}

 out:
	/*
	 * We're finished. If we added to the list, issue a RESUME to get DMA
	 * going again if suspended.
	 */
	if (old_queued != sc->tx_queued) {
		fxp_scb_wait(sc);
		CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_RESUME);

		/*
		 * Set a 5 second timer just in case we don't hear from the
		 * card again.
		 */
		ifp->if_timer = 5;
	}
}

/*
 * Process interface interrupts.
 */
static int
fxp_intr(arg)
	void *arg;
{
	struct fxp_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_if;
	u_int8_t statack;
	int claimed = 0;

	while ((statack = CSR_READ_1(sc, FXP_CSR_SCB_STATACK)) != 0) {
		claimed = 1;

		/*
		 * First ACK all the interrupts in this pass.
		 */
		CSR_WRITE_1(sc, FXP_CSR_SCB_STATACK, statack);

		/*
		 * Process receiver interrupts. If a no-resource (RNR)
		 * condition exists, get whatever packets we can and
		 * re-start the receiver.
		 */
		if (statack & (FXP_SCB_STATACK_FR | FXP_SCB_STATACK_RNR)) {
			struct fxp_rxdesc *rxd;
			struct mbuf *m;
			struct fxp_rfa *rfa;
			bus_dmamap_t rxmap;
 rcvloop:
			rxd = sc->rfa_head;
			rxmap = rxd->fr_dmamap;
			m = rxd->fr_mbhead;
			rfa = (struct fxp_rfa *)(m->m_ext.ext_buf +
			    RFA_ALIGNMENT_FUDGE);

			bus_dmamap_sync(sc->sc_dmat, rxmap, 0,
			    rxmap->dm_mapsize,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

			if (rfa->rfa_status & FXP_RFA_STATUS_C) {
				/*
				 * Remove first packet from the chain.
				 */
				sc->rfa_head = rxd->fr_next;
				rxd->fr_next = NULL;

				/*
				 * Add a new buffer to the receive chain.
				 * If this fails, the old buffer is recycled
				 * instead.
				 */
				if (fxp_add_rfabuf(sc, rxd) == 0) {
					struct ether_header *eh;
					u_int16_t total_len;

					total_len = rfa->actual_size &
					    (MCLBYTES - 1);
					if (total_len <
					    sizeof(struct ether_header)) {
						m_freem(m);
						goto rcvloop;
					}
					m->m_pkthdr.rcvif = ifp;
					m->m_pkthdr.len = m->m_len =
					    total_len -
					    sizeof(struct ether_header);
					eh = mtod(m, struct ether_header *);
#if NBPFILTER > 0
					if (ifp->if_bpf) {
						bpf_tap(ifp->if_bpf,
						    mtod(m, caddr_t),
						    total_len); 
						/*
						 * Only pass this packet up
						 * if it is for us.
						 */
						if ((ifp->if_flags &
						    IFF_PROMISC) &&
						    (rfa->rfa_status &
						    FXP_RFA_STATUS_IAMATCH) &&
						    (eh->ether_dhost[0] & 1)
						    == 0) {
							m_freem(m);
							goto rcvloop;
						}
					}
#endif /* NBPFILTER > 0 */
					m->m_data +=
					    sizeof(struct ether_header);
					ether_input(ifp, eh, m);
				}
				goto rcvloop;
			}
			if (statack & FXP_SCB_STATACK_RNR) {
				fxp_scb_wait(sc);
				CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
				    rxmap->dm_segs[0].ds_addr +
				    RFA_ALIGNMENT_FUDGE);
				CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND,
				    FXP_SCB_COMMAND_RU_START);
			}
		}
		/*
		 * Free any finished transmit mbuf chains.
		 */
		if (statack & FXP_SCB_STATACK_CNA) {
			struct fxp_cb_tx *txp;
			bus_dmamap_t txmap;

			for (txp = sc->cbl_first; sc->tx_queued;
			    txp = txp->cb_soft.next) {
				bus_dmamap_sync(sc->sc_dmat,
				    sc->sc_dmamap, FXP_TXDESCOFF(sc, txp),
				    FXP_TXDESCSIZE,
				    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
				if ((txp->cb_status & FXP_CB_STATUS_C) == 0)
					break;
				if (txp->cb_soft.mb_head != NULL) {
					txmap = txp->cb_soft.dmamap;
					bus_dmamap_sync(sc->sc_dmat, txmap,
					    0, txmap->dm_mapsize,
					    BUS_DMASYNC_POSTWRITE);
					bus_dmamap_unload(sc->sc_dmat, txmap);
					m_freem(txp->cb_soft.mb_head);
					txp->cb_soft.mb_head = NULL;
				}
				sc->tx_queued--;
			}
			sc->cbl_first = txp;
			ifp->if_flags &= ~IFF_OACTIVE;
			if (sc->tx_queued == 0) {
				ifp->if_timer = 0;
				if (sc->need_mcsetup)
					fxp_mc_setup(sc);
			}
			/*
			 * Try to start more packets transmitting.
			 */
			if (ifp->if_snd.ifq_head != NULL)
				fxp_start(ifp);
		}
	}

#if NRND > 0
	if (claimed)
		rnd_add_uint32(&sc->rnd_source, statack);
#endif
	return (claimed);
}

/*
 * Update packet in/out/collision statistics. The i82557 doesn't
 * allow you to access these counters without doing a fairly
 * expensive DMA to get _all_ of the statistics it maintains, so
 * we do this operation here only once per second. The statistics
 * counters in the kernel are updated from the previous dump-stats
 * DMA and then a new dump-stats DMA is started. The on-chip
 * counters are zeroed when the DMA completes. If we can't start
 * the DMA immediately, we don't wait - we just prepare to read
 * them again next time.
 */
void
fxp_tick(arg)
	void *arg;
{
	struct fxp_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_if;
	struct fxp_stats *sp = &sc->control_data->fcd_stats;
	int s = splnet();

	ifp->if_opackets += sp->tx_good;
	ifp->if_collisions += sp->tx_total_collisions;
	if (sp->rx_good) {
		ifp->if_ipackets += sp->rx_good;
		sc->rx_idle_secs = 0;
	} else {
		sc->rx_idle_secs++;
	}
	ifp->if_ierrors +=
	    sp->rx_crc_errors +
	    sp->rx_alignment_errors +
	    sp->rx_rnr_errors +
	    sp->rx_overrun_errors;
	/*
	 * If any transmit underruns occured, bump up the transmit
	 * threshold by another 512 bytes (64 * 8).
	 */
	if (sp->tx_underruns) {
		ifp->if_oerrors += sp->tx_underruns;
		if (tx_threshold < 192)
			tx_threshold += 64;
	}

	/*
	 * If we haven't received any packets in FXP_MAC_RX_IDLE seconds,
	 * then assume the receiver has locked up and attempt to clear
	 * the condition by reprogramming the multicast filter. This is
	 * a work-around for a bug in the 82557 where the receiver locks
	 * up if it gets certain types of garbage in the syncronization
	 * bits prior to the packet header. This bug is supposed to only
	 * occur in 10Mbps mode, but has been seen to occur in 100Mbps
	 * mode as well (perhaps due to a 10/100 speed transition).
	 */
	if (sc->rx_idle_secs > FXP_MAX_RX_IDLE) {
		sc->rx_idle_secs = 0;
		fxp_mc_setup(sc);
	}
	/*
	 * If there is no pending command, start another stats
	 * dump. Otherwise punt for now.
	 */
	if (CSR_READ_1(sc, FXP_CSR_SCB_COMMAND) == 0) {
		/*
		 * Start another stats dump.
		 */
		CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND,
		    FXP_SCB_COMMAND_CU_DUMPRESET);
	} else {
		/*
		 * A previous command is still waiting to be accepted.
		 * Just zero our copy of the stats and wait for the
		 * next timer event to update them.
		 */
		sp->tx_good = 0;
		sp->tx_underruns = 0;
		sp->tx_total_collisions = 0;

		sp->rx_good = 0;
		sp->rx_crc_errors = 0;
		sp->rx_alignment_errors = 0;
		sp->rx_rnr_errors = 0;
		sp->rx_overrun_errors = 0;
	}

	/* Tick the MII clock. */
	mii_tick(&sc->sc_mii);
	splx(s);

	/*
	 * Schedule another timeout one second from now.
	 */
	timeout(fxp_tick, sc, hz);
}

/*
 * Stop the interface. Cancels the statistics updater and resets
 * the interface.
 */
static void
fxp_stop(sc)
	struct fxp_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	struct fxp_rxdesc *rxd;
	struct fxp_cb_tx *txp;
	int i;

	/*
	 * Cancel stats updater.
	 */
	untimeout(fxp_tick, sc);

	/*
	 * Issue software reset
	 */
	CSR_WRITE_4(sc, FXP_CSR_PORT, FXP_PORT_SELECTIVE_RESET);
	DELAY(10);

	/*
	 * Release any xmit buffers.
	 */
	for (txp = sc->control_data->fcd_txcbs, i = 0; i < FXP_NTXCB; i++) {
		if (txp[i].cb_soft.mb_head != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txp[i].cb_soft.dmamap);
			m_freem(txp[i].cb_soft.mb_head);
			txp[i].cb_soft.mb_head = NULL;
		}
	}
	sc->tx_queued = 0;

	/*
	 * Free all the receive buffers then reallocate/reinitialize
	 */
	sc->rfa_head = NULL;
	sc->rfa_tail = NULL;
	for (i = 0; i < FXP_NRFABUFS; i++) {
		rxd = &sc->sc_rxdescs[i];
		if (rxd->fr_mbhead != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxd->fr_dmamap);
			m_freem(rxd->fr_mbhead);
			rxd->fr_mbhead = NULL;
		}
		if (fxp_add_rfabuf(sc, rxd) != 0) {
			/*
			 * This "can't happen" - we're at splnet()
			 * and we just freed the buffer we need
			 * above.
			 */
			panic("fxp_stop: no buffers!");
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/*
 * Watchdog/transmission transmit timeout handler. Called when a
 * transmission is started on the interface, but no interrupt is
 * received before the timeout. This usually indicates that the
 * card has wedged for some reason.
 */
static void
fxp_watchdog(ifp)
	struct ifnet *ifp;
{
	struct fxp_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;

	fxp_init(sc);
}

static void
fxp_init(xsc)
	void *xsc;
{
	struct fxp_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_if;
	struct fxp_cb_config *cbp;
	struct fxp_cb_ias *cb_ias;
	struct fxp_cb_tx *txp;
	int i, s, prm, error;

	/* Restore PCI configuration registers. */
	fxp_pci_confreg_restore(sc);

	s = splnet();
	/*
	 * Cancel any pending I/O
	 */
	fxp_stop(sc);

	prm = (ifp->if_flags & IFF_PROMISC) ? 1 : 0;
	sc->promisc_mode = prm;

	/*
	 * Initialize base of CBL and RFA memory. Loading with zero
	 * sets it up for regular linear addressing.
	 */
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, 0);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_BASE);

	fxp_scb_wait(sc);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_BASE);

	/*
	 * Initialize base of dump-stats buffer.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    sc->sc_cddma + FXP_CDOFF(fcd_stats));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_DUMP_ADR);

	/*
	 * We temporarily use memory that contains the TxCB list to
	 * construct the config CB. The TxCB list memory is rebuilt
	 * later.
	 */
	cbp = (struct fxp_cb_config *) sc->control_data->fcd_txcbs;

	/*
	 * This bcopy is kind of disgusting, but there are a bunch of must be
	 * zero and must be one bits in this structure and this is the easiest
	 * way to initialize them all to proper values.
	 */
	bcopy(fxp_cb_config_template, (void *)&cbp->cb_status,
		sizeof(fxp_cb_config_template));

	cbp->cb_status =	0;
	cbp->cb_command =	FXP_CB_COMMAND_CONFIG | FXP_CB_COMMAND_EL;
	cbp->link_addr =	-1;	/* (no) next command */
	cbp->byte_count =	22;	/* (22) bytes to config */
	cbp->rx_fifo_limit =	8;	/* rx fifo threshold (32 bytes) */
	cbp->tx_fifo_limit =	0;	/* tx fifo threshold (0 bytes) */
	cbp->adaptive_ifs =	0;	/* (no) adaptive interframe spacing */
	cbp->rx_dma_bytecount =	0;	/* (no) rx DMA max */
	cbp->tx_dma_bytecount =	0;	/* (no) tx DMA max */
	cbp->dma_bce =		0;	/* (disable) dma max counters */
	cbp->late_scb =		0;	/* (don't) defer SCB update */
	cbp->tno_int =		0;	/* (disable) tx not okay interrupt */
	cbp->ci_int =		0;	/* interrupt on CU not active */
	cbp->save_bf =		prm;	/* save bad frames */
	cbp->disc_short_rx =	!prm;	/* discard short packets */
	cbp->underrun_retry =	1;	/* retry mode (1) on DMA underrun */
	cbp->mediatype =	!sc->phy_10Mbps_only; /* interface mode */
	cbp->nsai =		1;	/* (don't) disable source addr insert */
	cbp->preamble_length =	2;	/* (7 byte) preamble */
	cbp->loopback =		0;	/* (don't) loopback */
	cbp->linear_priority =	0;	/* (normal CSMA/CD operation) */
	cbp->linear_pri_mode =	0;	/* (wait after xmit only) */
	cbp->interfrm_spacing =	6;	/* (96 bits of) interframe spacing */
	cbp->promiscuous =	prm;	/* promiscuous mode */
	cbp->bcast_disable =	0;	/* (don't) disable broadcasts */
	cbp->crscdt =		0;	/* (CRS only) */
	cbp->stripping =	!prm;	/* truncate rx packet to byte count */
	cbp->padding =		1;	/* (do) pad short tx packets */
	cbp->rcv_crc_xfer =	0;	/* (don't) xfer CRC to host */
	cbp->force_fdx =	0;	/* (don't) force full duplex */
	cbp->fdx_pin_en =	1;	/* (enable) FDX# pin */
	cbp->multi_ia =		0;	/* (don't) accept multiple IAs */
	cbp->mc_all =		sc->all_mcasts;/* accept all multicasts */

	/* Load the DMA map */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_tx_dmamaps[0], cbp,
	    sizeof(struct fxp_cb_config), NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load config buffer, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
	    0, sizeof(struct fxp_cb_config),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Start the config command/DMA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    sc->sc_cddma + FXP_CDOFF(fcd_txcbs[0].cb_status));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);
	/* ...and wait for it to complete. */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
	    0, sizeof(struct fxp_cb_config),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	i = 1000;
	while (!(cbp->cb_status & FXP_CB_STATUS_C) && --i) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
		    0, sizeof(struct fxp_cb_config),
		    BUS_DMASYNC_POSTREAD);
		DELAY(1);
	}
	if (i == 0) {
		printf("%s at line %d: dmasync timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return;
	}

	/* Unload the DMA map */
	bus_dmamap_unload(sc->sc_dmat, sc->sc_tx_dmamaps[0]);

	/*
	 * Now initialize the station address. Temporarily use the TxCB
	 * memory area like we did above for the config CB.
	 */
	cb_ias = (struct fxp_cb_ias *) sc->control_data->fcd_txcbs;
	cb_ias->cb_status = 0;
	cb_ias->cb_command = FXP_CB_COMMAND_IAS | FXP_CB_COMMAND_EL;
	cb_ias->link_addr = -1;
	bcopy(LLADDR(ifp->if_sadl), (void *)cb_ias->macaddr, 6);

	/* Load the DMA map */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_tx_dmamaps[0], cbp, 
	    sizeof(struct fxp_cb_ias), NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load address buffer, error = %d\n",
		    sc->sc_dev .dv_xname, error);
		return;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
		    0, sizeof(struct fxp_cb_ias),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Start the IAS (Individual Address Setup) command/DMA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);
	/* ...and wait for it to complete. */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
	    0, sizeof(struct fxp_cb_ias),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	i = 1000;
	while (!(cb_ias->cb_status & FXP_CB_STATUS_C) && --i) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dmamaps[0],
		    0, sizeof(struct fxp_cb_ias),
		    BUS_DMASYNC_POSTREAD);
		DELAY(1);
	}
	if (i == 0) {
		printf("%s at line %d: dmasync timeout\n",
		    sc->sc_dev.dv_xname, __LINE__);
		return;
	}

	/* Unload the DMA map */
	bus_dmamap_unload(sc->sc_dmat, sc->sc_tx_dmamaps[0]);

	/*
	 * Initialize transmit control block (TxCB) list.
	 */

	txp = sc->control_data->fcd_txcbs;
	bzero(txp, sizeof(sc->control_data->fcd_txcbs));
	for (i = 0; i < FXP_NTXCB; i++) {
		txp[i].cb_status = FXP_CB_STATUS_C | FXP_CB_STATUS_OK;
		txp[i].cb_command = FXP_CB_COMMAND_NOP;
		txp[i].link_addr = sc->sc_cddma +
		    FXP_CDOFF(fcd_txcbs[(i + 1) & FXP_TXCB_MASK].cb_status);
		txp[i].tbd_array_addr = sc->sc_cddma +
		    FXP_CDOFF(fcd_txcbs[i].tbd[0]);
		txp[i].cb_soft.dmamap = sc->sc_tx_dmamaps[i];
		txp[i].cb_soft.next = &txp[(i + 1) & FXP_TXCB_MASK];
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
		    FXP_TXDESCOFF(sc, &txp[i]), FXP_TXDESCSIZE,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
	/*
	 * Set the suspend flag on the first TxCB and start the control
	 * unit. It will execute the NOP and then suspend.
	 */
	txp->cb_command = FXP_CB_COMMAND_NOP | FXP_CB_COMMAND_S;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
	    FXP_TXDESCOFF(sc, txp), FXP_TXDESCSIZE,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	sc->cbl_first = sc->cbl_last = txp;
	sc->tx_queued = 1;

	fxp_scb_wait(sc);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);

	/*
	 * Initialize receiver buffer area - RFA.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    sc->rfa_head->fr_dmamap->dm_segs[0].ds_addr + RFA_ALIGNMENT_FUDGE);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_START);

	/*
	 * Set current media.
	 */
	mii_mediachg(&sc->sc_mii);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);

	/*
	 * Start stats updater.
	 */
	timeout(fxp_tick, sc, hz);
}

/*
 * Change media according to request.
 */
int
fxp_mii_mediachange(ifp)
	struct ifnet *ifp;
{
	struct fxp_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return (0);
}

/*
 * Notify the world which media we're using.
 */
void
fxp_mii_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct fxp_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

int
fxp_80c24_mediachange(ifp)
	struct ifnet *ifp;
{

	/* Nothing to do here. */
	return (0);
}

void
fxp_80c24_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct fxp_softc *sc = ifp->if_softc;

	/*
	 * Media is currently-selected media.  We cannot determine
	 * the link status.
	 */
	ifmr->ifm_status = 0;
	ifmr->ifm_active = sc->sc_mii.mii_media.ifm_cur->ifm_media;
}

/*
 * Add a buffer to the end of the RFA buffer list.
 * Return 0 if successful, 1 for failure. A failure results in
 * adding the 'oldm' (if non-NULL) on to the end of the list -
 * tossing out it's old contents and recycling it.
 * The RFA struct is stuck at the beginning of mbuf cluster and the
 * data pointer is fixed up to point just past it.
 */
static int
fxp_add_rfabuf(sc, rxd)
	struct fxp_softc *sc;
	struct fxp_rxdesc *rxd;
{
	struct mbuf *m, *oldm;
	struct fxp_rfa *rfa, *p_rfa;
	bus_dmamap_t rxmap;
	u_int32_t v;
	int error, rval = 0;

	oldm = rxd->fr_mbhead;
	rxmap = rxd->fr_dmamap;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			if (oldm == NULL)
				return 1;
			m = oldm;
			m->m_data = m->m_ext.ext_buf;
			rval = 1;
		}
	} else {
		if (oldm == NULL)
			return 1;
		m = oldm;
		m->m_data = m->m_ext.ext_buf;
		rval = 1;
	}

	rxd->fr_mbhead = m;

	/*
	 * Setup the DMA map for this receive buffer.
	 */
	if (m != oldm) {
		if (oldm != NULL)
			bus_dmamap_unload(sc->sc_dmat, rxmap);
		error = bus_dmamap_load(sc->sc_dmat, rxmap,
		    m->m_ext.ext_buf, MCLBYTES, NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: can't load rx buffer, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			panic("fxp_add_rfabuf");	/* XXX */
		}
	}

	/*
	 * Move the data pointer up so that the incoming data packet
	 * will be 32-bit aligned.
	 */
	m->m_data += RFA_ALIGNMENT_FUDGE;

	/*
	 * Get a pointer to the base of the mbuf cluster and move
	 * data start past the RFA descriptor.
	 */
	rfa = mtod(m, struct fxp_rfa *);
	m->m_data += sizeof(struct fxp_rfa);
	rfa->size = MCLBYTES - sizeof(struct fxp_rfa) - RFA_ALIGNMENT_FUDGE;

	/*
	 * Initialize the rest of the RFA.
	 */
	rfa->rfa_status = 0;
	rfa->rfa_control = FXP_RFA_CONTROL_EL;
	rfa->actual_size = 0;

	/*
	 * Note that since the RFA is misaligned, we cannot store values
	 * directly.  Instead, we must copy.
	 */
	v = -1;
	memcpy((void *)&rfa->link_addr, &v, sizeof(v));
	memcpy((void *)&rfa->rbd_addr, &v, sizeof(v));

	/*
	 * If there are other buffers already on the list, attach this
	 * one to the end by fixing up the tail to point to this one.
	 */
	if (sc->rfa_head != NULL) {
		p_rfa = (struct fxp_rfa *)
		    (sc->rfa_tail->fr_mbhead->m_ext.ext_buf +
		     RFA_ALIGNMENT_FUDGE);
		sc->rfa_tail->fr_next = rxd;
		v = rxmap->dm_segs[0].ds_addr + RFA_ALIGNMENT_FUDGE;
		memcpy((void *)&p_rfa->link_addr, &v, sizeof(v));
		p_rfa->rfa_control &= ~FXP_RFA_CONTROL_EL;
	} else {
		sc->rfa_head = rxd;
	}
	sc->rfa_tail = rxd;

	bus_dmamap_sync(sc->sc_dmat, rxmap, 0, rxmap->dm_mapsize,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	return (rval);
}

static volatile int
fxp_mdi_read(self, phy, reg)
	struct device *self;
	int phy;
	int reg;
{
	struct fxp_softc *sc = (struct fxp_softc *)self;
	int count = 10000;
	int value;

	CSR_WRITE_4(sc, FXP_CSR_MDICONTROL,
	    (FXP_MDI_READ << 26) | (reg << 16) | (phy << 21));

	while (((value = CSR_READ_4(sc, FXP_CSR_MDICONTROL)) & 0x10000000) == 0
	    && count--)
		DELAY(10);

	if (count <= 0)
		printf("%s: fxp_mdi_read: timed out\n", sc->sc_dev.dv_xname);

	return (value & 0xffff);
}

static void
fxp_statchg(self)
	struct device *self;
{

	/* XXX Update ifp->if_baudrate */
}

static void
fxp_mdi_write(self, phy, reg, value)
	struct device *self;
	int phy;
	int reg;
	int value;
{
	struct fxp_softc *sc = (struct fxp_softc *)self;
	int count = 10000;

	CSR_WRITE_4(sc, FXP_CSR_MDICONTROL,
	    (FXP_MDI_WRITE << 26) | (reg << 16) | (phy << 21) |
	    (value & 0xffff));

	while((CSR_READ_4(sc, FXP_CSR_MDICONTROL) & 0x10000000) == 0 &&
	    count--)
		DELAY(10);

	if (count <= 0)
		printf("%s: fxp_mdi_write: timed out\n", sc->sc_dev.dv_xname);
}

static int
fxp_ioctl(ifp, command, data)
	struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct fxp_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			fxp_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			 register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			 if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
				    LLADDR(ifp->if_sadl);
			 else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ifp->if_addrlen);
			 /* Set new address. */
			 fxp_init(sc);
			 break;
		    }
#endif
		default:
			fxp_init(sc);
			break;
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCSIFFLAGS:
		sc->all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;

		/*
		 * If interface is marked up and not running, then start it.
		 * If it is marked down and running, stop it.
		 * XXX If it's up then re-initialize it. This is so flags
		 * such as IFF_PROMISC are handled.
		 */
		if (ifp->if_flags & IFF_UP) {
			fxp_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				fxp_stop(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		sc->all_mcasts = (ifp->if_flags & IFF_ALLMULTI) ? 1 : 0;
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware
			 * filter accordingly.
			 */
			if (!sc->all_mcasts)
				fxp_mc_setup(sc);
			/*
			 * fxp_mc_setup() can turn on all_mcasts if we run
			 * out of space, so check it again rather than else {}.
			 */
			if (sc->all_mcasts)
				fxp_init(sc);
			error = 0;
		}
		break;

	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, command);
		break;

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return (error);
}

/*
 * Program the multicast filter.
 *
 * We have an artificial restriction that the multicast setup command
 * must be the first command in the chain, so we take steps to ensure
 * that. By requiring this, it allows us to keep the performance of
 * the pre-initialized command ring (esp. link pointers) by not actually
 * inserting the mcsetup command in the ring - i.e. it's link pointer
 * points to the TxCB ring, but the mcsetup descriptor itself is not part
 * of it. We then can do 'CU_START' on the mcsetup descriptor and have it
 * lead into the regular TxCB ring when it completes.
 *
 * This function must be called at splnet.
 */
static void
fxp_mc_setup(sc)
	struct fxp_softc *sc;
{
	struct fxp_cb_mcs *mcsp = &sc->control_data->fcd_mcscb;
	struct ifnet *ifp = &sc->sc_if;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	int nmcasts;

	if (sc->tx_queued) {
		sc->need_mcsetup = 1;
		return;
	}
	sc->need_mcsetup = 0;

	/*
	 * Initialize multicast setup descriptor.
	 */
	mcsp->cb_soft.next = sc->control_data->fcd_txcbs;
	mcsp->cb_soft.mb_head = NULL;
	mcsp->cb_soft.dmamap = NULL;
	mcsp->cb_status = 0;
	mcsp->cb_command = FXP_CB_COMMAND_MCAS | FXP_CB_COMMAND_S;
	mcsp->link_addr = sc->sc_cddma + FXP_CDOFF(fcd_txcbs[0].cb_status);

	nmcasts = 0;
	if (!sc->all_mcasts) {
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			/*
			 * Check for too many multicast addresses or if we're
			 * listening to a range.  Either way, we simply have
			 * to accept all multicasts.
			 */
			if (nmcasts >= MAXMCADDR ||
			    bcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN) != 0) {
				sc->all_mcasts = 1;
				nmcasts = 0;
				break;
			}
			bcopy(enm->enm_addrlo,
			    (void *)
			    &sc->control_data->fcd_mcscb.mc_addr[nmcasts][0],
			    ETHER_ADDR_LEN);
			nmcasts++;
			ETHER_NEXT_MULTI(step, enm);
		}
	}
	mcsp->mc_cnt = nmcasts * 6;
	sc->cbl_first = sc->cbl_last = (struct fxp_cb_tx *) mcsp;
	sc->tx_queued = 1;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap,
	    FXP_CDOFF(fcd_mcscb.cb_status), FXP_MCSDESCSIZE,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Wait until command unit is not active. This should never
	 * be the case when nothing is queued, but make sure anyway.
	 */
	while ((CSR_READ_1(sc, FXP_CSR_SCB_RUSCUS) >> 6) ==
	    FXP_SCB_CUS_ACTIVE) ;

	/*
	 * Start the multicast setup command.
	 */
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL,
	    sc->sc_cddma + FXP_CDOFF(fcd_mcscb.cb_status));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);

	ifp->if_timer = 5;
	return;
}
