/* $NetBSD: isp_sbus.c,v 1.13.2.1 1999/12/27 18:35:33 wrstuden Exp $ */
/*
 * SBus specific probe and attach routines for Qlogic ISP SCSI adapters.
 *
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center
 * All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/param.h>
#include <machine/vmparam.h>

#include <dev/ic/isp_netbsd.h>
#include <dev/microcode/isp/asm_sbus.h>
#include <dev/sbus/sbusvar.h>

static u_int16_t isp_sbus_rd_reg __P((struct ispsoftc *, int));
static void isp_sbus_wr_reg __P((struct ispsoftc *, int, u_int16_t));
static int isp_sbus_mbxdma __P((struct ispsoftc *));
static int isp_sbus_dmasetup __P((struct ispsoftc *, struct scsipi_xfer *,
	ispreq_t *, u_int16_t *, u_int16_t));
static void isp_sbus_dmateardown __P((struct ispsoftc *, struct scsipi_xfer *,
	u_int32_t));

#ifndef	ISP_1000_RISC_CODE
#define	ISP_1000_RISC_CODE	NULL
#endif
#ifndef	ISP_CODE_ORG
#define	ISP_CODE_ORG	0x1000
#endif

static struct ispmdvec mdvec = {
	isp_sbus_rd_reg,
	isp_sbus_wr_reg,
	isp_sbus_mbxdma,
	isp_sbus_dmasetup,
	isp_sbus_dmateardown,
	NULL,
	NULL,
	NULL,
	ISP_1000_RISC_CODE, 0, ISP_CODE_ORG, 0,
	BIU_BURST_ENABLE
};

struct isp_sbussoftc {
	struct ispsoftc	sbus_isp;
	sdparam		sbus_dev;
	bus_space_tag_t	sbus_bustag;
	bus_dma_tag_t	sbus_dmatag;
	bus_space_handle_t sbus_reg;
	int		sbus_node;
	int		sbus_pri;
	struct ispmdvec	sbus_mdvec;
	bus_dmamap_t	*sbus_dmamap;
	int16_t		sbus_poff[_NREG_BLKS];
};


static int isp_match __P((struct device *, struct cfdata *, void *));
static void isp_sbus_attach __P((struct device *, struct device *, void *));
struct cfattach isp_sbus_ca = {
	sizeof (struct isp_sbussoftc), isp_match, isp_sbus_attach
};

static int
isp_match(parent, cf, aux)
        struct device *parent;
        struct cfdata *cf;
        void *aux;
{
	int rv;
#ifdef DEBUG
	static int oneshot = 1;
#endif
	struct sbus_attach_args *sa = aux;

	rv = (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0 ||
		strcmp("PTI,ptisp", sa->sa_name) == 0 ||
		strcmp("ptisp", sa->sa_name) == 0 ||
		strcmp("SUNW,isp", sa->sa_name) == 0 ||
		strcmp("QLGC,isp", sa->sa_name) == 0);
#ifdef DEBUG
	if (rv && oneshot) {
		oneshot = 0;
		printf("Qlogic ISP Driver, NetBSD (sbus) Platform Version "
		    "%d.%d Core Version %d.%d\n",
		    ISP_PLATFORM_VERSION_MAJOR, ISP_PLATFORM_VERSION_MINOR,
		    ISP_CORE_VERSION_MAJOR, ISP_CORE_VERSION_MINOR);
	}
#endif
	return (rv);
}

static void
isp_sbus_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	int i, freq;
	struct sbus_attach_args *sa = aux;
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) self;
	struct ispsoftc *isp = &sbc->sbus_isp;
	ISP_LOCKVAL_DECL;

	printf(" for %s\n", sa->sa_name);

	sbc->sbus_bustag = sa->sa_bustag;
	sbc->sbus_dmatag = sa->sa_dmatag;
	if (sa->sa_nintr != 0)
		sbc->sbus_pri = sa->sa_pri;
	sbc->sbus_mdvec = mdvec;

	if (sa->sa_npromvaddrs != 0) {
		sbc->sbus_reg = (bus_space_handle_t)sa->sa_promvaddrs[0];
	} else {
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot, sa->sa_offset,
				 sa->sa_size, BUS_SPACE_MAP_LINEAR, 0,
				 &sbc->sbus_reg) != 0) {
			printf("%s: cannot map registers\n", self->dv_xname);
			return;
		}
	}
	sbc->sbus_node = sa->sa_node;

	freq = getpropint(sa->sa_node, "clock-frequency", 0);
	if (freq) {
		/*
		 * Convert from HZ to MHz, rounding up.
		 */
		freq = (freq + 500000)/1000000;
#if	0
		printf("%s: %d MHz\n", self->dv_xname, freq);
#endif
	}
	sbc->sbus_mdvec.dv_clock = freq;

	/*
	 * XXX: Now figure out what the proper burst sizes, etc., to use.
	 */
	sbc->sbus_mdvec.dv_conf1 |= BIU_SBUS_CONF1_FIFO_8;

	/*
	 * Some early versions of the PTI SBus adapter
	 * would fail in trying to download (via poking)
	 * FW. We give up on them.
	 */
	if (strcmp("PTI,ptisp", sa->sa_name) == 0 ||
	    strcmp("ptisp", sa->sa_name) == 0) {
		sbc->sbus_mdvec.dv_fwlen = 0;
	}

	isp->isp_mdvec = &sbc->sbus_mdvec;
	isp->isp_bustype = ISP_BT_SBUS;
	isp->isp_type = ISP_HA_SCSI_UNKNOWN;
	isp->isp_param = &sbc->sbus_dev;
	bzero(isp->isp_param, sizeof (sdparam));

	sbc->sbus_poff[BIU_BLOCK >> _BLK_REG_SHFT] = BIU_REGS_OFF;
	sbc->sbus_poff[MBOX_BLOCK >> _BLK_REG_SHFT] = SBUS_MBOX_REGS_OFF;
	sbc->sbus_poff[SXP_BLOCK >> _BLK_REG_SHFT] = SBUS_SXP_REGS_OFF;
	sbc->sbus_poff[RISC_BLOCK >> _BLK_REG_SHFT] = SBUS_RISC_REGS_OFF;
	sbc->sbus_poff[DMA_BLOCK >> _BLK_REG_SHFT] = DMA_REGS_OFF;

	isp->isp_confopts = self->dv_cfdata->cf_flags;
	ISP_LOCK(isp);
	isp_reset(isp);
	if (isp->isp_state != ISP_RESETSTATE) {
		ISP_UNLOCK(isp);
		return;
	}
	isp_init(isp);
	if (isp->isp_state != ISP_INITSTATE) {
		isp_uninit(isp);
		ISP_UNLOCK(isp);
		return;
	}

	for (i = 0; i < isp->isp_maxcmds; i++) {
		/* Allocate a DMA handle */
		if (bus_dmamap_create(sbc->sbus_dmatag, MAXPHYS, 1, MAXPHYS, 0,
		    BUS_DMA_NOWAIT, &sbc->sbus_dmamap[i]) != 0) {
			printf("%s: DMA map create error\n",
				self->dv_xname);
			return;
		}
	}

	/* Establish interrupt channel */
	bus_intr_establish(sbc->sbus_bustag, sbc->sbus_pri, 0,
	    (int(*)__P((void*)))isp_intr, sbc);

	/*
	 * do generic attach.
	 */
	isp_attach(isp);
	if (isp->isp_state != ISP_RUNSTATE) {
		isp_uninit(isp);
	}
	ISP_UNLOCK(isp);
}

static u_int16_t
isp_sbus_rd_reg(isp, regoff)
	struct ispsoftc *isp;
	int regoff;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	int offset = sbc->sbus_poff[(regoff & _BLK_REG_MASK) >> _BLK_REG_SHFT];
	offset += (regoff & 0xff);
	return (bus_space_read_2(sbc->sbus_bustag, sbc->sbus_reg, offset));
}

static void
isp_sbus_wr_reg(isp, regoff, val)
	struct ispsoftc *isp;
	int regoff;
	u_int16_t val;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	int offset = sbc->sbus_poff[(regoff & _BLK_REG_MASK) >> _BLK_REG_SHFT];
	offset += (regoff & 0xff);
	bus_space_write_2(sbc->sbus_bustag, sbc->sbus_reg, offset, val);
}

static int
isp_sbus_mbxdma(isp)
	struct ispsoftc *isp;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	bus_dma_segment_t seg;
	int rseg;
	size_t n;
	bus_size_t len;

	if (isp->isp_rquest_dma)
		return (0);

	n = sizeof (ISP_SCSI_XFER_T **) * isp->isp_maxcmds;
	isp->isp_xflist = (ISP_SCSI_XFER_T **) malloc(n, M_DEVBUF, M_WAITOK);
	if (isp->isp_xflist == NULL) {
		printf("%s: cannot alloc xflist array\n", isp->isp_name);
		return (1);
	}
	bzero(isp->isp_xflist, n);
	n = sizeof (bus_dmamap_t) * isp->isp_maxcmds;
	sbc->sbus_dmamap = (bus_dmamap_t *) malloc(n, M_DEVBUF, M_WAITOK);
	if (sbc->sbus_dmamap == NULL) {
		free(isp->isp_xflist, M_DEVBUF);
		printf("%s: cannot alloc dmamap array\n", isp->isp_name);
		return (1);
	}
	/*
	 * Allocate and map the request queue.
	 */
	len = ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN);
	if (bus_dmamem_alloc(sbc->sbus_dmatag, len, NBPG, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		free(sbc->sbus_dmamap, M_DEVBUF);
		free(isp->isp_xflist, M_DEVBUF);
		return (1);
	}

	if (bus_dmamem_map(sbc->sbus_dmatag, &seg, rseg, len,
	    (caddr_t *)&isp->isp_rquest, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		free(sbc->sbus_dmamap, M_DEVBUF);
		free(isp->isp_xflist, M_DEVBUF);
		return (1);
	}
	isp->isp_rquest_dma = seg.ds_addr;

	/*
	 * Allocate and map the result queue.
	 */
	len = ISP_QUEUE_SIZE(RESULT_QUEUE_LEN);
	if (bus_dmamem_alloc(sbc->sbus_dmatag, len, NBPG, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) {
		free(sbc->sbus_dmamap, M_DEVBUF);
		free(isp->isp_xflist, M_DEVBUF);
		return (1);
	}
	if (bus_dmamem_map(sbc->sbus_dmatag, &seg, rseg, len,
	    (caddr_t *)&isp->isp_result, BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) {
		free(sbc->sbus_dmamap, M_DEVBUF);
		free(isp->isp_xflist, M_DEVBUF);
		return (1);
	}
	isp->isp_result_dma = seg.ds_addr;
	return (0);
}

/*
 * Map a DMA request.
 * We're guaranteed that rq->req_handle is a value from 1 to isp->isp_maxcmds.
 */

static int
isp_sbus_dmasetup(isp, xs, rq, iptrp, optr)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	ispreq_t *rq;
	u_int16_t *iptrp;
	u_int16_t optr;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	bus_dmamap_t dmamap;
	int dosleep = (xs->xs_control & XS_CTL_NOSLEEP) != 0;
	int in = (xs->xs_control & XS_CTL_DATA_IN) != 0;

	if (xs->datalen == 0) {
		rq->req_seg_count = 1;
		goto mbxsync;
	}
	assert(rq->req_handle != 0 && rq->req_handle <= isp->isp_maxcmds);
	dmamap = sbc->sbus_dmamap[rq->req_handle - 1];
	if (dmamap->dm_nsegs != 0) {
		panic("%s: dma map already allocated\n", isp->isp_name);
		/* NOTREACHED */
	}
	if (bus_dmamap_load(sbc->sbus_dmatag, dmamap, xs->data, xs->datalen,
	    NULL, dosleep ? BUS_DMA_WAITOK : BUS_DMA_NOWAIT) != 0) {
		XS_SETERR(xs, HBA_BOTCH);
		return (CMD_COMPLETE);
	}
	bus_dmamap_sync(sbc->sbus_dmatag, dmamap, dmamap->dm_segs[0].ds_addr,
	    xs->datalen, in? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	if (in) {
		rq->req_flags |= REQFLAG_DATA_IN;
	} else {
		rq->req_flags |= REQFLAG_DATA_OUT;
	}
	rq->req_dataseg[0].ds_count = xs->datalen;
	rq->req_dataseg[0].ds_base =  dmamap->dm_segs[0].ds_addr;
	rq->req_seg_count = 1;
mbxsync:
        ISP_SWIZZLE_REQUEST(isp, rq);
#if	0
	/*
	 * If we ever map cacheable memory, we need to do something like this.
	 */
        bus_dmamap_sync(sbc->sbus_dmat, sbc->sbus_rquest_dmap, 0,
            sbc->sbus_rquest_dmap->dm_mapsize, BUS_DMASYNC_PREWRITE);
#endif
	return (CMD_QUEUED);
}

static void
isp_sbus_dmateardown(isp, xs, handle)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	u_int32_t handle;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	bus_dmamap_t dmamap;
	assert(handle != 0 && handle <= isp->isp_maxcmds);
	dmamap = sbc->sbus_dmamap[handle-1];
	if (dmamap->dm_nsegs == 0) {
		panic("%s: dma map not already allocated\n", isp->isp_name);
		/* NOTREACHED */
	}
	bus_dmamap_sync(sbc->sbus_dmatag, dmamap, dmamap->dm_segs[0].ds_addr,
	    xs->datalen, (xs->xs_control & XS_CTL_DATA_IN)?
	    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sbc->sbus_dmatag, dmamap);
}
