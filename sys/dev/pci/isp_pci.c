/*
 * PCI specific probe and attach routines for Qlogic ISP SCSI adapters.
 *
 *---------------------------------------
 * Copyright (c) 1997, 1998 by Matthew Jacob
 * NASA/Ames Research Center
 * All rights reserved.
 *---------------------------------------
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <dev/ic/isp_netbsd.h>
#include <dev/microcode/isp/asm_pci.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static u_int16_t isp_pci_rd_reg __P((struct ispsoftc *, int));
static void isp_pci_wr_reg __P((struct ispsoftc *, int, u_int16_t));
static int isp_pci_mbxdma __P((struct ispsoftc *));
static int isp_pci_dmasetup __P((struct ispsoftc *, struct scsipi_xfer *,
	ispreq_t *, u_int8_t *, u_int8_t));
static void isp_pci_dmateardown __P((struct ispsoftc *, struct scsipi_xfer *,
	u_int32_t));
static void isp_pci_reset1 __P((struct ispsoftc *));
static void isp_pci_dumpregs __P((struct ispsoftc *));
static int isp_pci_intr __P((void *));

static struct ispmdvec mdvec = {
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP_RISC_CODE,
	ISP_CODE_LENGTH,
	ISP_CODE_ORG,
	ISP_CODE_VERSION,
	BIU_BURST_ENABLE,	/* default to 8 byte burst */
	0
};

static struct ispmdvec mdvec_2100 = {
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	isp_pci_dmateardown,
	NULL,
	isp_pci_reset1,
	isp_pci_dumpregs,
	ISP2100_RISC_CODE,
	ISP2100_CODE_LENGTH,
	ISP2100_CODE_ORG,
	ISP2100_CODE_VERSION,
	BIU_BURST_ENABLE,	/* default to 8 byte burst */
	0			/* Not relevant to the 2100 */
};

#define	PCI_QLOGIC_ISP	\
	((PCI_PRODUCT_QLOGIC_ISP1020 << 16) | PCI_VENDOR_QLOGIC)

#ifndef	PCI_PRODUCT_QLOGIC_ISP2100
#define	PCI_PRODUCT_QLOGIC_ISP2100	0x2100
#endif
#define	PCI_QLOGIC_ISP2100	\
	((PCI_PRODUCT_QLOGIC_ISP2100 << 16) | PCI_VENDOR_QLOGIC)

#define IO_MAP_REG	0x10
#define MEM_MAP_REG	0x14


static int isp_pci_probe __P((struct device *, struct cfdata *, void *));
static void isp_pci_attach __P((struct device *, struct device *, void *));

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
	bus_dmamap_t		pci_xfer_dmap[MAXISPREQUEST];
	void *			pci_ih;
};

struct cfattach isp_pci_ca = {
	sizeof (struct isp_pcisoftc), isp_pci_probe, isp_pci_attach
};

static int
isp_pci_probe(parent, match, aux)
        struct device *parent;
        struct cfdata *match;
	void *aux; 
{       
        struct pci_attach_args *pa = aux;

	if (pa->pa_id == PCI_QLOGIC_ISP ||
	    pa->pa_id == PCI_QLOGIC_ISP2100) {
		return (1);
	} else {
		return (0);
	}
}


static void    
isp_pci_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
#ifdef	DEBUG
	static char oneshot = 1;
#endif
	struct pci_attach_args *pa = aux;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) self;
	struct ispsoftc *isp = &pcs->pci_isp;
	bus_space_tag_t st, iot, memt;
	bus_space_handle_t sh, ioh, memh;
	pci_intr_handle_t ih;
	const char *intrstr;
	int ioh_valid, memh_valid, i;
	ISP_LOCKVAL_DECL;

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
	if (pa->pa_id == PCI_QLOGIC_ISP) {
		isp->isp_mdvec = &mdvec;
		isp->isp_type = ISP_HA_SCSI_UNKNOWN;
		isp->isp_param = malloc(sizeof (sdparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf("%s: couldn't allocate sdparam table\n",
			       isp->isp_name);
			return;
		}
		bzero(isp->isp_param, sizeof (sdparam));
	} else if (pa->pa_id == PCI_QLOGIC_ISP2100) {
		u_int32_t data;
		isp->isp_mdvec = &mdvec_2100;
		if (ioh_valid == 0) {
			printf("%s: warning, ISP2100 cannot use I/O Space"
				" Mappings\n", isp->isp_name);
		} else {
			pcs->pci_st = iot;
			pcs->pci_sh = ioh;
		}

		isp->isp_type = ISP_HA_FC_2100;
		isp->isp_param = malloc(sizeof (fcparam), M_DEVBUF, M_NOWAIT);
		if (isp->isp_param == NULL) {
			printf("%s: couldn't allocate fcparam table\n",
			       isp->isp_name);
			return;
		}
		bzero(isp->isp_param, sizeof (fcparam));

		data = pci_conf_read(pa->pa_pc, pa->pa_tag,
			PCI_COMMAND_STATUS_REG);
		data |= PCI_COMMAND_MASTER_ENABLE |
			PCI_COMMAND_INVALIDATE_ENABLE;
		pci_conf_write(pa->pa_pc, pa->pa_tag,
			PCI_COMMAND_STATUS_REG, data);
	} else {
		return;
	}
#ifdef DEBUG
	if (oneshot) {
		oneshot = 0;
		printf("Qlogic ISP Driver, NetBSD (pci) Platform Version "
		    "%d.%d Core Version %d.%d\n",
		    ISP_PLATFORM_VERSION_MAJOR, ISP_PLATFORM_VERSION_MINOR,
		    ISP_CORE_VERSION_MAJOR, ISP_CORE_VERSION_MINOR);
	}
#endif
	ISP_LOCK(isp);
	isp_reset(isp);
	if (isp->isp_state != ISP_RESETSTATE) {
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}
	isp_init(isp);
	if (isp->isp_state != ISP_INITSTATE) {
		isp_uninit(isp);
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}

	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", isp->isp_name);
		isp_uninit(isp);
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}

	intrstr = pci_intr_string(pa->pa_pc, ih);
	if (intrstr == NULL)
		intrstr = "<I dunno>";
	pcs->pci_ih =
	  pci_intr_establish(pa->pa_pc, ih, IPL_BIO, isp_pci_intr, isp);
	if (pcs->pci_ih == NULL) {
		printf("%s: couldn't establish interrupt at %s\n",
			isp->isp_name, intrstr);
		isp_uninit(isp);
		ISP_UNLOCK(isp);
		free(isp->isp_param, M_DEVBUF);
		return;
	}
	printf("%s: interrupting at %s\n", isp->isp_name, intrstr);

	/*
	 * Create the DMA maps for the data transfers.
	 */
	for (i = 0; i < RQUEST_QUEUE_LEN; i++) {
		if (bus_dmamap_create(pcs->pci_dmat, MAXPHYS,
		    (MAXPHYS / NBPG) + 1, MAXPHYS, 0, BUS_DMA_NOWAIT,
		    &pcs->pci_xfer_dmap[i])) {
			printf("%s: can't create dma maps\n",
			    isp->isp_name);
			isp_uninit(isp);
			ISP_UNLOCK(isp);
			return;
		}
	}
	/*
	 * Do Generic attach now.
	 */
	isp_attach(isp);
	if (isp->isp_state != ISP_RUNSTATE) {
		isp_uninit(isp);
		free(isp->isp_param, M_DEVBUF);
	}
	ISP_UNLOCK(isp);
}

#define  PCI_BIU_REGS_OFF		BIU_REGS_OFF

static u_int16_t
isp_pci_rd_reg(isp, regoff)
	struct ispsoftc *isp;
	int regoff;
{
	u_int16_t rv;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int offset, oldsxp = 0;

	if ((regoff & BIU_BLOCK) != 0) {
		offset = PCI_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		if (isp->isp_type & ISP_HA_SCSI)
			offset = PCI_MBOX_REGS_OFF;
		else
			offset = PCI_MBOX_REGS2100_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = PCI_SXP_REGS_OFF;
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oldsxp = isp_pci_rd_reg(isp, BIU_CONF1);
		isp_pci_wr_reg(isp, BIU_CONF1, oldsxp & ~BIU_PCI_CONF1_SXP);
	} else {
		offset = PCI_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	rv = bus_space_read_2(pcs->pci_st, pcs->pci_sh, offset);
	if ((regoff & SXP_BLOCK) != 0) {
		isp_pci_wr_reg(isp, BIU_CONF1, oldsxp);
	}
	return (rv);
}

static void
isp_pci_wr_reg(isp, regoff, val)
	struct ispsoftc *isp;
	int regoff;
	u_int16_t val;
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int offset, oldsxp = 0;
	if ((regoff & BIU_BLOCK) != 0) {
		offset = PCI_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		if (isp->isp_type & ISP_HA_SCSI)
			offset = PCI_MBOX_REGS_OFF;
		else
			offset = PCI_MBOX_REGS2100_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = PCI_SXP_REGS_OFF;
		/*
		 * We will assume that someone has paused the RISC processor.
		 */
		oldsxp = isp_pci_rd_reg(isp, BIU_CONF1);
		isp_pci_wr_reg(isp, BIU_CONF1, oldsxp & ~BIU_PCI_CONF1_SXP);
	} else {
		offset = PCI_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	bus_space_write_2(pcs->pci_st, pcs->pci_sh, offset, val);
	if ((regoff & SXP_BLOCK) != 0) {
		isp_pci_wr_reg(isp, BIU_CONF1, oldsxp);
	}
}

static int
isp_pci_mbxdma(isp)
	struct ispsoftc *isp;
{
	struct isp_pcisoftc *pci = (struct isp_pcisoftc *)isp;
	bus_dma_segment_t seg;
	bus_size_t len;
	fcparam *fcp;
	int rseg;

	/*
	 * Allocate and map the request queue.
	 */
	len = ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN);
	if (bus_dmamem_alloc(pci->pci_dmat, len, NBPG, 0, &seg, 1, &rseg,
	      BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(pci->pci_dmat, &seg, rseg, len,
	      (caddr_t *)&isp->isp_rquest, BUS_DMA_NOWAIT|BUS_DMA_COHERENT))
		return (1);
	if (bus_dmamap_create(pci->pci_dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	      &pci->pci_rquest_dmap) ||
	    bus_dmamap_load(pci->pci_dmat, pci->pci_rquest_dmap,
	      (caddr_t)isp->isp_rquest, len, NULL, BUS_DMA_NOWAIT))
		return (1);

	isp->isp_rquest_dma = pci->pci_rquest_dmap->dm_segs[0].ds_addr;

	/*
	 * Allocate and map the result queue.
	 */
	len = ISP_QUEUE_SIZE(RESULT_QUEUE_LEN);
	if (bus_dmamem_alloc(pci->pci_dmat, len, NBPG, 0, &seg, 1, &rseg,
	      BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(pci->pci_dmat, &seg, rseg, len,
	      (caddr_t *)&isp->isp_result, BUS_DMA_NOWAIT|BUS_DMA_COHERENT))
		return (1);
	if (bus_dmamap_create(pci->pci_dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	      &pci->pci_result_dmap) ||
	    bus_dmamap_load(pci->pci_dmat, pci->pci_result_dmap,
	      (caddr_t)isp->isp_result, len, NULL, BUS_DMA_NOWAIT))
		return (1);
	isp->isp_result_dma = pci->pci_result_dmap->dm_segs[0].ds_addr;

	if (isp->isp_type & ISP_HA_SCSI) {
		return (0);
	}

	fcp = isp->isp_param;
	len = ISP2100_SCRLEN;
	if (bus_dmamem_alloc(pci->pci_dmat, len, NBPG, 0, &seg, 1, &rseg,
		BUS_DMA_NOWAIT) ||
	    bus_dmamem_map(pci->pci_dmat, &seg, rseg, len,
	      (caddr_t *)&fcp->isp_scratch, BUS_DMA_NOWAIT|BUS_DMA_COHERENT))
		return (1);
	if (bus_dmamap_create(pci->pci_dmat, len, 1, len, 0, BUS_DMA_NOWAIT,
	      &pci->pci_scratch_dmap) ||
	    bus_dmamap_load(pci->pci_dmat, pci->pci_scratch_dmap,
	      (caddr_t)fcp->isp_scratch, len, NULL, BUS_DMA_NOWAIT))
		return (1);
	fcp->isp_scdma = pci->pci_scratch_dmap->dm_segs[0].ds_addr;
	return (0);
}

static int
isp_pci_dmasetup(isp, xs, rq, iptrp, optr)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	ispreq_t *rq;
	u_int8_t *iptrp;
	u_int8_t optr;
{
	struct isp_pcisoftc *pci = (struct isp_pcisoftc *)isp;
	bus_dmamap_t dmap = pci->pci_xfer_dmap[rq->req_handle - 1];
	ispcontreq_t *crq;
	int segcnt, seg, error, ovseg, seglim, drq;

	if (xs->datalen == 0) {
		rq->req_seg_count = 1;
		goto mbxsync;
	}

	if (rq->req_handle > RQUEST_QUEUE_LEN || rq->req_handle < 1) {
		panic("%s: bad handle (%d) in isp_pci_dmasetup\n",
		    isp->isp_name, rq->req_handle);
		/* NOTREACHED */
	}

	if (xs->flags & SCSI_DATA_IN) {
		drq = REQFLAG_DATA_IN;
	} else {
		drq = REQFLAG_DATA_OUT;
	}

	if (isp->isp_type & ISP_HA_FC) {
		seglim = ISP_RQDSEG_T2;
		((ispreqt2_t *)rq)->req_totalcnt = xs->datalen;
		((ispreqt2_t *)rq)->req_flags |= drq;
	} else {
		seglim = ISP_RQDSEG;
		rq->req_flags |= drq;
	}
	error = bus_dmamap_load(pci->pci_dmat, dmap, xs->data, xs->datalen,
	    NULL, xs->flags & SCSI_NOSLEEP ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	if (error) {
		XS_SETERR(xs, HBA_BOTCH);
		return (CMD_COMPLETE);
	}

	segcnt = dmap->dm_nsegs;

	for (seg = 0, rq->req_seg_count = 0;
	     seg < segcnt && rq->req_seg_count < seglim;
	     seg++, rq->req_seg_count++) {
		if (isp->isp_type & ISP_HA_FC) {
			ispreqt2_t *rq2 = (ispreqt2_t *)rq;
			rq2->req_dataseg[rq2->req_seg_count].ds_count =
			    dmap->dm_segs[seg].ds_len;
			rq2->req_dataseg[rq2->req_seg_count].ds_base =
			    dmap->dm_segs[seg].ds_addr;
		} else {
			rq->req_dataseg[rq->req_seg_count].ds_count =
			    dmap->dm_segs[seg].ds_len;
			rq->req_dataseg[rq->req_seg_count].ds_base =
			    dmap->dm_segs[seg].ds_addr;
		}
	}

	if (seg == segcnt)
		goto dmasync;

	do {
		crq = (ispcontreq_t *)
			ISP_QUEUE_ENTRY(isp->isp_rquest, *iptrp);
		*iptrp = (*iptrp + 1) & (RQUEST_QUEUE_LEN - 1);
		if (*iptrp == optr) {
			printf("%s: Request Queue Overflow++\n",
			       isp->isp_name);
			bus_dmamap_unload(pci->pci_dmat, dmap);
			XS_SETERR(xs, HBA_BOTCH);
			return (CMD_COMPLETE);
		}
		rq->req_header.rqs_entry_count++;
		bzero((void *)crq, sizeof (*crq));
		crq->req_header.rqs_entry_count = 1;
		crq->req_header.rqs_entry_type = RQSTYPE_DATASEG;

		for (ovseg = 0; seg < segcnt && ovseg < ISP_CDSEG;
		    rq->req_seg_count++, seg++, ovseg++) {
			crq->req_dataseg[ovseg].ds_count =
			    dmap->dm_segs[seg].ds_len;
			crq->req_dataseg[ovseg].ds_base =
			    dmap->dm_segs[seg].ds_addr;
		}
	} while (seg < segcnt);

dmasync:
	bus_dmamap_sync(pci->pci_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->flags & SCSI_DATA_IN) ?  BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

mbxsync:

	bus_dmamap_sync(pci->pci_dmat, pci->pci_rquest_dmap, 0,
	    pci->pci_rquest_dmap->dm_mapsize, BUS_DMASYNC_PREWRITE);
	return (CMD_QUEUED);
}

static int
isp_pci_intr(arg)
	void *arg;
{
	struct isp_pcisoftc *pci = (struct isp_pcisoftc *)arg;
	bus_dmamap_sync(pci->pci_dmat, pci->pci_result_dmap, 0,
	    pci->pci_result_dmap->dm_mapsize, BUS_DMASYNC_POSTREAD);
	return (isp_intr(arg));
}

static void
isp_pci_dmateardown(isp, xs, handle)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	u_int32_t handle;
{
	struct isp_pcisoftc *pci = (struct isp_pcisoftc *)isp;
	bus_dmamap_t dmap = pci->pci_xfer_dmap[handle];

	bus_dmamap_sync(pci->pci_dmat, dmap, 0, dmap->dm_mapsize,
	    xs->flags & SCSI_DATA_IN ?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(pci->pci_dmat, dmap);
}

static void
isp_pci_reset1(isp)
	struct ispsoftc *isp;
{
	/* Make sure the BIOS is disabled */
	isp_pci_wr_reg(isp, HCCR, PCI_HCCR_CMD_BIOS);
}

static void
isp_pci_dumpregs(isp)
	struct ispsoftc *isp;
{
	struct isp_pcisoftc *pci = (struct isp_pcisoftc *)isp;
	printf("%s: PCI Status Command/Status=%x\n", pci->pci_isp.isp_name,
	    pci_conf_read(pci->pci_pc, pci->pci_tag, PCI_COMMAND_STATUS_REG));
}
