/*	$NetBSD: apbus.c,v 1.6 2001/07/26 22:55:13 wiz Exp $	*/

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

#include <uvm/uvm_extern.h>

#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#define _NEWSMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <newsmips/apbus/apbusvar.h>

static int  apbusmatch (struct device *, struct cfdata *, void *);
static void apbusattach (struct device *, struct device *, void *);
static int apbusprint (void *, const char *);
/* static void *aptokseg0 (void *); */
static void apbus_dma_unmapped (bus_dma_tag_t, bus_dmamap_t);
static int apbus_dma_mapalloc (bus_dma_tag_t, bus_dmamap_t, int);
static void apbus_dma_mapfree (bus_dma_tag_t, bus_dmamap_t);
static void apbus_dma_mapset (bus_dma_tag_t, bus_dmamap_t);
static int apbus_dmamap_create (bus_dma_tag_t, bus_size_t, int, bus_size_t,
			bus_size_t, int, bus_dmamap_t *);
static void apbus_dmamap_destroy (bus_dma_tag_t, bus_dmamap_t);
static int apbus_dmamap_load (bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
			struct proc *, int);
static int apbus_dmamap_load_mbuf (bus_dma_tag_t, bus_dmamap_t, struct mbuf *,
			int);
static int apbus_dmamap_load_uio (bus_dma_tag_t, bus_dmamap_t, struct uio *,
			int);
static int apbus_dmamap_load_raw (bus_dma_tag_t, bus_dmamap_t,
			bus_dma_segment_t *, int, bus_size_t, int);
static void apbus_dmamap_sync (bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
			bus_size_t, int);

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
	int (*ai_func) (void*);		/* function */
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
	int (*func) (void *);
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

static void
apbus_dma_unmapped(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	int seg;

	for (seg = 0; seg < map->dm_nsegs; seg++) {
		/*
		 * set MSB to indicate unmapped DMA.
		 * also need bit 30 for memory over 256MB.
		 */
		if ((map->dm_segs[seg].ds_addr & 0x30000000) == 0)
			map->dm_segs[seg].ds_addr |= 0x80000000;
		else
			map->dm_segs[seg].ds_addr |= 0xc0000000;
	}
}

#define	APBUS_NDMAMAP	(NEWS5000_APBUS_MAPSIZE / NEWS5000_APBUS_MAPENT)
#define	APBUS_MAPTBL(n, v)	(*(volatile u_int *)(NEWS5000_APBUS_DMAMAP + \
			 NEWS5000_APBUS_MAPENT * (n) + 1) = (v))
static u_char apbus_dma_maptbl[APBUS_NDMAMAP];

static int
apbus_dma_mapalloc(t, map, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	int flags;
{
	int i, j, cnt;

	cnt = round_page(map->_dm_size) / NBPG;

  again:
	for (i = 0; i < APBUS_NDMAMAP; i += j + 1) {
		for (j = 0; j < cnt; j++) {
			if (apbus_dma_maptbl[i + j])
				break;
		}
		if (j == cnt) {
			for (j = 0; j < cnt; j++)
				apbus_dma_maptbl[i + j] = 1;
			map->_dm_maptbl = i;
			map->_dm_maptblcnt = cnt;
			return 0;
		}
	}
	if ((flags & BUS_DMA_NOWAIT) == 0) {
		tsleep(&apbus_dma_maptbl, PRIBIO, "apdmat", 0);
		goto again;
	}
	return ENOMEM;
}

static void
apbus_dma_mapfree(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	int i, n;

	if (map->_dm_maptblcnt > 0) {
		n = map->_dm_maptbl;
		for (i = 0; i < map->_dm_maptblcnt; i++, n++) {
#ifdef DIAGNOSTIC
			if (apbus_dma_maptbl[n] == 0)
				panic("freeing free dma map");
			APBUS_MAPTBL(n, 0xffffffff);	/* causes DMA error */
#endif
			apbus_dma_maptbl[n] = 0;
		}
		wakeup(&apbus_dma_maptbl);
		map->_dm_maptblcnt = 0;
	}
}

static void
apbus_dma_mapset(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	int i;
	bus_addr_t addr, eaddr;
	int seg;
	bus_dma_segment_t *segs;

	i = 0;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		segs = &map->dm_segs[seg];
		for (addr = segs->ds_addr, eaddr = addr + segs->ds_len;
		    addr < eaddr; addr += NBPG, i++) {
#ifdef DIAGNOSTIC
			if (i >= map->_dm_maptblcnt)
				panic("dma map table overflow");
#endif
			APBUS_MAPTBL(map->_dm_maptbl + i,
				NEWS5000_APBUS_MAP_VALID |
				NEWS5000_APBUS_MAP_COHERENT |
				(addr >> PGSHIFT));
		}
	}
	map->dm_segs[0].ds_addr = map->_dm_maptbl << PGSHIFT;
	map->dm_segs[0].ds_len = map->dm_mapsize;
	map->dm_nsegs = 1;
}

static int
apbus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	int error;

	if (flags & NEWSMIPS_DMAMAP_MAPTBL)
		nsegments = round_page(size) / NBPG;
	error = _bus_dmamap_create(t, size, nsegments, maxsegsz, boundary,
	    flags, dmamp);
	if (error == 0 && (flags & NEWSMIPS_DMAMAP_MAPTBL)) {
		error = apbus_dma_mapalloc(t, *dmamp, flags);
		if (error) {
			_bus_dmamap_destroy(t, *dmamp);
			*dmamp = NULL;
		}
	}
	return error;
}

static void
apbus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
		apbus_dma_mapfree(t, map);
	_bus_dmamap_destroy(t, map);
}

static int
apbus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	int error;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error == 0) {
		if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
			apbus_dma_mapset(t, map);
		else
			apbus_dma_unmapped(t, map);
	}
	return error;
}

static int
apbus_dmamap_load_mbuf(t, map, m0, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m0;
	int flags;
{
	int error;

	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	if (error == 0) {
		if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
			apbus_dma_mapset(t, map);
		else
			apbus_dma_unmapped(t, map);
	}
	return error;
}

static int
apbus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{
	int error;

	error = _bus_dmamap_load_uio(t, map, uio, flags);
	if (error == 0) {
		if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
			apbus_dma_mapset(t, map);
		else
			apbus_dma_unmapped(t, map);
	}
	return error;
}

static int
apbus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	int error;

	error = _bus_dmamap_load_raw(t, map, segs, nsegs, size, flags);
	if (error == 0) {
		if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
			apbus_dma_mapset(t, map);
		else
			apbus_dma_unmapped(t, map);
	}
	return error;
}

static void
apbus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * Flush DMA cache by issuing IO read for the AProm of specified slot.
	 */
	bus_space_read_4(t->_slotbaset, t->_slotbaseh, 0);

	_bus_dmamap_sync(t, map, offset, len, ops);
}

struct newsmips_bus_dma_tag apbus_dma_tag = {
	apbus_dmamap_create, 
	apbus_dmamap_destroy,
	apbus_dmamap_load,
	apbus_dmamap_load_mbuf,
	apbus_dmamap_load_uio,
	apbus_dmamap_load_raw,
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
