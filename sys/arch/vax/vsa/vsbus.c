/*	$NetBSD: vsbus.c,v 1.57 2009/10/26 19:16:58 cegger Exp $ */
/*
 * Copyright (c) 1996, 1999 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vsbus.c,v 1.57 2009/10/26 19:16:58 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/stat.h>

#include <uvm/uvm_extern.h>

#define _VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/nexus.h>

#include <machine/uvax.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/ka43.h>

#include <machine/mainbus.h>
#include <machine/vsbus.h>

#include "ioconf.h"
#include "locators.h"
#include "opt_cputype.h"

static int	vsbus_match(device_t, cfdata_t, void *);
static void	vsbus_attach(device_t, device_t, void *);
static int	vsbus_print(void *, const char *);
static int	vsbus_search(device_t, cfdata_t, const int *, void *);

static SIMPLEQ_HEAD(, vsbus_dma) vsbus_dma;

CFATTACH_DECL_NEW(vsbus, sizeof(struct vsbus_softc),
    vsbus_match, vsbus_attach, NULL, NULL);

static struct vax_bus_dma_tag vsbus_bus_dma_tag = {
	._dmamap_create		= _bus_dmamap_create,
	._dmamap_destroy	= _bus_dmamap_destroy,
	._dmamap_load		= _bus_dmamap_load,
	._dmamap_load_mbuf	= _bus_dmamap_load_mbuf,
	._dmamap_load_uio	= _bus_dmamap_load_uio,
	._dmamap_load_raw	= _bus_dmamap_load_raw,
	._dmamap_unload		= _bus_dmamap_unload,
	._dmamap_sync		= _bus_dmamap_sync,
	._dmamem_alloc		= _bus_dmamem_alloc,
	._dmamem_free		= _bus_dmamem_free,
	._dmamem_map		= _bus_dmamem_map,
	._dmamem_unmap		= _bus_dmamem_unmap,
	._dmamem_mmap		= _bus_dmamem_mmap,
};

int
vsbus_print(void *aux, const char *name)
{
	struct vsbus_attach_args * const va = aux;

	aprint_normal(" csr 0x%lx vec %o ipl %x maskbit %d", va->va_paddr,
	    va->va_cvec & 511, va->va_br, va->va_maskno - 1);
	return UNCONF; 
}

int
vsbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp("vsbus", ma->ma_type);
}

void
vsbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	struct vsbus_softc *sc = device_private(self);
	int dbase, dsize;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_iot = ma->ma_iot;
	sc->sc_dmatag = vsbus_bus_dma_tag;

	switch (vax_boardtype) {
#if VAX49 || VAX53
	case VAX_BTYP_53:
	case VAX_BTYP_49:
		sc->sc_vsregs = vax_map_physmem(0x25c00000, 1);
		sc->sc_intreq = (char *)sc->sc_vsregs + 12;
		sc->sc_intclr = (char *)sc->sc_vsregs + 12;
		sc->sc_intmsk = (char *)sc->sc_vsregs + 8;
		vsbus_dma_init(sc, 8192);
		break;
#endif

#if VAX46 || VAX48
	case VAX_BTYP_48:
	case VAX_BTYP_46:
		sc->sc_vsregs = vax_map_physmem(VS_REGS, 1);
		sc->sc_intreq = (char *)sc->sc_vsregs + 15;
		sc->sc_intclr = (char *)sc->sc_vsregs + 15;
		sc->sc_intmsk = (char *)sc->sc_vsregs + 12;
		vsbus_dma_init(sc, 32768);
#endif

	default:
		sc->sc_vsregs = vax_map_physmem(VS_REGS, 1);
		sc->sc_intreq = (char *)sc->sc_vsregs + 15;
		sc->sc_intclr = (char *)sc->sc_vsregs + 15;
		sc->sc_intmsk = (char *)sc->sc_vsregs + 12;
		if (vax_boardtype == VAX_BTYP_410) {
			dbase = KA410_DMA_BASE;
			dsize = KA410_DMA_SIZE;
		} else {
			dbase = KA420_DMA_BASE;
			dsize = KA420_DMA_SIZE;
			*(char *)(sc->sc_vsregs + 0xe0) = 1; /* Big DMA */
		}
		sc->sc_dmasize = dsize;
		sc->sc_dmaaddr = uvm_km_alloc(kernel_map, dsize, 0,
		    UVM_KMF_VAONLY);
		ioaccess(sc->sc_dmaaddr, dbase, dsize/VAX_NBPG);
		break;
	}

	SIMPLEQ_INIT(&vsbus_dma);
	/*
	 * First: find which interrupts we won't care about.
	 * There are interrupts that interrupt on a periodic basic
	 * that we don't want to interfere with the rest of the 
	 * interrupt probing.
	 */
	*sc->sc_intmsk = 0;
	*sc->sc_intclr = 0xff;
	DELAY(1000000); /* Wait a second */
	sc->sc_mask = *sc->sc_intreq;
	aprint_normal_dev(self, "interrupt mask %x\n", sc->sc_mask);
	/*
	 * now check for all possible devices on this "bus"
	 */
	config_search_ia(vsbus_search, self, "vsbus", NULL);

	/* Autoconfig finished, enable interrupts */
	*sc->sc_intmsk = ~sc->sc_mask;
}

int
vsbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct vsbus_softc *sc = device_private(parent);
	struct vsbus_attach_args va;
	int i, vec, br;
	u_char c;

	va.va_paddr = cf->cf_loc[VSBUSCF_CSR];
	va.va_addr = vax_map_physmem(va.va_paddr, 1);
	va.va_dmat = &sc->sc_dmatag;
	va.va_memt = sc->sc_iot;

	*sc->sc_intmsk = 0;
	*sc->sc_intclr = 0xff;
	scb_vecref(0, 0); /* Clear vector ref */

	i = config_match(parent, cf, &va);
	vax_unmap_physmem(va.va_addr, 1);
	c = *sc->sc_intreq & ~sc->sc_mask;
	if (i == 0)
		goto forgetit;
	if (i > 10)
		c = sc->sc_mask; /* Fooling interrupt */
	else if (c == 0)
		goto forgetit;

	*sc->sc_intmsk = c;
	DELAY(100);
	*sc->sc_intmsk = 0;
	va.va_maskno = ffs((u_int)c);
	i = scb_vecref(&vec, &br);
	if (i == 0)
		goto fail;
	if (vec == 0)
		goto fail;

	va.va_br = br;
	va.va_cvec = vec;
	va.va_dmaaddr = sc->sc_dmaaddr;
	va.va_dmasize = sc->sc_dmasize;
	*sc->sc_intmsk = c; /* Allow interrupts during attach */
	config_attach(parent, cf, &va, vsbus_print);
	*sc->sc_intmsk = 0;
	return 0;

fail:
	printf("%s%d at %s csr 0x%x %s\n",
	    cf->cf_name, cf->cf_unit, device_xname(parent),
	    cf->cf_loc[VSBUSCF_CSR], (i ? "zero vector" : "didn't interrupt"));
forgetit:
	return 0;
}

/*
 * Sets a new interrupt mask. Returns the old one.
 * Works like spl functions.
 */
unsigned char
vsbus_setmask(int mask)
{
	struct vsbus_softc * const sc = device_lookup_private(&vsbus_cd, 0);
	unsigned char ch;

	if (sc == NULL)
		return 0;

	ch = *sc->sc_intmsk;
	*sc->sc_intmsk = mask;
	return ch;
}

/*
 * Clears the interrupts in mask.
 */
void
vsbus_clrintr(int mask)
{
	struct vsbus_softc * const sc = device_lookup_private(&vsbus_cd, 0);
	if (sc == NULL)
		return;

	*sc->sc_intclr = mask;
}

/*
 * Copy data from/to a user process' space from the DMA area.
 * Use the physical memory directly.
 */
void
vsbus_copytoproc(struct proc *p, void *fromv, void *tov, int len)
{
	char *from = fromv, *to = tov;
	struct pte *pte;
	paddr_t pa;

	if ((vaddr_t)to & KERNBASE) { /* In kernel space */
		memcpy(to, from, len);
		return;
	}

#ifdef DIAGNOSTIC
	if (p == NULL)
		panic("vsbus_copytoproc: no proc");
#endif

	if ((vaddr_t)to & 0x40000000)
		pte = &p->p_vmspace->vm_map.pmap->pm_p1br[vax_btop((vaddr_t)to & ~0x40000000)];
	else
		pte = &p->p_vmspace->vm_map.pmap->pm_p0br[vax_btop((vaddr_t)to)];
	if ((vaddr_t)to & PGOFSET) {
		int cz = round_page((vaddr_t)to) - (vaddr_t)to;

		pa = (pte->pg_pfn << VAX_PGSHIFT) | (PAGE_SIZE - cz) | KERNBASE;
		memcpy((void *)pa, from, min(cz, len));
		from += cz;
		to += cz;
		len -= cz;
		pte += 8; /* XXX */
	}
	while (len > 0) {
		pa = (pte->pg_pfn << VAX_PGSHIFT) | KERNBASE;
		memcpy((void *)pa, from, min(PAGE_SIZE, len));
		from += PAGE_SIZE;
		to += PAGE_SIZE;
		len -= PAGE_SIZE;
		pte += 8; /* XXX */
	}
}

void
vsbus_copyfromproc(struct proc *p, void *fromv, void *tov, int len)
{
	char *from = fromv, *to = tov;
	struct pte *pte;
	paddr_t pa;

	if ((vaddr_t)from & KERNBASE) { /* In kernel space */
		memcpy(to, from, len);
		return;
	}

#ifdef DIAGNOSTIC
	if (p == NULL)
		panic("vsbus_copyfromproc: no proc");
#endif

	if ((vaddr_t)from & 0x40000000)
		pte = &p->p_vmspace->vm_map.pmap->pm_p1br[vax_btop((vaddr_t)from & ~0x40000000)];
	else
		pte = &p->p_vmspace->vm_map.pmap->pm_p0br[vax_btop((vaddr_t)from)];
	if ((vaddr_t)from & PGOFSET) {
		int cz = round_page((vaddr_t)from) - (vaddr_t)from;

		pa = (pte->pg_pfn << VAX_PGSHIFT) | (PAGE_SIZE - cz) | KERNBASE;
		memcpy(to, (void *)pa, min(cz, len));
		from += cz;
		to += cz;
		len -= cz;
		pte += 8; /* XXX */
	}
	while (len > 0) {
		pa = (pte->pg_pfn << VAX_PGSHIFT) | KERNBASE;
		memcpy(to,  (void *)pa, min(PAGE_SIZE, len));
		from += PAGE_SIZE;
		to += PAGE_SIZE;
		len -= PAGE_SIZE;
		pte += 8; /* XXX */
	}
}

/* 
 * There can only be one user of the DMA area on VS2k/VS3100 at one
 * time, so keep track of it here.
 */ 
static int vsbus_active = 0;

void
vsbus_dma_start(struct vsbus_dma *vd)
{
 
	SIMPLEQ_INSERT_TAIL(&vsbus_dma, vd, vd_q);

	if (vsbus_active == 0)
		vsbus_dma_intr();
}
 
void
vsbus_dma_intr(void)
{	
	struct vsbus_dma *vd;
	
	vd = SIMPLEQ_FIRST(&vsbus_dma); 
	if (vd == NULL) {
		vsbus_active = 0;
		return;
	}
	vsbus_active = 1;
	SIMPLEQ_REMOVE_HEAD(&vsbus_dma, vd_q);
	(*vd->vd_go)(vd->vd_arg);
}

