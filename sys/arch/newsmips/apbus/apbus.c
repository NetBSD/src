/*	$NetBSD: apbus.c,v 1.3 2000/10/12 03:11:38 onoe Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#define _NEWSMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <newsmips/apbus/apbusvar.h>

static int  apbusmatch __P((struct device *, struct cfdata *, void *));
static void apbusattach __P((struct device *, struct device *, void *));
static int apbusprint __P((void *, const char *));
/* static void *aptokseg0 __P((void *)); */

#define	MAXAPDEVNUM	32

struct apbus_softc {
	struct device apbs_dev;
};

struct cfattach ap_ca = {
	sizeof(struct apbus_softc), apbusmatch, apbusattach
};

#define	APBUS_DEVNAMELEN	16

struct ap_intrhand {
	struct ap_intrhand *ai_next;
	int ai_mask;
	int ai_priority;
	int (*ai_func) __P((void*));	/* function */
	void *ai_aux;			/* softc */
	char ai_name[APBUS_DEVNAMELEN];
	int ai_ctlno;
};

#define	NLEVEL	2

static struct ap_intrhand *apintr[NLEVEL];

static int
apbusmatch(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "ap") != 0)
		return 0;

	return 1;
}


static void
apbusattach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct apbus_attach_args child;
	struct apbus_dev *apdev;
	struct apbus_ctl *apctl;

	*(volatile u_int *)(NEWS5000_APBUS_INTST) = 0xffffffff;
	*(volatile u_int *)(NEWS5000_APBUS_INTMSK) = 0xffffffff;
	*(volatile u_int *)(NEWS5000_APBUS_CTRL) = 0x00000004;
	*(volatile u_int *)(NEWS5000_APBUS_DMA) = 0xffffffff;

	printf("\n");

	/*
	 * get first ap-device
	 */
	apdev = apbus_lookupdev(NULL);

	/*
	 * trace device chain
	 */
	while (apdev) {
		apctl = apdev->apbd_ctl;

		/*
		 * probe physical device only
		 * (no pseudo device)
		 */
		if (apctl && apctl->apbc_hwbase) {
			/*
			 * ... and, all units
			 */
			while (apctl) {
				/* make apbus_attach_args for devices */
				child.apa_name = apdev->apbd_name;
				child.apa_ctlnum = apctl->apbc_ctlno;
				child.apa_slotno = apctl->apbc_sl;
				child.apa_hwbase = apctl->apbc_hwbase;

				config_found(self, &child, apbusprint);

				apctl = apctl->apbc_link;
			}
		}

		apdev = apdev->apbd_link;
	}
}

int
apbusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct apbus_attach_args *a = aux;

	if (pnp)
		printf("%s at %s slot%d addr 0x%lx",
			a->apa_name, pnp, a->apa_slotno, a->apa_hwbase);

	return UNCONF;
}

#if 0
void *
aptokseg0(va)
	void *va;
{
	vaddr_t addr = (vaddr_t)va;

	if (addr >= 0xfff00000) {
		addr -= 0xfff00000;
		addr += physmem << PGSHIFT;
		addr += 0x80000000;
		va = (void *)addr;
	}
	return va;
}
#endif

void
apbus_wbflush()
{
	volatile int *wbflush = (int *)NEWS5000_WBFLUSH;

	(void)*wbflush;
}

/*
 * called by hardware interrupt routine
 */
int
apbus_intr_call(level, stat)
	int level;
	int stat;
{
	int nintr = 0;
	struct ap_intrhand *ai;

	for (ai = apintr[level]; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_mask & stat) {
			nintr += (*ai->ai_func)(ai->ai_aux);
		}
	}
	return nintr;
}

/*
 * register device interrupt routine
 */
void *
apbus_intr_establish(level, mask, priority, func, aux, name, ctlno)
	int level;
	int mask;
	int priority;
	int (*func) __P((void *));
	void *aux;
	char *name;
	int ctlno;
{
	struct ap_intrhand *ai, **aip;
	volatile unsigned int *inten0 = (volatile unsigned int *)NEWS5000_INTEN0;
	volatile unsigned int *inten1 = (volatile unsigned int *)NEWS5000_INTEN1;

	ai = malloc(sizeof(*ai), M_DEVBUF, M_NOWAIT);
	if (ai == NULL)
		panic("apbus_intr_establish: can't malloc handler info");
	ai->ai_mask = mask;
	ai->ai_priority = priority;
	ai->ai_func = func;
	ai->ai_aux = aux;
	strncpy(ai->ai_name, name, APBUS_DEVNAMELEN-1);
	ai->ai_ctlno = ctlno;

	for (aip = &apintr[level]; *aip != NULL; aip = &(*aip)->ai_next) {
		if ((*aip)->ai_priority < priority)
			break;
	}
	ai->ai_next = *aip;
	*aip = ai;
	switch (level) {
	case 0:
		*inten0 |= mask;
		break;
	case 1:
		*inten1 |= mask;
		break;
	}

	return (void *)ai;
}

void
apbus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * Flush DMA cache by issueing IO read for the AProm of specified slot.
	 */
	bus_space_read_4(t->_slotbaset, t->_slotbaseh, 0);

	_bus_dmamap_sync(t, map, offset, len, ops);
}

struct newsmips_bus_dma_tag apbus_dma_tag = {
	_bus_dmamap_create, 
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	apbus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

struct newsmips_bus_dma_tag *
apbus_dmatag_init(apa)
	struct apbus_attach_args *apa;
{
	struct newsmips_bus_dma_tag *dmat;

	dmat = malloc(sizeof(*dmat), M_DEVBUF, M_NOWAIT);
	if (dmat != NULL) {
		memcpy(dmat, &apbus_dma_tag, sizeof(*dmat));
		dmat->_slotno = apa->apa_slotno;
		dmat->_slotbaset = 0;
		dmat->_slotbaseh = apa->apa_hwbase;
	}
	return dmat;
}
