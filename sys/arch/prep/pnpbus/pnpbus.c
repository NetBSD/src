/*	$NetBSD: pnpbus.c,v 1.1.2.4 2006/05/24 10:57:10 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pnpbus.c,v 1.1.2.4 2006/05/24 10:57:10 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/residual.h>
#include <machine/pnp.h>
#include <machine/isa_machdep.h>

#include <dev/isa/isareg.h>

#include <prep/pnpbus/pnpbusvar.h>

static int	pnpbus_match(struct device *, struct cfdata *, void *);
static void	pnpbus_attach(struct device *, struct device *, void *);
static int	pnpbus_print(void *, const char *);
static int	pnpbus_search(struct device *, struct cfdata *,
			      const int *, void *);

CFATTACH_DECL(pnpbus, sizeof(struct pnpbus_softc),
    pnpbus_match, pnpbus_attach, NULL, NULL);

struct pnpbus_softc *pnpbus_softc;
extern struct cfdriver pnpbus_cd;

static int
pnpbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

static void
pnpbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct pnpbus_softc *sc = (struct pnpbus_softc *)self;
	struct pnpbus_attach_args *paa = aux;

	printf("\n");

	pnpbus_softc = sc;
	sc->sc_ic = paa->paa_ic;
	sc->sc_iot = paa->paa_iot;
	sc->sc_memt = paa->paa_memt;

	(void)config_search_ia(pnpbus_search, self, "pnpbus", aux);
}

static int
pnp_newirq(void *v, struct pnpresources *r, int size)
{
	struct _S4_Pack *p = v;
	struct pnpbus_irq *irq;

	irq = malloc(sizeof(struct pnpbus_irq), M_DEVBUF, M_NOWAIT);

	irq->mask = le16dec(&p->IRQMask[0]);

	if (size > 2)
		irq->flags = p->IRQInfo;
	else
		irq->flags = 0x1;

	SIMPLEQ_INSERT_TAIL(&r->irq, irq, next);
	r->numirq++;

	return 0;
}

static int
pnp_newdma(void *v, struct pnpresources *r, int size)
{
	struct _S5_Pack *p = v;
	struct pnpbus_dma *dma;

	dma = malloc(sizeof(struct pnpbus_dma), M_DEVBUF, M_NOWAIT);

	dma->mask = le16dec(&p->DMAMask);
	if (size > 2)
		dma->flags = p->DMAInfo;
	else
		dma->flags = 0x01;

	SIMPLEQ_INSERT_TAIL(&r->dma, dma, next);
	r->numdma++;

	return 0;
}

static int
pnp_newioport(void *v, struct pnpresources *r, int size)
{
	struct _S8_Pack *p = v;
	struct pnpbus_io *io;
	uint16_t mask;

	io = malloc(sizeof(struct pnpbus_io), M_DEVBUF, M_NOWAIT);
	mask = p->IOInfo & ISAAddr16bit ? 0xffff : 0x03ff;
	io->minbase = (p->RangeMin[0] | (p->RangeMin[1] << 8)) & mask;
	io->maxbase = (p->RangeMax[0] | (p->RangeMax[1] << 8)) & mask;
	io->align = p->IOAlign;
	io->len = p->IONum;
	io->flags = p->IOInfo;

	SIMPLEQ_INSERT_TAIL(&r->io, io, next);
	r->numio++;

	return 0;
}

static int
pnp_newfixedioport(void *v, struct pnpresources *r, int size)
{
	struct _S9_Pack *p = v;
	struct pnpbus_io *io;

	io = malloc(sizeof(struct pnpbus_io), M_DEVBUF, M_NOWAIT);
	io->minbase = (p->Range[0] | (p->Range[1] << 8)) & 0x3ff;
	io->len = p->IONum;
	io->maxbase = -1;
	io->flags = 0;
	io->align = 1;

	SIMPLEQ_INSERT_TAIL(&r->io, io, next);
	r->numio++;

	return 0;
}

static int
pnp_newaddr(void *v, struct pnpresources *r, int size)
{
	struct pnpbus_io *io;
	struct pnpbus_mem *mem;
	struct _L4_Pack *pack = v;
	struct _L4_PPCPack *p =  &pack->L4_Data.L4_PPCPack;

	if (p->PPCData[0] == 1) {/* type IO */
		io = malloc(sizeof(struct pnpbus_io), M_DEVBUF, M_NOWAIT);
		io->minbase = (uint16_t)le64dec(&p->PPCData[4]);
		io->maxbase = -1;
		io->align = p->PPCData[1];
		io->len = (uint16_t)le64dec(&p->PPCData[12]);
		io->flags = 0;
		SIMPLEQ_INSERT_TAIL(&r->io, io, next);
		r->numio++;

		return 0;
	} else if (p->PPCData[0] == 2) {
		mem = malloc(sizeof(struct pnpbus_mem), M_DEVBUF, M_NOWAIT);
		mem->minbase = (uint32_t)le64dec(&p->PPCData[4]);
		mem->maxbase = -1;
		mem->align = p->PPCData[1];
		mem->len = (uint32_t)le64dec(&p->PPCData[12]);
		mem->flags = 0;
		SIMPLEQ_INSERT_TAIL(&r->mem, mem, next);
		r->nummem++;

		return 0;
	} else 
		return -1;
}

static int
pnp_newcompatid(void *v, struct pnpresources *r, int size)
{
	struct _S3_Pack *p = v;
	struct pnpbus_compatid *id;
	uint32_t cid;

	id = malloc(sizeof(*id), M_DEVBUF, M_NOWAIT);
	cid = le32dec(p->CompatId);
	pnp_devid_to_string(cid, id->idstr);
	id->next = r->compatids;
	r->compatids = id;

	return 0;
}

/*
 * Call if match succeeds.  This way we don't allocate lots of ram
 * for structures we never use if the device isn't attached.
 */

int
pnpbus_scan(struct pnpbus_dev_attach_args *pna, PPC_DEVICE *dev)
{
	struct pnpresources *r = &pna->pna_res;
	uint32_t l;
	uint8_t *p, *q;
	void *v;
	int tag, size, item;

	l = be32toh(dev->AllocatedOffset);
	p = res->DevicePnPHeap + l;

	if (p == NULL)
		return -1;

	for (; p[0] != END_TAG; p += size) {
		tag = *p;
		v = p;
		if (tag_type(p[0]) == PNP_SMALL) {
			size = tag_small_count(tag) + 1;
			item = tag_small_item_name(tag);
			switch (item) {
			case IRQFormat:
				pnp_newirq(v, r, size);
				break;
			case DMAFormat:
				pnp_newdma(v, r, size);
				break;
			case IOPort:
				pnp_newioport(v, r, size);
				break;
			case FixedIOPort:
				pnp_newfixedioport(v, r, size);
				break;
			}
		} else {
			struct _L4_Pack *pack = v;
			struct _L4_PPCPack *pa = &pack->L4_Data.L4_PPCPack;

			q = p;
			size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;
			item = tag_large_item_name(tag);
			if (item == LargeVendorItem &&
			    pa->Type == LV_GenericAddress)
				pnp_newaddr(v, r, size);
		}
	}

	/* scan for compatid's */

	l = be32toh(dev->CompatibleOffset);
	p = res->DevicePnPHeap + l;

	if (p == NULL)
		return -1;

	for (; p[0] != END_TAG; p += size) {
		tag = *p;
		v = p;
		if (tag_type(p[0]) == PNP_SMALL) {
			size = tag_small_count(tag) + 1;
			item = tag_small_item_name(tag);
			switch (item) {
			case CompatibleDevice:
				pnp_newcompatid(v, r, size);
				break;
			}
		} else {
			q = p;
			size = (q[1] | (q[2] << 8)) + 3 /* tag + length */;
		}
	}
	return 0;
}

/*
 * Setup the basic pna structure.
 */

static void
pnp_getpna(struct pnpbus_dev_attach_args *pna, struct pnpbus_attach_args *paa,
	PPC_DEVICE *dev)
{
	uint32_t l;
	DEVICE_ID *id = &dev->DeviceId;
	struct pnpresources *r = &pna->pna_res;

	l = be32toh(dev->AllocatedOffset);

	pna->pna_iot = paa->paa_iot;
	pna->pna_memt = paa->paa_memt;
	pna->pna_ic = paa->paa_ic;
	pnp_devid_to_string(id->DevId, pna->pna_devid);
	pna->pna_ppc_dev = dev;
	memset(r, 0, sizeof(*r));
	SIMPLEQ_INIT(&r->mem);
	SIMPLEQ_INIT(&r->io);
	SIMPLEQ_INIT(&r->irq);
	SIMPLEQ_INIT(&r->dma);
}

static int
pnpbus_search(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct pnpbus_dev_attach_args pna;
	struct pnpbus_attach_args *paa = aux;
	PPC_DEVICE *ppc_dev;
	int i;
	uint32_t ndev;

	ndev = be32toh(res->ActualNumDevices);
	ppc_dev = res->Devices;

	for (i = 0; i < ((ndev > MAX_DEVICES) ? MAX_DEVICES : ndev); i++) {
		pnp_getpna(&pna, paa, &ppc_dev[i]);
		if (config_match(parent, cf, &pna) > 0)
			config_attach(parent, cf, &pna, pnpbus_print);
	}

	return 0;
}

static void
pnpbus_printres(struct pnpresources *r)
{
	struct pnpbus_io *io;
	struct pnpbus_mem *mem;
	struct pnpbus_irq *irq;
	struct pnpbus_dma *dma;
	int p = 0;

	if (!SIMPLEQ_EMPTY(&r->mem)) {
		aprint_normal("mem");
		SIMPLEQ_FOREACH(mem, &r->mem, next) {
			aprint_normal(" 0x%x", mem->minbase);
			if (mem->len > 1)
				aprint_normal("-0x%x",
				    mem->minbase + mem->len - 1);
		}
		p++;
	}
	if (!SIMPLEQ_EMPTY(&r->io)) {
		if (p++)
			aprint_normal(", ");
		aprint_normal("port");
		SIMPLEQ_FOREACH(io, &r->io, next) {
			aprint_normal(" 0x%x", io->minbase);
			if (io->len > 1)
				aprint_normal("-0x%x",
				    io->minbase + io->len - 1);
		}
	}
	if (!SIMPLEQ_EMPTY(&r->irq)) {
		if (p++)
			aprint_normal(", ");
		aprint_normal("irq");
		SIMPLEQ_FOREACH(irq, &r->irq, next) {
			aprint_normal(" %d", ffs(irq->mask) - 1);
		}
	}
	if (!SIMPLEQ_EMPTY(&r->dma)) {
		if (p++)
			aprint_normal(", ");
		aprint_normal("DMA");
		SIMPLEQ_FOREACH(dma, &r->dma, next) {
			aprint_normal(" %d", ffs(dma->mask) - 1);
		}
	}
}

void
pnpbus_print_devres(struct pnpbus_dev_attach_args *pna)
{
	aprint_normal(": ");
	pnpbus_printres(&pna->pna_res);
	aprint_normal("\n");
}

static int
pnpbus_print(void *args, const char *name)
{
	struct pnpbus_dev_attach_args *pna = args;

	pnpbus_print_devres(pna);	
	return (UNCONF);
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
pnpbus_intr_establish(int idx, int level, int (*ih_fun)(void *),
    void *ih_arg, struct pnpresources *r)
{
	struct pnpbus_irq *irq;
	int irqnum, type;

	if (idx >= r->numirq)
		return 0;

	irq = SIMPLEQ_FIRST(&r->irq);
	while (idx--)
		irq = SIMPLEQ_NEXT(irq, next);

	irqnum = ffs(irq->mask) - 1;
	type = (irq->flags & 0x0c) ? IST_LEVEL : IST_EDGE;

	return (void *)intr_establish(irqnum, type, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
pnpbus_intr_disestablish(void *arg)
{

	intr_disestablish(arg);
}

int
pnpbus_getirqnum(struct pnpresources *r, int idx, int *irqp, int *istp)
{
	struct pnpbus_irq *irq;

	if (idx >= r->numirq)
		return EINVAL;

	irq = SIMPLEQ_FIRST(&r->irq);
	while (idx--)
		irq = SIMPLEQ_NEXT(irq, next);

	if (irqp != NULL)
		*irqp = ffs(irq->mask) - 1;
	if (istp != NULL)
		*istp = (irq->flags &0x0c) ? IST_LEVEL : IST_EDGE;
	return 0;
}

int
pnpbus_getdmachan(struct pnpresources *r, int idx, int *chanp)
{
	struct pnpbus_dma *dma;

	if (idx >= r->numdma)
		return EINVAL;

	dma = SIMPLEQ_FIRST(&r->dma);
	while (idx--)
		dma = SIMPLEQ_NEXT(dma, next);

	if (chanp != NULL)
		*chanp = ffs(dma->mask) - 1;
	return 0;
}

int
pnpbus_getioport(struct pnpresources *r, int idx, int *basep, int *sizep)
{
	struct pnpbus_io *io;

	if (idx >= r->numio)
		return EINVAL;

	io = SIMPLEQ_FIRST(&r->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	if (basep)
		*basep = io->minbase;
	if (sizep)
		*sizep = io->len;
	return 0;
}

int
pnpbus_io_map(struct pnpresources *r, int idx, bus_space_tag_t *tagp,
    bus_space_handle_t *hdlp)
{
	struct pnpbus_io *io;

	if (idx >= r->numio)
		return EINVAL;

	io = SIMPLEQ_FIRST(&r->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	*tagp = &prep_isa_io_space_tag;
	return (bus_space_map(&prep_isa_io_space_tag, io->minbase, io->len,
	    0, hdlp));
}

void
pnpbus_io_unmap(struct pnpresources *r, int idx, bus_space_tag_t tag,
    bus_space_handle_t hdl)
{
	struct pnpbus_io *io;

	if (idx >= r->numio)
		return;

	io = SIMPLEQ_FIRST(&r->io);
	while (idx--)
		io = SIMPLEQ_NEXT(io, next);

	bus_space_unmap(tag, hdl, io->len);
}
