/*	$NetBSD: vme_machdep.c,v 1.5 1998/02/06 00:24:42 pk Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>

#include <vm/vm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>

#include <dev/vme/vmevar.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/dev/vmereg.h>

struct vmebus_softc { 
	struct device	 sc_dev;	/* base device */
	struct vmebusreg *sc_reg; 	/* VME control registers */
	struct vmebusvec *sc_vec;	/* VME interrupt vector */
	struct rom_range *sc_range;	/* ROM range property */
	int		 sc_nrange;
	volatile u_int32_t *sc_ioctags;	/* VME IO-cache tag registers */
	volatile u_int32_t *sc_iocflush;/* VME IO-cache flush registers */
	int 		 (*sc_vmeintr) __P((void *));
	struct bootpath	 *sc_bp;
};
struct  vmebus_softc *vmebus_sc;/*XXX*/

/* autoconfiguration driver */
static int	vmematch    __P((struct device *, struct cfdata *, void *));
static void	vmeattach   __P((struct device *, struct device *, void *));
#if defined(SUN4)
static void	vmeattach4  __P((struct device *, struct device *, void *));
int 		vmeintr4  __P((void *));
#endif
#if defined(SUN4M)
static void	vmeattach4m __P((struct device *, struct device *, void *));
int 		vmeintr4m __P((void *));
#endif


static int	sparc_vme_probe __P((void *, bus_space_tag_t, vme_addr_t,
				     size_t, vme_size_t, vme_mod_t,
				     int (*) __P((void *, void *)), void *));
static int	sparc_vme_map __P((void *, vme_addr_t, vme_size_t, vme_mod_t,
				   bus_space_tag_t, bus_space_handle_t *));
static void	sparc_vme_unmap __P((void *));
static int	sparc_vme_mmap_cookie __P((void *, vme_addr_t, vme_mod_t,
				   bus_space_tag_t, int *));
static int	sparc_vme_intr_map __P((void *, int, int, vme_intr_handle_t *));
static void *	sparc_vme_intr_establish __P((void *, vme_intr_handle_t,
					      int (*) __P((void *)), void *));
static void	sparc_vme_intr_disestablish __P((void *, void *));

static void	vmebus_translate __P((struct vmebus_softc *, vme_mod_t,
				      struct rom_reg *));
static void	sparc_vme_bus_establish __P((void *, struct device *));
#if defined(SUN4M)
static void	sparc_vme4m_barrier __P((void *));
#endif

/*
 * DMA functions.
 */
#if defined(SUN4)
static int	sparc_vme4_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
static void	sparc_vme4_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme4_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));

static int	sparc_vme4_dmamem_alloc __P((bus_dma_tag_t, bus_size_t,
		    bus_size_t, bus_size_t, bus_dma_segment_t *,
		    int, int *, int));
static void	sparc_vme4_dmamem_free __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
#endif

#if defined(SUN4M)
static int	sparc_vme4m_dmamap_create __P((bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *));

static int	sparc_vme4m_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
static void	sparc_vme4m_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme4m_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));

static int	sparc_vme4m_dmamem_alloc __P((bus_dma_tag_t, bus_size_t,
		    bus_size_t, bus_size_t, bus_dma_segment_t *,
		    int, int *, int));
static void	sparc_vme4m_dmamem_free __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
#endif

#if 0
static void	sparc_vme_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
static int	sparc_vme_dmamem_map __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
static void	sparc_vme_dmamem_unmap __P((bus_dma_tag_t, caddr_t, size_t));
static int	sparc_vme_dmamem_mmap __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int, int, int, int));
#endif

struct cfattach vme_ca = {
	sizeof(struct vmebus_softc), vmematch, vmeattach
};

struct sparc_bus_space_tag sparc_vme_bus_tag = {
	NULL, /* cookie */
	NULL, /* bus_map */
	NULL, /* bus_unmap */
	NULL, /* bus_subregion */
	NULL  /* barrier */
};

struct vme_chipset_tag sparc_vme_chipset_tag = {
	NULL,
	sparc_vme_probe,
	sparc_vme_map,
	sparc_vme_unmap,
	sparc_vme_mmap_cookie,
	sparc_vme_intr_map,
	sparc_vme_intr_establish,
	sparc_vme_intr_disestablish,
	sparc_vme_bus_establish
};


#if defined(SUN4)
struct sparc_bus_dma_tag sparc_vme4_dma_tag = {
	NULL,	/* cookie */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	sparc_vme4_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	sparc_vme4_dmamap_unload,
	sparc_vme4_dmamap_sync,

	sparc_vme4_dmamem_alloc,
	sparc_vme4_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};
#endif

#if defined(SUN4M)
struct sparc_bus_dma_tag sparc_vme4m_dma_tag = {
	NULL,	/* cookie */
	sparc_vme4m_dmamap_create,
	_bus_dmamap_destroy,
	sparc_vme4m_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	sparc_vme4m_dmamap_unload,
	sparc_vme4m_dmamap_sync,

	sparc_vme4m_dmamem_alloc,
	sparc_vme4m_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};
#endif


void
sparc_vme_bus_establish(cookie, dev)
	void *cookie;
	struct device *dev;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
	struct bootpath *bp = sc->sc_bp;
	char *name;

	name = dev->dv_cfdata->cf_driver->cd_name;
#ifdef DEBUG
	printf("sparc_vme_bus_establish: %s%d\n", name, dev->dv_unit);
#endif
	if (bp != NULL && strcmp(bp->name, name) == 0 &&
	    dev->dv_unit == bp->val[1]) {
		bp->dev = dev;
#ifdef DEBUG
printf("sparc_vme_bus_establish: on the boot path\n");
#endif
		sc->sc_bp++;
		bootpath_store(1, sc->sc_bp);
	}
}


int
vmematch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	if (CPU_ISSUN4C)
		return (0);

	return (strcmp(cf->cf_driver->cd_name, ra->ra_name) == 0);
}

void
vmeattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)self;
	struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	if (ra->ra_bp != NULL && strcmp(ra->ra_bp->name, "vme") == 0) {
		sc->sc_bp = ra->ra_bp + 1;
		bootpath_store(1, sc->sc_bp);
	}

#if defined(SUN4)
	if (CPU_ISSUN4)
		vmeattach4(parent, self, aux);
#endif

#if defined(SUN4M)
	if (CPU_ISSUN4M)
		vmeattach4m(parent, self, aux);
#endif

	bootpath_store(1, NULL);
}

#if defined(SUN4)
void
vmeattach4(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)self;
	struct vme_busattach_args vba;

	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	/* VME interrupt entry point */
	sc->sc_vmeintr = vmeintr4;

/*XXX*/	sparc_vme_chipset_tag.cookie = self;
/*XXX*/	sparc_vme4_dma_tag._cookie = self;

	vba.vba_bustag = &sparc_vme_bus_tag;
	vba.vba_chipset_tag = &sparc_vme_chipset_tag;
	vba.vba_dmatag = &sparc_vme4_dma_tag;

	printf("\n");
	(void)config_search(vmesearch, self, &vba);
	return;
}
#endif

#if defined(SUN4M)
/* sun4m vmebus */
void
vmeattach4m(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)self;
	struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;
	int node, rlen;
	struct vme_busattach_args vba;
	int cline;

	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	/* VME interrupt entry point */
	sc->sc_vmeintr = vmeintr4m;

/*XXX*/	sparc_vme_chipset_tag.cookie = self;
/*XXX*/	sparc_vme4m_dma_tag._cookie = self;
	sparc_vme_bus_tag.sparc_barrier = sparc_vme4m_barrier;

	vba.vba_bustag = &sparc_vme_bus_tag;
	vba.vba_chipset_tag = &sparc_vme_chipset_tag;
	vba.vba_dmatag = &sparc_vme4m_dma_tag;

	node = ra->ra_node;

	/* Map VME control space */
	sc->sc_reg = (struct vmebusreg *)
		mapdev(&ra->ra_reg[0], 0, 0, ra->ra_reg[0].rr_len);
	sc->sc_vec = (struct vmebusvec *)
		mapdev(&ra->ra_reg[1], 0, 0, ra->ra_reg[1].rr_len);
	sc->sc_ioctags = (u_int32_t *)
		mapdev(&ra->ra_reg[1], 0, VME_IOC_TAGOFFSET, VME_IOC_SIZE);
	sc->sc_iocflush = (u_int32_t *)
		mapdev(&ra->ra_reg[1], 0, VME_IOC_FLUSHOFFSET, VME_IOC_SIZE);

/*XXX*/	sparc_vme_bus_tag.cookie = sc->sc_reg;

	/*
	 * Get "range" property.
	 */
	rlen = getproplen(node, "ranges");
	if (rlen > 0) {
		sc->sc_nrange = rlen / sizeof(struct rom_range);
		sc->sc_range =
			(struct rom_range *)malloc(rlen, M_DEVBUF, M_NOWAIT);
		if (sc->sc_range == 0)  
			panic("vme: PROM ranges too large: %d", rlen);
		(void)getprop(node, "ranges", sc->sc_range, rlen);
	}

	vmebus_sc = sc;

	/*
	 * Invalidate all IO-cache entries.
	 */
	for (cline = VME_IOC_SIZE/VME_IOC_LINESZ; cline > 0;) {
		sc->sc_ioctags[--cline] = 0;
	}

	/* Enable IO-cache */
	sc->sc_reg->vmebus_cr |= VMEBUS_CR_C;

	printf(": version 0x%x\n",
	       sc->sc_reg->vmebus_cr & VMEBUS_CR_IMPL);

	(void)config_search(vmesearch, self, &vba);
}
#endif

void sparc_vme_async_fault __P((void));
void
sparc_vme_async_fault()
{
	struct vmebus_softc *sc = vmebus_sc;
	u_int32_t addr;

	addr = sc->sc_reg->vmebus_afar;
	printf("vme afsr: %x; addr %x\n", sc->sc_reg->vmebus_afsr, addr);
}

int
sparc_vme_probe(cookie, tag, addr, offset, size, mod, callback, arg)
	void *cookie;
	bus_space_tag_t tag;
	vme_addr_t addr;
	size_t offset;
	vme_size_t size;
	int mod;
	int (*callback) __P((void *, void *));
	void *arg;
{
	struct rom_reg reg;
	caddr_t tmp;
	int result;
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;

/* XXX - Use bus_space_[un]map() etc. */
	reg.rr_paddr = (void *)addr;
	vmebus_translate(sc, mod, &reg);
	tmp = (caddr_t)mapdev(&reg, TMPMAP_VA, 0, NBPG);
	result = probeget(tmp + offset, size) != -1;
	if (result && callback != NULL)
		result = (*callback)(tmp, arg);
	pmap_remove(pmap_kernel(), TMPMAP_VA, TMPMAP_VA+NBPG);
	return (result);
}

int
sparc_vme_map(cookie, addr, size, mod, tag, handlep)
	void *cookie;
	vme_addr_t addr;
	vme_size_t size;
	int mod;
	bus_space_tag_t tag;
	bus_space_handle_t *handlep;
{
	struct rom_reg reg;
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;

	reg.rr_paddr = (void *)addr;
	vmebus_translate(sc, mod, &reg);
	*handlep = (bus_space_handle_t)mapdev(&reg, 0, 0, size);
	return (0);
}

int
sparc_vme_mmap_cookie(cookie, addr, mod, tag, handlep)
	void *cookie;
	vme_addr_t addr;
	int mod;
	bus_space_tag_t tag;
	int *handlep;
{
	struct rom_reg reg;
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
  
	reg.rr_paddr = (void *)addr;
	vmebus_translate(sc, mod, &reg);
	*handlep = (int)reg.rr_paddr | PMAP_IOENC(reg.rr_iospace) | PMAP_NC;
	return (0);
}

void
vmebus_translate(sc, mod, rr)
	struct vmebus_softc *sc;
	vme_mod_t mod;
	struct rom_reg *rr;
{
	register int j;

	if (CPU_ISSUN4) {
		(int)rr->rr_iospace = (mod & VMEMOD_D32)
			? PMAP_VME32
			: PMAP_VME16;

		switch (mod & ~VMEMOD_D32) {
		case VMEMOD_A16|VMEMOD_D|VMEMOD_S:
			rr->rr_paddr += 0xffff0000;
			break;
		case VMEMOD_A24|VMEMOD_D|VMEMOD_S:
			rr->rr_paddr += 0xff000000;
			break;
		case VMEMOD_A32|VMEMOD_D|VMEMOD_S:
			break;
		default:
			panic("vmebus_translate: unsupported VME modifier: %x",
				mod);
		}
		return;
	}


	/* sun4m VME node: translate through "ranges" property */
	if (sc->sc_nrange == 0)
		panic("vmebus: no ranges");

	/* Translate into parent address spaces */
	for (j = 0; j < sc->sc_nrange; j++) {
		if (sc->sc_range[j].cspace == mod) {
			(int)rr->rr_paddr +=
				sc->sc_range[j].poffset;
			(int)rr->rr_iospace =
				sc->sc_range[j].pspace;
			return;
		}
	}
	panic("sparc_vme_translate: modifier %x not supported", mod);
}

#if defined(SUN4M)
void
sparc_vme4m_barrier(cookie)
	void *cookie;
{
	struct vmebusreg *vbp = (struct vmebusreg *)cookie;

	/* Read async fault status to flush write-buffers */
	(*(volatile int *)&vbp->vmebus_afsr);
}
#endif



/*
 * VME Interrupt Priority Level to sparc Processor Interrupt Level.
 */
static int vme_ipl_to_pil[] = {
	0,
	2,
	3,
	5,
	7,
	9,
	11,
	13
};


/*
 * All VME device interrupts go through vmeintr(). This function reads
 * the VME vector from the bus, then dispatches the device interrupt
 * handler.  All handlers for devices that map to the same Processor
 * Interrupt Level (according to the table above) are on a linked list
 * of `sparc_vme_intr_handle' structures. The head of which is passed
 * down as the argument to `vmeintr(void *arg)'.
 */
struct sparc_vme_intr_handle {
	struct intrhand ih;
	struct sparc_vme_intr_handle *next;
	int	vec;		/* VME interrupt vector */
	int	pri;		/* VME interrupt priority */
	struct vmebus_softc *sc;/*XXX*/
};

#if defined(SUN4)
int
vmeintr4(arg)
	void *arg;
{
	struct sparc_vme_intr_handle *ihp = (vme_intr_handle_t)arg;
	int level, vec;
	int i = 0;

	level = (ihp->pri << 1) | 1;

	vec = ldcontrolb((caddr_t)(AC_VMEINTVEC | level));

	if (vec == -1) {
		printf("vme: spurious interrupt\n");
		return 1; /* XXX - pretend we handled it, for now */
	}

	for (; ihp; ihp = ihp->next)
		if (ihp->vec == vec && ihp->ih.ih_fun)
			i += (ihp->ih.ih_fun)(ihp->ih.ih_arg);
	return (i);
}
#endif

#if defined(SUN4M)
int
vmeintr4m(arg)
	void *arg;
{
	struct sparc_vme_intr_handle *ihp = (vme_intr_handle_t)arg;
	int level, vec;
	int i = 0;

	level = (ihp->pri << 1) | 1;

#if 0
	int pending;

	/* Flush VME <=> Sbus write buffers */
	(*(volatile int *)&ihp->sc->sc_reg->vmebus_afsr);

	pending = *((int*)ICR_SI_PEND);
	if ((pending & SINTR_VME(ihp->pri)) == 0) {
		printf("vmeintr: non pending at pri %x(p 0x%x)\n",
			ihp->pri, pending);
		return (0);
	}
#endif
#if 0
	/* Why gives this a bus timeout sometimes? */
	vec = ihp->sc->sc_vec->vmebusvec[level];
#else
	/* so, arrange to catch the fault... */
	{
	extern struct user *proc0paddr;
	extern int fkbyte __P((caddr_t, struct pcb *));
	caddr_t addr = (caddr_t)&ihp->sc->sc_vec->vmebusvec[level];
	struct pcb *xpcb;
	u_long saveonfault;
	int s;

	s = splhigh();
	if (curproc == NULL)
		xpcb = (struct pcb *)proc0paddr;
	else
		xpcb = &curproc->p_addr->u_pcb;

	saveonfault = (u_long)xpcb->pcb_onfault;
	vec = fkbyte(addr, xpcb);
	xpcb->pcb_onfault = (caddr_t)saveonfault;

	splx(s);
	}
#endif

	if (vec == -1) {
		printf("vme: spurious interrupt: ");
		printf("SI: 0x%x, VME AFSR: 0x%x, VME AFAR 0x%x\n",
			*((int*)ICR_SI_PEND),
			ihp->sc->sc_reg->vmebus_afsr,
			ihp->sc->sc_reg->vmebus_afar);
		return 1; /* XXX - pretend we handled it, for now */
	}

	for (; ihp; ihp = ihp->next)
		if (ihp->vec == vec && ihp->ih.ih_fun)
			i += (ihp->ih.ih_fun)(ihp->ih.ih_arg);
	return (i);
}
#endif

int
sparc_vme_intr_map(cookie, vec, pri, ihp)
	void *cookie;
	int vec;
	int pri;
	vme_intr_handle_t *ihp;
{
	struct sparc_vme_intr_handle *ih;

	ih = (vme_intr_handle_t)
	    malloc(sizeof(struct sparc_vme_intr_handle), M_DEVBUF, M_NOWAIT);
	ih->pri = pri;
	ih->vec = vec;
	ih->sc = cookie;/*XXX*/
	*ihp = ih;
	return (0);
}

void *
sparc_vme_intr_establish(cookie, vih, func, arg)
	void *cookie;
	vme_intr_handle_t vih;
	int (*func) __P((void *));
	void *arg;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
	struct sparc_vme_intr_handle *svih =
			(struct sparc_vme_intr_handle *)vih;
	struct intrhand *ih;
	int level;

	/* Translate VME priority to processor IPL */
	level = vme_ipl_to_pil[svih->pri];

	svih->ih.ih_fun = func;
	svih->ih.ih_arg = arg;
	svih->next = NULL;

	/* ensure the interrupt subsystem will call us at this level */
	for (ih = intrhand[level]; ih != NULL; ih = ih->ih_next)
		if (ih->ih_fun == sc->sc_vmeintr)
			break;

	if (ih == NULL) {
		ih = (struct intrhand *)
			malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
		if (ih == NULL)
			panic("vme_addirq");
		bzero(ih, sizeof *ih);
		ih->ih_fun = sc->sc_vmeintr;
		ih->ih_arg = vih;
		intr_establish(level, ih);
	} else {
		svih->next = (vme_intr_handle_t)ih->ih_arg;
		ih->ih_arg = vih;
	}
	return (NULL);
}

void
sparc_vme_unmap(cookie)
	void * cookie;
{
	/* Not implemented */
	panic("sparc_vme_unmap");
}

void
sparc_vme_intr_disestablish(cookie, a)
	void *cookie;
	void *a;
{
	/* Not implemented */
	panic("sparc_vme_intr_disestablish");
}



/*
 * VME DMA functions.
 */

#if defined(SUN4)
int
sparc_vme4_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	int error;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error != 0)
		return (error);

	/* Adjust DVMA address to VME view */
	map->dm_segs[0].ds_addr -= DVMA_BASE;
	return (0);
}

void
sparc_vme4_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	map->dm_segs[0].ds_addr += DVMA_BASE;
	_bus_dmamap_unload(t, map);
}

int
sparc_vme4_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	int error;

	error = _bus_dmamem_alloc(t, size, alignment, boundary,
				  segs, nsegs, rsegs, flags);
	if (error != 0)
		return (error);

	segs[0].ds_addr -= DVMA_BASE;
	return (0);
}

void
sparc_vme4_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	segs[0].ds_addr += DVMA_BASE;
	_bus_dmamem_free(t, segs, nsegs);
}

void
sparc_vme4_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * XXX Should perform cache flushes as necessary (e.g. 4/200 W/B).
	 */
}
#endif /* SUN4 */

#if defined(SUN4M)
static int
sparc_vme4m_dmamap_create (t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	int align;

	/* VME DVMA addresses must always be 8K aligned */
	align = 8192;

	/* XXX - todo: allocate DVMA addresses from assigned ranges:
		 upper 8MB for A32 space; upper 1MB for A24 space */
	return (_bus_dmamap_create(t, size, nsegments, maxsegsz,
				    boundary, /*align,*/ flags, dmamp));
}

int
sparc_vme4m_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;
	volatile u_int32_t	*ioctags;
	int			error;

	buflen = (buflen + VME_IOC_PAGESZ - 1) & ~(VME_IOC_PAGESZ - 1);
	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	if (error != 0)
		return (error);

	/* allocate IO cache entries for this range */
	ioctags = sc->sc_ioctags + VME_IOC_LINE(map->dm_segs[0].ds_addr);
	for (;buflen > 0;) {
		*ioctags = VME_IOC_IC | VME_IOC_W;
		ioctags += VME_IOC_LINESZ/sizeof(*ioctags);
		buflen -= VME_IOC_PAGESZ;
	}
	return (0);
}


void
sparc_vme4m_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;
	volatile u_int32_t	*flushregs;
	int			len;

	/* Flush VME IO cache */
	len = map->dm_segs[0].ds_len;
	flushregs = sc->sc_iocflush + VME_IOC_LINE(map->dm_segs[0].ds_addr);
	for (;len > 0;) {
		*flushregs = 0;
		flushregs += VME_IOC_LINESZ/sizeof(*flushregs);
		len -= VME_IOC_PAGESZ;
	}
	/* Read a tag to synchronize the IOC flushes */
	(*sc->sc_ioctags);

	_bus_dmamap_unload(t, map);
}

int
sparc_vme4m_dmamem_alloc(t, size, alignmnt, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignmnt, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	int error;

	error = _bus_dmamem_alloc(t, size, alignmnt, boundary,
				  segs, nsegs, rsegs, flags);
	if (error != 0)
		return (error);

	return (0);
}

void
sparc_vme4m_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	_bus_dmamem_free(t, segs, nsegs);
}

void
sparc_vme4m_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{

	/*
	 * XXX Should perform cache flushes as necessary.
	 */
}
#endif /* SUN4M */
