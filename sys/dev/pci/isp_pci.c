/* $NetBSD: isp_pci.c,v 1.72.2.1 2001/10/11 00:02:10 fvdl Exp $ */
/*
 * This driver, which is contained in NetBSD in the files:
 *
 *	sys/dev/ic/isp.c
 *	sys/dev/ic/isp_inline.h
 *	sys/dev/ic/isp_netbsd.c
 *	sys/dev/ic/isp_netbsd.h
 *	sys/dev/ic/isp_target.c
 *	sys/dev/ic/isp_target.h
 *	sys/dev/ic/isp_tpublic.h
 *	sys/dev/ic/ispmbox.h
 *	sys/dev/ic/ispreg.h
 *	sys/dev/ic/ispvar.h
 *	sys/microcode/isp/asm_sbus.h
 *	sys/microcode/isp/asm_1040.h
 *	sys/microcode/isp/asm_1080.h
 *	sys/microcode/isp/asm_12160.h
 *	sys/microcode/isp/asm_2100.h
 *	sys/microcode/isp/asm_2200.h
 *	sys/pci/isp_pci.c
 *	sys/sbus/isp_sbus.c
 *
 * Is being actively maintained by Matthew Jacob (mjacob@netbsd.org).
 * This driver also is shared source with FreeBSD, OpenBSD, Linux, Solaris,
 * Linux versions. This tends to be an interesting maintenance problem.
 *
 * Please coordinate with Matthew Jacob on changes you wish to make here.
 */
/*
 * PCI specific probe and attach routines for Qlogic ISP SCSI adapters.
 */
/*
 * Copyright (C) 1997, 1998, 1999 National Aeronautics & Space Administration
 * All rights reserved.
 *
 * Additional Copyright (C) 2000, 2001 by Matthew Jacob
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <dev/ic/isp_netbsd.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <uvm/uvm_extern.h>
#include <sys/reboot.h>

static u_int16_t isp_pci_rd_reg(struct ispsoftc *, int);
static void isp_pci_wr_reg(struct ispsoftc *, int, u_int16_t);
#if !(defined(ISP_DISABLE_1080_SUPPORT) && defined(ISP_DISABLE_12160_SUPPORT))
static u_int16_t isp_pci_rd_reg_1080(struct ispsoftc *, int);
static void isp_pci_wr_reg_1080(struct ispsoftc *, int, u_int16_t);
#endif
static int
isp_pci_rd_isr(struct ispsoftc *, u_int16_t *, u_int16_t *, u_int16_t *);
static int
isp_pci_rd_isr_2300(struct ispsoftc *, u_int16_t *, u_int16_t *, u_int16_t *);
static int isp_pci_mbxdma(struct ispsoftc *);
static int isp_pci_dmasetup(struct ispsoftc *, XS_T *, ispreq_t *,
    u_int16_t *, u_int16_t);
static void isp_pci_dmateardown(struct ispsoftc *, XS_T *, u_int16_t);
static void isp_pci_reset1(struct ispsoftc *);
static void isp_pci_dumpregs(struct ispsoftc *, const char *);
static int isp_pci_intr(void *);

#if	defined(ISP_DISABLE_1020_SUPPORT)
#define	ISP_1040_RISC_CODE	NULL
#else
#define	ISP_1040_RISC_CODE	isp_1040_risc_code
#include <dev/microcode/isp/asm_1040.h>
#endif

#if	defined(ISP_DISABLE_1080_SUPPORT)
#define	ISP_1080_RISC_CODE	NULL
#else
#define	ISP_1080_RISC_CODE	isp_1080_risc_code
#include <dev/microcode/isp/asm_1080.h>
#endif

#if	defined(ISP_DISABLE_12160_SUPPORT)
#define	ISP_12160_RISC_CODE	NULL
#else
#define	ISP_12160_RISC_CODE	isp_12160_risc_code
#include <dev/microcode/isp/asm_12160.h>
#endif

#if	defined(ISP_DISABLE_2100_SUPPORT)
#define	ISP_2100_RISC_CODE	NULL
#else
#define	ISP_2100_RISC_CODE	isp_2100_risc_code
#include <dev/microcode/isp/asm_2100.h>
#endif

#if	defined(ISP_DISABLE_2200_SUPPORT)
#define	ISP_2200_RISC_CODE	NULL
#else
#define	ISP_2200_RISC_CODE	isp_2200_risc_code
#include <dev/microcode/isp/asm_2200.h>
#endif

#if	defined(ISP_DISABLE_2300_SUPPORT)
#define	ISP_2300_RISC_CODE	NULL
#else
#define	ISP_2300_RISC_CODE	isp_2300_risc_code
#include <dev/microcode/isp/asm_2300.h>
#endif

#ifndef	ISP_DISABLE_1020_SUPPORT
static struct ispmdvec mdvec = {
	isp_pci_rd_isr,
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_1040_RISC_CODE,
	BIU_BURST_ENABLE|BIU_PCI_CONF1_FIFO_64
};
#endif

#ifndef	ISP_DISABLE_1080_SUPPORT
static struct ispmdvec mdvec_1080 = {
	isp_pci_rd_isr,
	isp_pci_rd_reg_1080,
	isp_pci_wr_reg_1080,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_1080_RISC_CODE,
	BIU_BURST_ENABLE|BIU_PCI_CONF1_FIFO_64
};
#endif

#ifndef	ISP_DISABLE_12160_SUPPORT
static struct ispmdvec mdvec_12160 = {
	isp_pci_rd_isr,
	isp_pci_rd_reg_1080,
	isp_pci_wr_reg_1080,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_12160_RISC_CODE,
	BIU_BURST_ENABLE|BIU_PCI_CONF1_FIFO_64
};
#endif

#ifndef	ISP_DISABLE_2100_SUPPORT
static struct ispmdvec mdvec_2100 = {
	isp_pci_rd_isr,
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_2100_RISC_CODE
};
#endif

#ifndef	ISP_DISABLE_2200_SUPPORT
static struct ispmdvec mdvec_2200 = {
	isp_pci_rd_isr,
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_2200_RISC_CODE
};
#endif

#ifndef	ISP_DISABLE_2300_SUPPORT
static struct ispmdvec mdvec_2300 = {
	isp_pci_rd_isr_2300,
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_2300_RISC_CODE
};
#endif

#ifndef	PCI_VENDOR_QLOGIC
#define	PCI_VENDOR_QLOGIC	0x1077
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP1020
#define	PCI_PRODUCT_QLOGIC_ISP1020	0x1020
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP1080
#define	PCI_PRODUCT_QLOGIC_ISP1080	0x1080
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP1240
#define	PCI_PRODUCT_QLOGIC_ISP1240	0x1240
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP1280
#define	PCI_PRODUCT_QLOGIC_ISP1280	0x1280
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP12160
#define	PCI_PRODUCT_QLOGIC_ISP12160	0x1216
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP2100
#define	PCI_PRODUCT_QLOGIC_ISP2100	0x2100
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP2200
#define	PCI_PRODUCT_QLOGIC_ISP2200	0x2200
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP2300
#define	PCI_PRODUCT_QLOGIC_ISP2300	0x2300
#endif

#ifndef	PCI_PRODUCT_QLOGIC_ISP2312
#define	PCI_PRODUCT_QLOGIC_ISP2312	0x2312
#endif

#define	PCI_QLOGIC_ISP	((PCI_PRODUCT_QLOGIC_ISP1020 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP1080	\
	((PCI_PRODUCT_QLOGIC_ISP1080 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP1240	\
	((PCI_PRODUCT_QLOGIC_ISP1240 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP1280	\
	((PCI_PRODUCT_QLOGIC_ISP1280 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP12160	\
	((PCI_PRODUCT_QLOGIC_ISP12160 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP2100	\
	((PCI_PRODUCT_QLOGIC_ISP2100 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP2200	\
	((PCI_PRODUCT_QLOGIC_ISP2200 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP2300	\
	((PCI_PRODUCT_QLOGIC_ISP2300 << 16) | PCI_VENDOR_QLOGIC)

#define	PCI_QLOGIC_ISP2312	\
	((PCI_PRODUCT_QLOGIC_ISP2312 << 16) | PCI_VENDOR_QLOGIC)

#define	IO_MAP_REG	0x10
#define	MEM_MAP_REG	0x14
#define	PCIR_ROMADDR	0x30

#define	PCI_DFLT_LTNCY	0x40
#define	PCI_DFLT_LNSZ	0x10


static int isp_pci_probe(struct device *, struct cfdata *, void *);
static void isp_pci_attach(struct device *, struct device *, void *);

struct isp_pcisoftc {
	struct ispsoftc		pci_isp;
	pci_chipset_tag_t	pci_pc;
	pcitag_t		pci_tag;
	bus_space_tag_t		pci_st;
	bus_space_handle_t	pci_sh;
	bus_dma_tag_t		pci_dmat;
	bus_dmamap_t		pci_scratch_dmap;	/* for fcp only */
	bus_dmamap_t		pci_rquest_dmap;
	bus_dmamap_t		pci_result_dmap;
	bus_dmamap_t		*pci_xfer_dmap;
	void *			pci_ih;
	int16_t			pci_poff[_NREG_BLKS];
};

struct cfattach isp_pci_ca = {
	sizeof (struct isp_pcisoftc), isp_pci_probe, isp_pci_attach
};

#ifdef	DEBUG
const char vstring[] = 
    "Qlogic ISP Driver, NetBSD (pci) Platform Version %d.%d Core Version %d.%d";
#endif

static int
isp_pci_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;
	switch (pa->pa_id) {
#ifndef	ISP_DISABLE_1020_SUPPORT
	case PCI_QLOGIC_ISP:
		return (1);
#endif
#ifndef	ISP_DISABLE_1080_SUPPORT
	case PCI_QLOGIC_ISP1080:
	case PCI_QLOGIC_ISP1240:
	case PCI_QLOGIC_ISP1280:
		return (1);
#endif
#ifndef	ISP_DISABLE_12160_SUPPORT
	case PCI_QLOGIC_ISP12160:
		return (1);
#endif
#ifndef	ISP_DISABLE_2100_SUPPORT
	case PCI_QLOGIC_ISP2100:
		return (1);
#endif
#ifndef	ISP_DISABLE_2200_SUPPORT
	case PCI_QLOGIC_ISP2200:
		return (1);
#endif
#ifndef	ISP_DISABLE_2300_SUPPORT
	case PCI_QLOGIC_ISP2300:
	case PCI_QLOGIC_ISP2312:
		return (1);
#endif
	default:
		return (0);
	}
}


static void
isp_pci_attach(struct device *parent, struct device *self, void *aux)
{
#ifdef	DEBUG
	static char oneshot = 1;
#endif
	static const char nomem[] = "%s: no mem for sdparam table\n";
	u_int32_t data, rev, linesz = PCI_DFLT_LNSZ;
	struct pci_attach_args *pa = aux;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) self;
	struct ispsoftc *isp = &pcs->pci_isp;
	bus_space_tag_t st, iot, memt;
	bus_space_handle_t sh, ioh, memh;
	pci_intr_handle_t ih;
	const char *intrstr;
	int ioh_valid, memh_valid;

	ioh_valid = (pci_mapreg_map(pa, IO_MAP_REG,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, MEM_MAP_REG,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);
	if (memh_valid) {
		st = memt;
		sh = memh;
	} else if (ioh_valid) {
		st = iot;
		sh = ioh;
	} else {
		printf(": unable to map device registers\n");
		return;
	}
	printf("\n");

	pcs->pci_st = st;
	pcs->pci_sh = sh;
	pcs->pci_dmat = pa->pa_dmat;
	pcs->pci_pc = pa->pa_pc;
	pcs->pci_tag = pa->pa_tag;
	pcs->pci_poff[BIU_BLOCK >> _BLK_REG_SHFT] = BIU_REGS_OFF;
	pcs->pci_poff[MBOX_BLOCK >> _BLK_REG_SHFT] = PCI_MBOX_REGS_OFF;
	pcs->pci_poff[SXP_BLOCK >> _BLK_REG_SHFT] = PCI_SXP_REGS_OFF;
	pcs->pci_poff[RISC_BLOCK >> _BLK_REG_SHFT] = PCI_RISC_REGS_OFF;
	pcs->pci_poff[DMA_BLOCK >> _BLK_REG_SHFT] = DMA_REGS_OFF;
	rev = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG) & 0xff;

#ifndef	ISP_DISABLE_1020_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP) {
		isp->isp_mdvec = &mdvec;
		isp->isp_type = ISP_HA_SCSI_UNKNOWN;
		isp->isp_param = malloc(sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, sizeof (sdparam));
	}
#endif
#ifndef	ISP_DISABLE_1080_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP1080) {
		isp->isp_mdvec = &mdvec_1080;
		isp->isp_type = ISP_HA_SCSI_1080;
		isp->isp_param = malloc(sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, sizeof (sdparam));
		pcs->pci_poff[DMA_BLOCK >> _BLK_REG_SHFT] =
		    ISP1080_DMA_REGS_OFF;
	}
	if (pa->pa_id == PCI_QLOGIC_ISP1240) {
		isp->isp_mdvec = &mdvec_1080;
		isp->isp_type = ISP_HA_SCSI_1240;
		isp->isp_param =
		    malloc(2 * sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, 2 * sizeof (sdparam));
		pcs->pci_poff[DMA_BLOCK >> _BLK_REG_SHFT] =
		    ISP1080_DMA_REGS_OFF;
	}
	if (pa->pa_id == PCI_QLOGIC_ISP1280) {
		isp->isp_mdvec = &mdvec_1080;
		isp->isp_type = ISP_HA_SCSI_1280;
		isp->isp_param =
		    malloc(2 * sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, 2 * sizeof (sdparam));
		pcs->pci_poff[DMA_BLOCK >> _BLK_REG_SHFT] =
		    ISP1080_DMA_REGS_OFF;
	}
#endif
#ifndef	ISP_DISABLE_12160_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP12160) {
		isp->isp_mdvec = &mdvec_12160;
		isp->isp_type = ISP_HA_SCSI_12160;
		isp->isp_param =
		    malloc(2 * sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, 2 * sizeof (sdparam));
		pcs->pci_poff[DMA_BLOCK >> _BLK_REG_SHFT] =
		    ISP1080_DMA_REGS_OFF;
	}
#endif
#ifndef	ISP_DISABLE_2100_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP2100) {
		isp->isp_mdvec = &mdvec_2100;
		isp->isp_type = ISP_HA_FC_2100;
		isp->isp_param = malloc(sizeof (fcparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, sizeof (fcparam));
		pcs->pci_poff[MBOX_BLOCK >> _BLK_REG_SHFT] =
		    PCI_MBOX_REGS2100_OFF;
		if (rev < 3) {
			/*
			 * XXX: Need to get the actual revision
			 * XXX: number of the 2100 FB. At any rate,
			 * XXX: lower cache line size for early revision
			 * XXX; boards.
			 */
			linesz = 1;
		}
	}
#endif
#ifndef	ISP_DISABLE_2200_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP2200) {
		isp->isp_mdvec = &mdvec_2200;
		isp->isp_type = ISP_HA_FC_2200;
		isp->isp_param = malloc(sizeof (fcparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, sizeof (fcparam));
		pcs->pci_poff[MBOX_BLOCK >> _BLK_REG_SHFT] =
		    PCI_MBOX_REGS2100_OFF;
		data = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG);
	}
#endif
#ifndef	ISP_DISABLE_2300_SUPPORT
	if (pa->pa_id == PCI_QLOGIC_ISP2300 ||
	    pa->pa_id == PCI_QLOGIC_ISP2312) {
		isp->isp_mdvec = &mdvec_2300;
		isp->isp_type = ISP_HA_FC_2300;
		isp->isp_param = malloc(sizeof (fcparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf(nomem, isp->isp_name);
			return;
		}
		memset(isp->isp_param, 0, sizeof (fcparam));
		pcs->pci_poff[MBOX_BLOCK >> _BLK_REG_SHFT] =
		    PCI_MBOX_REGS2300_OFF;
		data = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_CLASS_REG);
	}
#endif
	/*
	 * Set up logging levels.
	 */
#ifdef	ISP_LOGDEFAULT
	isp->isp_dblev = ISP_LOGDEFAULT;
#else
	isp->isp_dblev = ISP_LOGWARN|ISP_LOGERR;
	if (bootverbose)
		isp->isp_dblev |= ISP_LOGCONFIG|ISP_LOGINFO;
#ifdef	SCSIDEBUG
	isp->isp_dblev |= ISP_LOGDEBUG1|ISP_LOGDEBUG2;
#endif
#ifdef	DEBUG
	isp->isp_dblev |= ISP_LOGDEBUG0;
#endif
#endif

#ifdef	DEBUG
	if (oneshot) {
		oneshot = 0;
		isp_prt(isp, ISP_LOGCONFIG, vstring,
		    ISP_PLATFORM_VERSION_MAJOR, ISP_PLATFORM_VERSION_MINOR,
		    ISP_CORE_VERSION_MAJOR, ISP_CORE_VERSION_MINOR);
	}
#endif

	isp->isp_revision = rev;

	/*
	 * Make sure that command register set sanely.
	 */
	data = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	data |= PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_INVALIDATE_ENABLE;

	/*
	 * Not so sure about these- but I think it's important that they get
	 * enabled......
	 */
	data |= PCI_COMMAND_PARITY_ENABLE | PCI_COMMAND_SERR_ENABLE;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, data);

	/*
	 * Make sure that the latency timer, cache line size,
	 * and ROM is disabled.
	 */
	data = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG);
	data &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
	data &= ~(PCI_CACHELINE_MASK << PCI_CACHELINE_SHIFT);
	data |= (PCI_DFLT_LTNCY	<< PCI_LATTIMER_SHIFT);
	data |= (linesz << PCI_CACHELINE_SHIFT);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_BHLC_REG, data);

	data = pci_conf_read(pa->pa_pc, pa->pa_tag, PCIR_ROMADDR);
	data &= ~1;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCIR_ROMADDR, data);

	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", isp->isp_name);
		free(isp->isp_param, M_DEVBUF);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	if (intrstr == NULL)
		intrstr = "<I dunno>";
	pcs->pci_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
	    isp_pci_intr, isp);
	if (pcs->pci_ih == NULL) {
		printf("%s: couldn't establish interrupt at %s\n",
			isp->isp_name, intrstr);
		free(isp->isp_param, M_DEVBUF);
		return;
	}

	printf("%s: interrupting at %s\n", isp->isp_name, intrstr);

	if (IS_FC(isp)) {
		DEFAULT_NODEWWN(isp) = 0x400000007F000002;
		DEFAULT_PORTWWN(isp) = 0x400000007F000002;
	}

	isp->isp_confopts = self->dv_cfdata->cf_flags;
	isp->isp_role = ISP_DEFAULT_ROLES;
	ISP_LOCK(isp);
	isp->isp_osinfo.no_mbox_ints = 1;
	isp_reset(isp);
	if (isp->isp_state != ISP_RESETSTATE) {
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}
	ENABLE_INTS(isp);
	isp_init(isp);
	if (isp->isp_state != ISP_INITSTATE) {
		isp_uninit(isp);
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}
	/*
	 * Do platform attach.
	 */
	ISP_UNLOCK(isp);
	isp_attach(isp);
	if (isp->isp_state != ISP_RUNSTATE) {
		ISP_LOCK(isp);
		isp_uninit(isp);
		free(isp->isp_param, M_DEVBUF);
		ISP_UNLOCK(isp);
	}
}

#define	IspVirt2Off(a, x)	\
	(((struct isp_pcisoftc *)a)->pci_poff[((x) & _BLK_REG_MASK) >> \
	_BLK_REG_SHFT] + ((x) & 0xff))

#define	BXR2(pcs, off)		\
	bus_space_read_2(pcs->pci_st, pcs->pci_sh, off)
#define	BXW2(pcs, off, v)	\
	bus_space_write_2(pcs->pci_st, pcs->pci_sh, off, v)


static INLINE int
isp_pci_rd_debounced(struct ispsoftc *isp, int off, u_int16_t *rp)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	u_int16_t val0, val1;
	int i = 0;

	do {
		val0 = BXR2(pcs, IspVirt2Off(isp, off));
		val1 = BXR2(pcs, IspVirt2Off(isp, off));
	} while (val0 != val1 && ++i < 1000);
	if (val0 != val1) {
		return (1);
	}
	*rp = val0;
	return (0);
}

static int
isp_pci_rd_isr(struct ispsoftc *isp, u_int16_t *isrp,
    u_int16_t *semap, u_int16_t *mbp)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	u_int16_t isr, sema;

	if (IS_2100(isp)) {
		if (isp_pci_rd_debounced(isp, BIU_ISR, &isr)) {
		    return (0);
		}
		if (isp_pci_rd_debounced(isp, BIU_SEMA, &sema)) {
		    return (0);
		}
	} else {
		isr = BXR2(pcs, IspVirt2Off(isp, BIU_ISR));
		sema = BXR2(pcs, IspVirt2Off(isp, BIU_SEMA));
	}
	isp_prt(isp, ISP_LOGDEBUG3, "ISR 0x%x SEMA 0x%x", isr, sema);
	isr &= INT_PENDING_MASK(isp);
	sema &= BIU_SEMA_LOCK;
	if (isr == 0 && sema == 0) {
		return (0);
	}
	*isrp = isr;
	if ((*semap = sema) != 0) {
		if (IS_2100(isp)) {
			if (isp_pci_rd_debounced(isp, OUTMAILBOX0, mbp)) {
				return (0);
			}
		} else {
			*mbp = BXR2(pcs, IspVirt2Off(isp, OUTMAILBOX0));
		}
	}
	return (1);
}

#ifndef	ISP_DISABLE_2300_SUPPORT
static int
isp_pci_rd_isr_2300(struct ispsoftc *isp, u_int16_t *isrp,
    u_int16_t *semap, u_int16_t *mbox0p)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	u_int32_t r2hisr;

	if (!(BXR2(pcs, IspVirt2Off(isp, BIU_ISR)) & BIU2100_ISR_RISC_INT)) {
		*isrp = 0;
		return (0);
	}
	r2hisr = bus_space_read_4(pcs->pci_st, pcs->pci_sh,
	    IspVirt2Off(pcs, BIU_R2HSTSLO));
	isp_prt(isp, ISP_LOGDEBUG3, "RISC2HOST ISR 0x%x", r2hisr);
	if ((r2hisr & BIU_R2HST_INTR) == 0) {
		*isrp = 0;
		return (0);
	}
	switch (r2hisr & BIU_R2HST_ISTAT_MASK) {
	case ISPR2HST_ROM_MBX_OK:
	case ISPR2HST_ROM_MBX_FAIL:
	case ISPR2HST_MBX_OK:
	case ISPR2HST_MBX_FAIL:
	case ISPR2HST_ASYNC_EVENT:
	case ISPR2HST_FPOST:
	case ISPR2HST_FPOST_CTIO:
		*isrp = r2hisr & 0xffff;
		*mbox0p = (r2hisr >> 16);
		*semap = 1;
		return (1);
	case ISPR2HST_RSPQ_UPDATE:
		*isrp = r2hisr & 0xffff;
		*mbox0p = 0;
		*semap = 0;
		return (1);
	default:
		return (0);
	}
}
#endif

static u_int16_t
isp_pci_rd_reg(struct ispsoftc *isp, int regoff)
{
	u_int16_t rv;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int oldconf = 0;

	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK) {
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oldconf = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1),
		    oldconf | BIU_PCI_CONF1_SXP);
	}
	rv = BXR2(pcs, IspVirt2Off(isp, regoff));
	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK) {
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), oldconf);
	}
	return (rv);
}

static void
isp_pci_wr_reg(struct ispsoftc *isp, int regoff, u_int16_t val)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int oldconf = 0;

	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK) {
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oldconf = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1),
		    oldconf | BIU_PCI_CONF1_SXP);
	}
	BXW2(pcs, IspVirt2Off(isp, regoff), val);
	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK) {
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), oldconf);
	}
}

#if !(defined(ISP_DISABLE_1080_SUPPORT) && defined(ISP_DISABLE_12160_SUPPORT))
static u_int16_t
isp_pci_rd_reg_1080(struct ispsoftc *isp, int regoff)
{
	u_int16_t rv, oc = 0;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;

	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK ||
	    (regoff & _BLK_REG_MASK) == (SXP_BLOCK|SXP_BANK1_SELECT)) {
		u_int16_t tc;
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oc = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		tc = oc & ~BIU_PCI1080_CONF1_DMA;
		if (regoff & SXP_BANK1_SELECT)
			tc |= BIU_PCI1080_CONF1_SXP1;
		else
			tc |= BIU_PCI1080_CONF1_SXP0;
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), tc);
	} else if ((regoff & _BLK_REG_MASK) == DMA_BLOCK) {
		oc = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), 
		    oc | BIU_PCI1080_CONF1_DMA);
	}
	rv = BXR2(pcs, IspVirt2Off(isp, regoff));
	if (oc) {
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), oc);
	}
	return (rv);
}

static void
isp_pci_wr_reg_1080(struct ispsoftc *isp, int regoff, u_int16_t val)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int oc = 0;

	if ((regoff & _BLK_REG_MASK) == SXP_BLOCK ||
	    (regoff & _BLK_REG_MASK) == (SXP_BLOCK|SXP_BANK1_SELECT)) {
		u_int16_t tc;
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oc = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		tc = oc & ~BIU_PCI1080_CONF1_DMA;
		if (regoff & SXP_BANK1_SELECT)
			tc |= BIU_PCI1080_CONF1_SXP1;
		else
			tc |= BIU_PCI1080_CONF1_SXP0;
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), tc);
	} else if ((regoff & _BLK_REG_MASK) == DMA_BLOCK) {
		oc = BXR2(pcs, IspVirt2Off(isp, BIU_CONF1));
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), 
		    oc | BIU_PCI1080_CONF1_DMA);
	}
	BXW2(pcs, IspVirt2Off(isp, regoff), val);
	if (oc) {
		BXW2(pcs, IspVirt2Off(isp, BIU_CONF1), oc);
	}
}
#endif

static int
isp_pci_mbxdma(struct ispsoftc *isp)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *)isp;
	bus_dma_tag_t dmat = pcs->pci_dmat;
	bus_dma_segment_t sg;
	bus_size_t len;
	fcparam *fcp;
	int rs, i;

	if (isp->isp_rquest_dma)	/* been here before? */
		return (0);

	len = isp->isp_maxcmds * sizeof (XS_T *);
	isp->isp_xflist = (XS_T **) malloc(len, M_DEVBUF, M_WAITOK);
	if (isp->isp_xflist == NULL) {
		isp_prt(isp, ISP_LOGERR, "cannot malloc xflist array");
		return (1);
	}
	memset(isp->isp_xflist, 0, len);
	len = isp->isp_maxcmds * sizeof (bus_dmamap_t);
	pcs->pci_xfer_dmap = (bus_dmamap_t *) malloc(len, M_DEVBUF, M_WAITOK);
	if (pcs->pci_xfer_dmap == NULL) {
		free(isp->isp_xflist, M_DEVBUF);
		isp->isp_xflist = NULL;
		isp_prt(isp, ISP_LOGERR, "cannot malloc dma map array");
		return (1);
	}
	for (i = 0; i < isp->isp_maxcmds; i++) {
		if (bus_dmamap_create(dmat, MAXPHYS, (MAXPHYS / PAGE_SIZE) + 1,
		    MAXPHYS, 0, BUS_DMA_NOWAIT, &pcs->pci_xfer_dmap[i])) {
			isp_prt(isp, ISP_LOGERR, "cannot create dma maps");
			break;
		}
	}
	if (i < isp->isp_maxcmds) {
		while (--i >= 0) {
			bus_dmamap_destroy(dmat, pcs->pci_xfer_dmap[i]);
		}
		free(isp->isp_xflist, M_DEVBUF);
		free(pcs->pci_xfer_dmap, M_DEVBUF);
		isp->isp_xflist = NULL;
		pcs->pci_xfer_dmap = NULL;
		return (1);
	}

	/*
	 * Allocate and map the request queue.
	 */
	len = ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN(isp));
	if (bus_dmamem_alloc(dmat, len, PAGE_SIZE, 0, &sg, 1, &rs,
			     BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(pcs->pci_dmat, &sg, rs, len,
	    (caddr_t *)&isp->isp_rquest, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		goto dmafail;
	}

	if (bus_dmamap_create(dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	    &pcs->pci_rquest_dmap) || bus_dmamap_load(dmat,
	    pcs->pci_rquest_dmap, (caddr_t)isp->isp_rquest, len, NULL,
	    BUS_DMA_NOWAIT)) {
		goto dmafail;
	}

	isp->isp_rquest_dma = pcs->pci_rquest_dmap->dm_segs[0].ds_addr;

	/*
	 * Allocate and map the result queue.
	 */
	len = ISP_QUEUE_SIZE(RESULT_QUEUE_LEN(isp));
	if (bus_dmamem_alloc(dmat, len, PAGE_SIZE, 0, &sg, 1, &rs,
			     BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(dmat, &sg, rs, len, (caddr_t *)&isp->isp_result,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		goto dmafail;
	}
	if (bus_dmamap_create(dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	    &pcs->pci_result_dmap) || bus_dmamap_load(pcs->pci_dmat,
	    pcs->pci_result_dmap, (caddr_t)isp->isp_result, len, NULL,
	    BUS_DMA_NOWAIT)) {
		goto dmafail;
	}
	isp->isp_result_dma = pcs->pci_result_dmap->dm_segs[0].ds_addr;

	if (IS_SCSI(isp)) {
		return (0);
	}

	fcp = isp->isp_param;
	len = ISP2100_SCRLEN;
	if (bus_dmamem_alloc(dmat, len, PAGE_SIZE, 0, &sg, 1, &rs,
			     BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(dmat, &sg, rs, len, (caddr_t *)&fcp->isp_scratch,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		goto dmafail;
	}
	if (bus_dmamap_create(dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	    &pcs->pci_scratch_dmap) || bus_dmamap_load(dmat,
	    pcs->pci_scratch_dmap, (caddr_t)fcp->isp_scratch, len, NULL,
	    BUS_DMA_NOWAIT)) {
		goto dmafail;
	}
	fcp->isp_scdma = pcs->pci_scratch_dmap->dm_segs[0].ds_addr;
	return (0);
dmafail:
	isp_prt(isp, ISP_LOGERR, "mailbox dma setup failure");
	for (i = 0; i < isp->isp_maxcmds; i++) {
		bus_dmamap_destroy(dmat, pcs->pci_xfer_dmap[i]);
	}
	free(isp->isp_xflist, M_DEVBUF);
	free(pcs->pci_xfer_dmap, M_DEVBUF);
	isp->isp_xflist = NULL;
	pcs->pci_xfer_dmap = NULL;
	return (1);
}

static int
isp_pci_dmasetup(struct ispsoftc *isp, struct scsipi_xfer *xs, ispreq_t *rq,
    u_int16_t *iptrp, u_int16_t optr)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *)isp;
	bus_dmamap_t dmap;
	ispcontreq_t *crq;
	int segcnt, seg, error, ovseg, seglim, drq;

	dmap = pcs->pci_xfer_dmap[isp_handle_index(rq->req_handle)];

	if (xs->datalen == 0) {
		rq->req_seg_count = 1;
		goto mbxsync;
	}
	if (xs->xs_control & XS_CTL_DATA_IN) {
		drq = REQFLAG_DATA_IN;
	} else {
		drq = REQFLAG_DATA_OUT;
	}

	if (IS_FC(isp)) {
		seglim = ISP_RQDSEG_T2;
		((ispreqt2_t *)rq)->req_totalcnt = xs->datalen;
		((ispreqt2_t *)rq)->req_flags |= drq;
	} else {
		rq->req_flags |= drq;
		if (XS_CDBLEN(xs) > 12) {
			seglim = 0;
		} else {
			seglim = ISP_RQDSEG;
		}
	}
	error = bus_dmamap_load(pcs->pci_dmat, dmap, xs->data, xs->datalen,
	    NULL, ((xs->xs_control & XS_CTL_NOSLEEP) ?
	    BUS_DMA_NOWAIT : BUS_DMA_WAITOK) | BUS_DMA_STREAMING |
	    ((xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMA_READ : BUS_DMA_WRITE));
	if (error) {
		XS_SETERR(xs, HBA_BOTCH);
		return (CMD_COMPLETE);
	}

	segcnt = dmap->dm_nsegs;

	isp_prt(isp, ISP_LOGDEBUG2, "%d byte %s %p in %d segs",
	    xs->datalen, (xs->xs_control & XS_CTL_DATA_IN)? "read to" :
	    "write from", xs->data, segcnt);

	for (seg = 0, rq->req_seg_count = 0;
	    seglim && seg < segcnt && rq->req_seg_count < seglim;
	    seg++, rq->req_seg_count++) {
		if (IS_FC(isp)) {
			ispreqt2_t *rq2 = (ispreqt2_t *)rq;
#if	_BYTE_ORDER == _BIG_ENDIAN
			rq2->req_dataseg[rq2->req_seg_count].ds_count =
			    bswap32(dmap->dm_segs[seg].ds_len);
			rq2->req_dataseg[rq2->req_seg_count].ds_base =
			    bswap32(dmap->dm_segs[seg].ds_addr);
#else
			rq2->req_dataseg[rq2->req_seg_count].ds_count =
			    dmap->dm_segs[seg].ds_len;
			rq2->req_dataseg[rq2->req_seg_count].ds_base =
			    dmap->dm_segs[seg].ds_addr;
#endif
		} else {
#if	_BYTE_ORDER == _BIG_ENDIAN
			rq->req_dataseg[rq->req_seg_count].ds_count =
			    bswap32(dmap->dm_segs[seg].ds_len);
			rq->req_dataseg[rq->req_seg_count].ds_base =
			    bswap32(dmap->dm_segs[seg].ds_addr);
#else
			rq->req_dataseg[rq->req_seg_count].ds_count =
			    dmap->dm_segs[seg].ds_len;
			rq->req_dataseg[rq->req_seg_count].ds_base =
			    dmap->dm_segs[seg].ds_addr;
#endif
		}
		isp_prt(isp, ISP_LOGDEBUG2, "seg0.[%d]={0x%lx,%lu}",
		    rq->req_seg_count, (long) dmap->dm_segs[seg].ds_addr,
		    (unsigned long) dmap->dm_segs[seg].ds_len);
	}

	if (seg == segcnt)
		goto dmasync;

	do {
		crq = (ispcontreq_t *) ISP_QUEUE_ENTRY(isp->isp_rquest, *iptrp);
		*iptrp = ISP_NXT_QENTRY(*iptrp, RQUEST_QUEUE_LEN(isp));
		if (*iptrp == optr) {
			isp_prt(isp, ISP_LOGDEBUG0, "Request Queue Overflow++");
			bus_dmamap_unload(pcs->pci_dmat, dmap);
			XS_SETERR(xs, HBA_BOTCH);
			return (CMD_EAGAIN);
		}
		rq->req_header.rqs_entry_count++;
		memset((void *)crq, 0, sizeof (*crq));
		crq->req_header.rqs_entry_count = 1;
		crq->req_header.rqs_entry_type = RQSTYPE_DATASEG;

		for (ovseg = 0; seg < segcnt && ovseg < ISP_CDSEG;
		    rq->req_seg_count++, seg++, ovseg++) {
#if	_BYTE_ORDER == _BIG_ENDIAN
			crq->req_dataseg[ovseg].ds_count =
			    bswap32(dmap->dm_segs[seg].ds_len);
			crq->req_dataseg[ovseg].ds_base =
			    bswap32(dmap->dm_segs[seg].ds_addr);
#else
			crq->req_dataseg[ovseg].ds_count =
			    dmap->dm_segs[seg].ds_len;
			crq->req_dataseg[ovseg].ds_base =
			    dmap->dm_segs[seg].ds_addr;
#endif
			isp_prt(isp, ISP_LOGDEBUG2, "seg%d.[%d]={0x%lx,%lu}",
			    rq->req_header.rqs_entry_count - 1,
			    rq->req_seg_count, (long)dmap->dm_segs[seg].ds_addr,
			    (unsigned long) dmap->dm_segs[seg].ds_len);
		}
	} while (seg < segcnt);


dmasync:
	bus_dmamap_sync(pcs->pci_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->xs_control & XS_CTL_DATA_IN) ?  BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

mbxsync:
	ISP_SWIZZLE_REQUEST(isp, rq);
	bus_dmamap_sync(pcs->pci_dmat, pcs->pci_rquest_dmap, 0,
	    pcs->pci_rquest_dmap->dm_mapsize, BUS_DMASYNC_PREWRITE);
	return (CMD_QUEUED);
}

static int
isp_pci_intr(void *arg)
{
	u_int16_t isr, sema, mbox;
	struct ispsoftc *isp = arg;

	isp->isp_intcnt++;
	if (ISP_READ_ISR(isp, &isr, &sema, &mbox) == 0) {
		isp->isp_intbogus++; 
		return (0);
	} else {
		struct isp_pcisoftc *pcs = (struct isp_pcisoftc *)arg;
		bus_dmamap_sync(pcs->pci_dmat, pcs->pci_result_dmap, 0,
		    pcs->pci_result_dmap->dm_mapsize, BUS_DMASYNC_POSTREAD);
		isp->isp_osinfo.onintstack = 1;
		isp_intr(isp, isr, sema, mbox);
		isp->isp_osinfo.onintstack = 0;
		return (1);
	}
}

static void
isp_pci_dmateardown(struct ispsoftc *isp, XS_T *xs, u_int16_t handle)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *)isp;
	bus_dmamap_t dmap = pcs->pci_xfer_dmap[isp_handle_index(handle)];
	bus_dmamap_sync(pcs->pci_dmat, dmap, 0, dmap->dm_mapsize,
	    xs->xs_control & XS_CTL_DATA_IN ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(pcs->pci_dmat, dmap);
}

static void
isp_pci_reset1(struct ispsoftc *isp)
{
	/* Make sure the BIOS is disabled */
	isp_pci_wr_reg(isp, HCCR, PCI_HCCR_CMD_BIOS);
}

static void
isp_pci_dumpregs(struct ispsoftc *isp, const char *msg)
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *)isp;
	if (msg)
		printf("%s: %s\n", isp->isp_name, msg);
	if (IS_SCSI(isp))
		printf("    biu_conf1=%x", ISP_READ(isp, BIU_CONF1));
	else
		printf("    biu_csr=%x", ISP_READ(isp, BIU2100_CSR));
	printf(" biu_icr=%x biu_isr=%x biu_sema=%x ", ISP_READ(isp, BIU_ICR),
	    ISP_READ(isp, BIU_ISR), ISP_READ(isp, BIU_SEMA));
	printf("risc_hccr=%x\n", ISP_READ(isp, HCCR));


	if (IS_SCSI(isp)) {
		ISP_WRITE(isp, HCCR, HCCR_CMD_PAUSE);
		printf("    cdma_conf=%x cdma_sts=%x cdma_fifostat=%x\n",
			ISP_READ(isp, CDMA_CONF), ISP_READ(isp, CDMA_STATUS),
			ISP_READ(isp, CDMA_FIFO_STS));
		printf("    ddma_conf=%x ddma_sts=%x ddma_fifostat=%x\n",
			ISP_READ(isp, DDMA_CONF), ISP_READ(isp, DDMA_STATUS),
			ISP_READ(isp, DDMA_FIFO_STS));
		printf("    sxp_int=%x sxp_gross=%x sxp(scsi_ctrl)=%x\n",
			ISP_READ(isp, SXP_INTERRUPT),
			ISP_READ(isp, SXP_GROSS_ERR),
			ISP_READ(isp, SXP_PINS_CTRL));
		ISP_WRITE(isp, HCCR, HCCR_CMD_RELEASE);
	}
	printf("    mbox regs: %x %x %x %x %x\n",
	    ISP_READ(isp, OUTMAILBOX0), ISP_READ(isp, OUTMAILBOX1),
	    ISP_READ(isp, OUTMAILBOX2), ISP_READ(isp, OUTMAILBOX3),
	    ISP_READ(isp, OUTMAILBOX4));
	printf("    PCI Status Command/Status=%x\n",
	    pci_conf_read(pcs->pci_pc, pcs->pci_tag, PCI_COMMAND_STATUS_REG));
}
