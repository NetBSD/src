/*	$NetBSD: isp_pci.c,v 1.7 1997/03/28 21:51:51 cgd Exp $	*/

/*
 * PCI specific probe and attach routines for Qlogic ISP SCSI adapters.
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
#include <machine/bus.h>
#include <machine/intr.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <vm/vm.h>

#include <dev/ic/ispreg.h>
#include <dev/ic/ispvar.h>
#include <dev/ic/ispmbox.h>
#include <dev/microcode/isp/asm_pci.h>

#ifdef	__alpha__	/* XXX */
/* XXX XXX NEED REAL DMA MAPPING SUPPORT XXX XXX */
extern vm_offset_t alpha_XXX_dmamap(vm_offset_t);
#undef vtophys
#define	vtophys(va)	alpha_XXX_dmamap((vm_offset_t) va)
#endif
#define	KVTOPHYS(x)	vtophys(x)


static u_int16_t isp_pci_rd_reg __P((struct ispsoftc *, int));
static void isp_pci_wr_reg __P((struct ispsoftc *, int, u_int16_t));
static vm_offset_t
isp_pci_mbxdma __P((struct ispsoftc *, vm_offset_t, u_int32_t));
static int
isp_pci_dmasetup __P((struct ispsoftc *, struct scsi_xfer *, ispreq_t *,
		      u_int8_t *, u_int8_t));

static void isp_pci_reset1 __P((struct ispsoftc *));

static struct ispmdvec mdvec = {
	isp_pci_rd_reg,
	isp_pci_wr_reg,
	isp_pci_mbxdma,
	isp_pci_dmasetup,
	NULL,
	NULL,
	isp_pci_reset1,
	ISP_RISC_CODE,
	ISP_CODE_LENGTH,
	ISP_CODE_ORG,
/*	BIU_PCI_CONF1_FIFO_16 | BIU_BURST_ENABLE */ 0
};

#define	PCI_QLOGIC_ISP	\
	((PCI_PRODUCT_QLOGIC_ISP1020 << 16) | PCI_VENDOR_QLOGIC)

#define IO_MAP_REG	0x10
#define MEM_MAP_REG	0x14

int isp_pci_prefer_io = 0;	/* 1 -> map via I/O (patchable data) */


#ifdef	__BROKEN_INDIRECT_CONFIG
static int isp_pci_probe __P((struct device *, void *, void *));
#else
static int isp_pci_probe __P((struct device *, struct cfdata *, void *));
#endif
static void isp_pci_attach __P((struct device *, struct device *, void *));

struct isp_pcisoftc {
	struct ispsoftc		pci_isp;
	bus_space_tag_t		pci_st;
	bus_space_handle_t	pci_sh;
	void *			pci_ih;
};

struct cfattach isp_pci_ca = {
	sizeof (struct isp_pcisoftc), isp_pci_probe, isp_pci_attach
};

static int
isp_pci_probe(parent, match, aux)
        struct device *parent;
#ifdef	__BROKEN_INDIRECT_CONFIG
        void *match, *aux; 
#else
        struct cfdata *match;
	void *aux; 
#endif
{       
        struct pci_attach_args *pa = aux;

	if (pa->pa_id == PCI_QLOGIC_ISP) {
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
	struct pci_attach_args *pa = aux;
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) self;
	bus_addr_t busbase;
	bus_size_t bussize;
	bus_space_tag_t st;
	bus_space_handle_t sh;
	pci_intr_handle_t ih;
	const char *intrstr;
	int cacheable;

	if (isp_pci_prefer_io) {
		if (pci_io_find(pa->pa_pc, pa->pa_tag, IO_MAP_REG, &busbase,
		    &bussize)) {
			printf(": unable to find PCI I/O base\n");
			return;
		}
		st = pa->pa_iot;
		if (bus_space_map(st, busbase, bussize, 0, &sh)) {
			printf(": unable to map I/O registers\n");
			return;
		}
	} else {
		if (pci_mem_find(pa->pa_pc, pa->pa_tag, MEM_MAP_REG, &busbase,
		    &bussize, &cacheable)) {
			printf(": unable to find PCI memory base\n");
			return;
		}
		st = pa->pa_memt;
		if (bus_space_map(st, busbase, bussize, 0, &sh)) {
			printf(": unable to map memory registers\n");
			return;
		}
	}
	printf("\n");

	pcs->pci_st = st;
	pcs->pci_sh = sh;
	pcs->pci_isp.isp_mdvec = &mdvec;
	isp_reset(&pcs->pci_isp);
	if (pcs->pci_isp.isp_state != ISP_RESETSTATE) {
		return;
	}
	isp_init(&pcs->pci_isp);
	if (pcs->pci_isp.isp_state != ISP_INITSTATE) {
		isp_uninit(&pcs->pci_isp);
		return;
	}

	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", pcs->pci_isp.isp_name);
		isp_uninit(&pcs->pci_isp);
		return;
	}

	intrstr = pci_intr_string(pa->pa_pc, ih);
	if (intrstr == NULL)
		intrstr = "<I dunno>";
	pcs->pci_ih =
	  pci_intr_establish(pa->pa_pc, ih, IPL_BIO, isp_intr, &pcs->pci_isp);
	if (pcs->pci_ih == NULL) {
		printf("%s: couldn't establish interrupt at %s\n",
			pcs->pci_isp.isp_name, intrstr);
		isp_uninit(&pcs->pci_isp);
		return;
	}
	printf("%s: interrupting at %s\n", pcs->pci_isp.isp_name, intrstr);

	/*
	 * Do Generic attach now.
	 */
	isp_attach(&pcs->pci_isp);
	if (pcs->pci_isp.isp_state != ISP_RUNSTATE) {
		isp_uninit(&pcs->pci_isp);
	}
}

#define  PCI_BIU_REGS_OFF		0x00
#define	 PCI_MBOX_REGS_OFF		0x70
#define	 PCI_SXP_REGS_OFF		0x80
#define	 PCI_RISC_REGS_OFF		0x80

static u_int16_t
isp_pci_rd_reg(isp, regoff)
	struct ispsoftc *isp;
	int regoff;
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int offset;
	if ((regoff & BIU_BLOCK) != 0) {
		offset = PCI_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		offset = PCI_MBOX_REGS_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = PCI_SXP_REGS_OFF;
		/*
		 * XXX
		 */
		panic("SXP Registers not accessible yet!");
	} else {
		offset = PCI_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	return bus_space_read_2(pcs->pci_st, pcs->pci_sh, offset);
}

static void
isp_pci_wr_reg(isp, regoff, val)
	struct ispsoftc *isp;
	int regoff;
	u_int16_t val;
{
	struct isp_pcisoftc *pcs = (struct isp_pcisoftc *) isp;
	int offset;
	if ((regoff & BIU_BLOCK) != 0) {
		offset = PCI_BIU_REGS_OFF;
	} else if ((regoff & MBOX_BLOCK) != 0) {
		offset = PCI_MBOX_REGS_OFF;
	} else if ((regoff & SXP_BLOCK) != 0) {
		offset = PCI_SXP_REGS_OFF;
		/*
		 * XXX
		 */
		panic("SXP Registers not accessible yet!");
	} else {
		offset = PCI_RISC_REGS_OFF;
	}
	regoff &= 0xff;
	offset += regoff;
	bus_space_write_2(pcs->pci_st, pcs->pci_sh, offset, val);
}

static vm_offset_t
isp_pci_mbxdma(isp, kva, len)
	struct ispsoftc *isp;
	vm_offset_t kva;
	u_int32_t len;
{
	vm_offset_t pg, start, s1;

	start = KVTOPHYS(kva);

	pg = kva + NBPG;
	s1 = (start >> PGSHIFT) + 1;
	len -= NBPG;

	while ((int32_t)len > 0) {
		if (s1 != (KVTOPHYS(pg) >> PGSHIFT)) {
			printf("%s: mailboxes across noncontiguous pages\n",
					isp->isp_name);
			return ((vm_offset_t) 0);
		}
		len -= NBPG;
		pg += NBPG;
		s1++;
	}
	return (start);
}

/*
 * TODO: reduce the number of segments by
 *	cchecking for adjacent physical page.
 */

static int
isp_pci_dmasetup(isp, xs, rq, iptrp, optr)
	struct ispsoftc *isp;
	struct scsi_xfer *xs;
	ispreq_t *rq;
	u_int8_t *iptrp;
	u_int8_t optr;
{
	ispcontreq_t *crq;
	unsigned long thiskv, nextkv;
	int datalen, amt;

	if (xs->datalen == 0) {
		rq->req_seg_count = 1;
		rq->req_flags |= REQFLAG_DATA_IN;
		return (0);
	}

	if (xs->flags & SCSI_DATA_IN) {
		rq->req_flags |= REQFLAG_DATA_IN;
	} else {
		rq->req_flags |= REQFLAG_DATA_OUT;
	}
	datalen = xs->datalen;
	thiskv = (unsigned long) xs->data;

	while (datalen && rq->req_seg_count < ISP_RQDSEG) {
		nextkv = (thiskv + NBPG) & ~(NBPG-1);
		amt = nextkv - thiskv;
		if (amt > datalen)
			amt = datalen;
		rq->req_dataseg[rq->req_seg_count].ds_count = amt;
		rq->req_dataseg[rq->req_seg_count].ds_base = KVTOPHYS(thiskv);
#if	0
		printf("%s: seg%d: 0x%lx..0x%lx\n", isp->isp_name,
		       rq->req_seg_count, thiskv,
		       thiskv + (unsigned long) amt);
#endif
		datalen -= amt;
		thiskv = nextkv;
		rq->req_seg_count++;
	}

	if (datalen == 0) {
		return (0);
	}

	do {
		int seg;
		crq = (ispcontreq_t *) &isp->isp_rquest[*iptrp][0];
		*iptrp = (*iptrp + 1) & (RQUEST_QUEUE_LEN - 1);
		if (*iptrp == optr) {
			printf("%s: Request Queue Overflow++\n",
			       isp->isp_name);
			return (1);
		}
		rq->req_header.rqs_entry_count++;
		bzero((void *)crq, sizeof (*crq));
		crq->req_header.rqs_entry_count = 1;
		crq->req_header.rqs_entry_type = RQSTYPE_DATASEG;
		seg = 0;
		while (datalen && seg < ISP_CDSEG) {
			nextkv = (thiskv + NBPG) & ~(NBPG-1);
			amt = nextkv - thiskv;
			if (amt > datalen)
				amt = datalen;
			crq->req_dataseg[seg].ds_count = amt;
			crq->req_dataseg[seg].ds_base = KVTOPHYS(thiskv);
#if	0
			printf("%s: Cont%d seg%d: 0x%lx..0x%lx\n",
			      isp->isp_name, rq->req_header.rqs_entry_count,
			       seg, thiskv, thiskv + (unsigned long) amt);
#endif
			datalen -= amt;
			thiskv = nextkv;
			rq->req_seg_count++;
			seg++;
		}
	} while (datalen > 0);
	return (0);
}

static void
isp_pci_reset1(isp)
	struct ispsoftc *isp;
{
	/* Make sure the BIOS is disabled */
	isp_pci_wr_reg(isp, HCCR, PCI_HCCR_CMD_BIOS);
}
