/*	$NetBSD: isp_sbus.c,v 1.9 1998/03/21 20:14:14 pk Exp $	*/

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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>
#include <sparc/cpu.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/dev/sbusvar.h>

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/microcode/isp/asm_sbus.h>
#include <machine/param.h>
#include <machine/vmparam.h>


static u_int16_t isp_sbus_rd_reg __P((struct ispsoftc *, int));
static void isp_sbus_wr_reg __P((struct ispsoftc *, int, u_int16_t));
static int isp_sbus_mbxdma __P((struct ispsoftc *));
static int isp_sbus_dmasetup __P((struct ispsoftc *, struct scsipi_xfer *,
	ispreq_t *, u_int8_t *, u_int8_t));
static void isp_sbus_dmateardown __P((struct ispsoftc *, struct scsipi_xfer *,
	u_int32_t));

static struct ispmdvec mdvec = {
	isp_sbus_rd_reg,
	isp_sbus_wr_reg,
	isp_sbus_mbxdma,
	isp_sbus_dmasetup,
	isp_sbus_dmateardown,
	NULL,
	NULL,
	NULL,
	ISP_RISC_CODE,
	ISP_CODE_LENGTH,
	ISP_CODE_ORG,
	ISP_CODE_VERSION,
	0,
	0
};

struct isp_sbussoftc {
	struct ispsoftc	sbus_isp;
	sdparam		sbus_dev;
	bus_space_tag_t	sbus_bustag;
	volatile u_char	*sbus_reg;
	int		sbus_node;
	int		sbus_pri;
	vm_offset_t	sbus_kdma_allocs[MAXISPREQUEST];
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
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0 ||
		strcmp("SUNW,isp", sa->sa_name) == 0 ||
		strcmp("QLGC,isp", sa->sa_name) == 0);
}

static void
isp_sbus_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) self;

	sbc->sbus_bustag = sa->sa_bustag;
	sbc->sbus_pri = sa->sa_pri;
#if 0
	printf(" pri %d\n", sbc->sbus_pri);
#endif

	if (sa->sa_promvaddr) {
		sbc->sbus_reg = (volatile u_char *) sa->sa_promvaddr;
	} else {
		bus_space_handle_t bh;
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset,
				 sa->sa_size,
				 0, 0, &bh) != 0) {
			printf("%s: cannot map registers\n", self->dv_xname);
			return;
		}
		sbc->sbus_reg = (volatile u_char *)bh;
	}
	sbc->sbus_node = sa->sa_node;

	sbc->sbus_isp.isp_mdvec = &mdvec;
	sbc->sbus_isp.isp_type = ISP_HA_SCSI_UNKNOWN;
	sbc->sbus_isp.isp_param = &sbc->sbus_dev;
	bzero(sbc->sbus_isp.isp_param, sizeof (sdparam));
	isp_reset(&sbc->sbus_isp);
	if (sbc->sbus_isp.isp_state != ISP_RESETSTATE) {
		return;
	}
	isp_init(&sbc->sbus_isp);
	if (sbc->sbus_isp.isp_state != ISP_INITSTATE) {
		isp_uninit(&sbc->sbus_isp);
		return;
	}

	/* Establish interrupt channel */
	bus_intr_establish(sbc->sbus_bustag,
			   sbc->sbus_pri, 0,
			   (int(*)__P((void*)))isp_intr, sbc);

	/*
	 * Do Generic attach now.
	 */
	isp_attach(&sbc->sbus_isp);
	if (sbc->sbus_isp.isp_state != ISP_RUNSTATE) {
		isp_uninit(&sbc->sbus_isp);
	}
}

#define  SBUS_BIU_REGS_OFF		0x00
#define	 SBUS_MBOX_REGS_OFF		0x80
#define	 SBUS_SXP_REGS_OFF		0x200
#define	 SBUS_RISC_REGS_OFF		0x400

static u_int16_t
isp_sbus_rd_reg(isp, regoff)
	struct ispsoftc *isp;
	int regoff;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;

	int offset;
	if ((regoff & BIU_BLOCK) != 0) {
		offset = SBUS_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		offset = SBUS_MBOX_REGS_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = SBUS_SXP_REGS_OFF;
	} else {
		offset = SBUS_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	return (*((u_int16_t *) &sbc->sbus_reg[offset]));
}

static void
isp_sbus_wr_reg (isp, regoff, val)
	struct ispsoftc *isp;
	int regoff;
	u_int16_t val;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	int offset;

	if ((regoff & BIU_BLOCK) != 0) {
		offset = SBUS_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		offset = SBUS_MBOX_REGS_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = SBUS_SXP_REGS_OFF;
	} else {
		offset = SBUS_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	*((u_int16_t *) &sbc->sbus_reg[offset]) = val;
}

static int
isp_sbus_mbxdma(isp)
	struct ispsoftc *isp;
{
	size_t len;

	/*
	 * NOTE: Since most Sun machines aren't I/O coherent,
	 * map the mailboxes through kdvma space to force them
	 * to be uncached.
	 */

	/*
	 * Allocate and map the request queue.
	 */
	len = ISP_QUEUE_SIZE(RQUEST_QUEUE_LEN(isp));
	isp->isp_rquest = (volatile caddr_t)malloc(len, M_DEVBUF, M_NOWAIT);
	if (isp->isp_rquest == 0)
		return (1);
	isp->isp_rquest_dma = (u_int32_t)kdvma_mapin((caddr_t)isp->isp_rquest,
	    len, 0);
	if (isp->isp_rquest_dma == 0)
		return (1);

	/*
	 * Allocate and map the result queue.
	 */
	len = ISP_QUEUE_SIZE(RESULT_QUEUE_LEN(isp));
	isp->isp_result = (volatile caddr_t)malloc(len, M_DEVBUF, M_NOWAIT);
	if (isp->isp_result == 0)
		return (1);
	isp->isp_result_dma = (u_int32_t)kdvma_mapin((caddr_t)isp->isp_result,
	    len, 0);
	if (isp->isp_result_dma == 0)
		return (1);

	return (0);
}

/*
 * TODO: If kdvma_mapin fails, try using multiple smaller chunks..
 */

static int
isp_sbus_dmasetup(isp, xs, rq, iptrp, optr)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	ispreq_t *rq;
	u_int8_t *iptrp;
	u_int8_t optr;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	vm_offset_t kdvma;
	int dosleep = (xs->flags & SCSI_NOSLEEP) != 0;

	if (xs->datalen == 0) {
		rq->req_seg_count = 1;
		return (0);
	}

	if (rq->req_handle > RQUEST_QUEUE_LEN(isp) ||
	    rq->req_handle < 1) {
		panic("%s: bad handle (%d) in isp_sbus_dmasetup\n",
			isp->isp_name, rq->req_handle);
		/* NOTREACHED */
	}
	if (CPU_ISSUN4M) {
		kdvma = (vm_offset_t)
			kdvma_mapin((caddr_t)xs->data, xs->datalen, dosleep);
		if (kdvma == (vm_offset_t) 0) {
			return (1);
		}
	} else {
		kdvma = (vm_offset_t) xs->data;
	}

	if (sbc->sbus_kdma_allocs[rq->req_handle - 1] != (vm_offset_t) 0) {
		panic("%s: kdma handle already allocated\n", isp->isp_name);
		/* NOTREACHED */
	}
	sbc->sbus_kdma_allocs[rq->req_handle - 1] = kdvma;
	if (xs->flags & SCSI_DATA_IN) {
		rq->req_flags |= REQFLAG_DATA_IN;
	} else {
		rq->req_flags |= REQFLAG_DATA_OUT;
	}
	rq->req_dataseg[0].ds_count = xs->datalen;
	rq->req_dataseg[0].ds_base =  (u_int32_t) kdvma;
	rq->req_seg_count = 1;
	return (0);
}

static void
isp_sbus_dmateardown(isp, xs, handle)
	struct ispsoftc *isp;
	struct scsipi_xfer *xs;
	u_int32_t handle;
{
	struct isp_sbussoftc *sbc = (struct isp_sbussoftc *) isp;
	vm_offset_t kdvma;

	if (xs->flags & SCSI_DATA_IN) {
		cpuinfo.cache_flush(xs->data, xs->datalen - xs->resid);
	}

	if (handle >= RQUEST_QUEUE_LEN(isp)) {
		panic("%s: bad handle (%d) in isp_sbus_dmateardown\n",
			isp->isp_name, handle);
		/* NOTREACHED */
	}
	if (sbc->sbus_kdma_allocs[handle] == (vm_offset_t) 0) {
		panic("%s: kdma handle not already allocated\n", isp->isp_name);
		/* NOTREACHED */
	}
	kdvma = sbc->sbus_kdma_allocs[handle];
	sbc->sbus_kdma_allocs[handle] = (vm_offset_t) 0;
	if (CPU_ISSUN4M) {
		dvma_mapout(kdvma, (vm_offset_t) xs->data, xs->datalen);
	}
}
