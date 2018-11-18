/*	$NetBSD: apbus.c,v 1.22.48.1 2018/11/18 11:54:02 martin Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apbus.c,v 1.22.48.1 2018/11/18 11:54:02 martin Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#define _NEWSMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <newsmips/apbus/apbusvar.h>

static int  apbusmatch(device_t, cfdata_t, void *);
static void apbusattach(device_t, device_t, void *);
static int apbusprint(void *, const char *);
#if 0
static void *aptokseg0 (void *);
#endif
static void apbus_dma_unmapped(bus_dma_tag_t, bus_dmamap_t);
static int apbus_dma_mapalloc(bus_dma_tag_t, bus_dmamap_t, int);
static void apbus_dma_mapfree(bus_dma_tag_t, bus_dmamap_t);
static void apbus_dma_mapset(bus_dma_tag_t, bus_dmamap_t);
static int apbus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
    bus_size_t, int, bus_dmamap_t *);
static void apbus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
static int apbus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
    struct proc *, int);
static int apbus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t, struct mbuf *,
    int);
static int apbus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t, struct uio *,
    int);
static int apbus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
    bus_dma_segment_t *, int, bus_size_t, int);
static void apbus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
    bus_size_t, int);

#define	MAXAPDEVNUM	32

CFATTACH_DECL_NEW(ap, 0,
    apbusmatch, apbusattach, NULL, NULL);

#define	NLEVEL	2
static struct newsmips_intr apintr_tab[NLEVEL];

static int
apbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "ap") != 0)
		return 0;

	return 1;
}


static void
apbusattach(device_t parent, device_t self, void *aux)
{
	struct apbus_attach_args child;
	struct apbus_dev *apdev;
	struct apbus_ctl *apctl;
	struct newsmips_intr *ip;
	int i;

	apbus_map_romwork();
	mips_set_wbflush(apbus_wbflush);

	*(volatile uint32_t *)(NEWS5000_APBUS_INTST) = 0xffffffff;
	*(volatile uint32_t *)(NEWS5000_APBUS_INTMSK) = 0xffffffff;
	*(volatile uint32_t *)(NEWS5000_APBUS_CTRL) = 0x00000004;
	*(volatile uint32_t *)(NEWS5000_APBUS_DMA) = 0xffffffff;

	aprint_normal("\n");

	for (i = 0; i < NLEVEL; i++) {
		ip = &apintr_tab[i];
		LIST_INIT(&ip->intr_q);
	}

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
apbusprint(void *aux, const char *pnp)
{
	struct apbus_attach_args *a = aux;

	if (pnp)
		aprint_normal("%s at %s slot%d addr 0x%lx",
		    a->apa_name, pnp, a->apa_slotno, a->apa_hwbase);

	return UNCONF;
}

#if 0
void *
aptokseg0(void *va)
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
apbus_wbflush(void)
{
	volatile int32_t * const our_wbflush = (int32_t *)NEWS5000_WBFLUSH;

	(*mips_locore_jumpvec.ljv_wbflush)();
	(void)*our_wbflush;
}

/*
 * called by hardware interrupt routine
 */
int
apbus_intr_dispatch(int level, int stat)
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih;
	int nintr;

	ip = &apintr_tab[level];

	nintr = 0;
	LIST_FOREACH(ih, &ip->intr_q, ih_q) {
		if (ih->ih_mask & stat)
			nintr += (*ih->ih_func)(ih->ih_arg);
	}
	return nintr;
}

/*
 * register device interrupt routine
 */
void *
apbus_intr_establish(int level, int mask, int priority, int (*func)(void *),
    void *arg, const char *name, int ctlno)
{
	struct newsmips_intr *ip;
	struct newsmips_intrhand *ih, *curih;
	volatile uint32_t *inten0, *inten1;

	ip = &apintr_tab[level];

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		panic("%s: can't malloc handler info", __func__);
	ih->ih_mask = mask;
	ih->ih_priority = priority;
	ih->ih_func = func;
	ih->ih_arg = arg;

	if (LIST_EMPTY(&ip->intr_q)) {
		LIST_INSERT_HEAD(&ip->intr_q, ih, ih_q);
		goto done;
	}

	for (curih = LIST_FIRST(&ip->intr_q);
	    LIST_NEXT(curih, ih_q) != NULL;
	    curih = LIST_NEXT(curih, ih_q)) {
		if (ih->ih_priority > curih->ih_priority) {
			LIST_INSERT_BEFORE(curih, ih, ih_q);
			goto done;
		}
	}

	LIST_INSERT_AFTER(curih, ih, ih_q);

 done:
	switch (level) {
	case 0:
		inten0 = (volatile uint32_t *)NEWS5000_INTEN0;
		*inten0 |= mask;
		break;
	case 1:
		inten1 = (volatile uint32_t *)NEWS5000_INTEN1;
		*inten1 |= mask;
		break;
	}

	return (void *)ih;
}

static void
apbus_dma_unmapped(bus_dma_tag_t t, bus_dmamap_t map)
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
#define	APBUS_MAPTBL(n, v)	\
			(*(volatile uint32_t *)(NEWS5000_APBUS_DMAMAP + \
			 NEWS5000_APBUS_MAPENT * (n) + 1) = (v))
static uint8_t apbus_dma_maptbl[APBUS_NDMAMAP];

static int
apbus_dma_mapalloc(bus_dma_tag_t t, bus_dmamap_t map, int flags)
{
	int i, j, cnt;

	cnt = round_page(map->_dm_size) / PAGE_SIZE;

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
apbus_dma_mapfree(bus_dma_tag_t t, bus_dmamap_t map)
{
	int i, n;

	if (map->_dm_maptblcnt > 0) {
		n = map->_dm_maptbl;
		for (i = 0; i < map->_dm_maptblcnt; i++, n++) {
#ifdef DIAGNOSTIC
			if (apbus_dma_maptbl[n] == 0)
				panic("freeing free DMA map");
			APBUS_MAPTBL(n, 0xffffffff);	/* causes DMA error */
#endif
			apbus_dma_maptbl[n] = 0;
		}
		wakeup(&apbus_dma_maptbl);
		map->_dm_maptblcnt = 0;
	}
}

static void
apbus_dma_mapset(bus_dma_tag_t t, bus_dmamap_t map)
{
	int i;
	bus_addr_t addr, eaddr;
	int seg;
	bus_dma_segment_t *segs;

	i = 0;
	for (seg = 0; seg < map->dm_nsegs; seg++) {
		segs = &map->dm_segs[seg];
		for (addr = segs->ds_addr, eaddr = addr + segs->ds_len;
		    addr < eaddr; addr += PAGE_SIZE, i++) {
#ifdef DIAGNOSTIC
			if (i >= map->_dm_maptblcnt)
				panic("DMA map table overflow");
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
apbus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	int error;

	if (flags & NEWSMIPS_DMAMAP_MAPTBL)
		nsegments = round_page(size) / PAGE_SIZE;
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
apbus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{

	if (map->_dm_flags & NEWSMIPS_DMAMAP_MAPTBL)
		apbus_dma_mapfree(t, map);
	_bus_dmamap_destroy(t, map);
}

static int
apbus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
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
apbus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
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
apbus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
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
apbus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
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
apbus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{

	/*
	 * Flush DMA cache by issuing IO read for the AProm of specified slot.
	 */
	bus_space_read_4(t->_slotbaset, t->_slotbaseh, 0);

	bus_dmamap_sync(&newsmips_default_bus_dma_tag, map, offset, len, ops);
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
apbus_dmatag_init(struct apbus_attach_args *apa)
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
