/*	$NetBSD: vme_machdep.c,v 1.16 1999/02/08 00:13:20 fvdl Exp $	*/

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
#include <sys/extent.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>

#include <vm/vm.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <sparc/sparc/iommuvar.h>
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
	bus_space_tag_t	 sc_bustag;
	bus_dma_tag_t	 sc_dmatag;
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
static int	vmematch_iommu  __P((struct device *, struct cfdata *, void *));
static void	vmeattach_iommu __P((struct device *, struct device *, void *));
static int	vmematch_mainbus  __P((struct device *, struct cfdata *, void *));
static void	vmeattach_mainbus __P((struct device *, struct device *, void *));
#if defined(SUN4)
int 		vmeintr4  __P((void *));
#endif
#if defined(SUN4M)
int 		vmeintr4m __P((void *));
static int	sparc_vme_error __P((void));
#endif


static int	sparc_vme_probe __P((void *, bus_space_tag_t, vme_addr_t,
				     size_t, vme_size_t, vme_mod_t,
				     int (*) __P((void *, void *)), void *));
static int	sparc_vme_map __P((void *, vme_addr_t, vme_size_t, vme_mod_t,
				   bus_space_tag_t, bus_space_handle_t *));
static void	sparc_vme_unmap __P((void *));
static int	sparc_vme_mmap_cookie __P((void *, vme_addr_t, vme_mod_t,
				   bus_space_tag_t, bus_space_handle_t *));
static int	sparc_vme_intr_map __P((void *, int, int, vme_intr_handle_t *));
static void *	sparc_vme_intr_establish __P((void *, vme_intr_handle_t,
					      int (*) __P((void *)), void *));
static void	sparc_vme_intr_disestablish __P((void *, void *));

static int	vmebus_translate __P((struct vmebus_softc *, vme_mod_t,
				      vme_addr_t, bus_type_t *, bus_addr_t *));
static void	sparc_vme_bus_establish __P((void *, struct device *));
#if defined(SUN4M)
static void	sparc_vme4m_barrier __P(( bus_space_tag_t, bus_space_handle_t,
					  bus_size_t, bus_size_t, int));

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

static int	sparc_vme_dmamem_map __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
#if 0
static void	sparc_vme_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme_dmamem_unmap __P((bus_dma_tag_t, caddr_t, size_t));
static int	sparc_vme_dmamem_mmap __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int, int, int, int));
#endif

struct cfattach vme_mainbus_ca = {
	sizeof(struct vmebus_softc), vmematch_mainbus, vmeattach_mainbus
};

struct cfattach vme_iommu_ca = {
	sizeof(struct vmebus_softc), vmematch_iommu, vmeattach_iommu
};

int	(*vmeerr_handler) __P((void));

/* If the PROM does not provide the `ranges' property, we make up our own */
struct rom_range vmebus_translations[] = {
#define _DS (VMEMOD_D|VMEMOD_S)
	{ VMEMOD_A16|_DS, 0, PMAP_VME16, 0xffff0000, 0 },
	{ VMEMOD_A24|_DS, 0, PMAP_VME16, 0xff000000, 0 },
	{ VMEMOD_A32|_DS, 0, PMAP_VME16, 0x00000000, 0 },
	{ VMEMOD_A16|VMEMOD_D32|_DS, 0, PMAP_VME32, 0xffff0000, 0 },
	{ VMEMOD_A24|VMEMOD_D32|_DS, 0, PMAP_VME32, 0xff000000, 0 },
	{ VMEMOD_A32|VMEMOD_D32|_DS, 0, PMAP_VME32, 0x00000000, 0 }
#undef _DS
};

/*
 * DMA on sun4 VME devices use the last MB of virtual space, which
 * is mapped by hardware onto the first MB of VME space.
 */
struct extent *vme_dvmamap;

struct sparc_bus_space_tag sparc_vme_bus_tag = {
	NULL, /* cookie */
	NULL, /* parent bus tag */
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
	sparc_vme_dmamem_map,
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
	sparc_vme_dmamem_map,
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
vmematch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (!CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
}

int
vmematch_iommu(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iommu_attach_args *ia = aux;

	return (strcmp(cf->cf_driver->cd_name, ia->iom_name) == 0);
}


void
vmeattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4)
	struct mainbus_attach_args *ma = aux;
	struct vmebus_softc *sc = (struct vmebus_softc *)self;
	struct vme_busattach_args vba;

	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	if (ma->ma_bp != NULL && strcmp(ma->ma_bp->name, "vme") == 0) {
		sc->sc_bp = ma->ma_bp + 1;
		bootpath_store(1, sc->sc_bp);
	}

	/* VME interrupt entry point */
	sc->sc_vmeintr = vmeintr4;

/*XXX*/	sparc_vme_chipset_tag.cookie = self;
/*XXX*/	sparc_vme4_dma_tag._cookie = self;

	vba.vba_bustag = &sparc_vme_bus_tag;
	vba.vba_chipset_tag = &sparc_vme_chipset_tag;
	vba.vba_dmatag = &sparc_vme4_dma_tag;

	/* Fall back to our own `range' construction */
	sc->sc_range = vmebus_translations;
	sc->sc_nrange =
		sizeof(vmebus_translations)/sizeof(vmebus_translations[0]);

	vme_dvmamap = extent_create("vmedvma", VME4_DVMA_BASE, VME4_DVMA_END,
				    M_DEVBUF, 0, 0, EX_NOWAIT);
	if (vme_dvmamap == NULL)
		panic("vme: unable to allocate DVMA map");

	printf("\n");
	(void)config_search(vmesearch, self, &vba);

	bootpath_store(1, NULL);
#endif
	return;
}

/* sun4m vmebus */
void
vmeattach_iommu(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4M)
	struct vmebus_softc *sc = (struct vmebus_softc *)self;
	struct iommu_attach_args *ia = aux;
	struct vme_busattach_args vba;
	bus_space_handle_t bh;
	int node;
	int cline;

	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	sc->sc_bustag = ia->iom_bustag;
	sc->sc_dmatag = ia->iom_dmatag;

	/* VME interrupt entry point */
	sc->sc_vmeintr = vmeintr4m;

/*XXX*/	sparc_vme_chipset_tag.cookie = self;
/*XXX*/	sparc_vme4m_dma_tag._cookie = self;
	sparc_vme_bus_tag.sparc_bus_barrier = sparc_vme4m_barrier;

	vba.vba_bustag = &sparc_vme_bus_tag;
	vba.vba_chipset_tag = &sparc_vme_chipset_tag;
	vba.vba_dmatag = &sparc_vme4m_dma_tag;

	node = ia->iom_node;

	/*
	 * Map VME control space
	 */
	if (ia->iom_nreg < 2) {
		printf("%s: only %d register sets\n", self->dv_xname,
			ia->iom_nreg);
		return;
	}

	if (bus_space_map2(ia->iom_bustag,
			  (bus_type_t)ia->iom_reg[0].ior_iospace,
			  (bus_addr_t)ia->iom_reg[0].ior_pa,
			  (bus_size_t)ia->iom_reg[0].ior_size,
			  BUS_SPACE_MAP_LINEAR,
			  0, &bh) != 0) {
		panic("%s: can't map vmebusreg", self->dv_xname);
	}
	sc->sc_reg = (struct vmebusreg *)bh;

	if (bus_space_map2(ia->iom_bustag,
			  (bus_type_t)ia->iom_reg[1].ior_iospace,
			  (bus_addr_t)ia->iom_reg[1].ior_pa,
			  (bus_size_t)ia->iom_reg[1].ior_size,
			  BUS_SPACE_MAP_LINEAR,
			  0, &bh) != 0) {
		panic("%s: can't map vmebusvec", self->dv_xname);
	}
	sc->sc_vec = (struct vmebusvec *)bh;

	/*
	 * Map VME IO cache tags and flush control.
	 */
	if (bus_space_map2(ia->iom_bustag,
			  (bus_type_t)ia->iom_reg[1].ior_iospace,
			  (bus_addr_t)ia->iom_reg[1].ior_pa + VME_IOC_TAGOFFSET,
			  VME_IOC_SIZE,
			  BUS_SPACE_MAP_LINEAR,
			  0, &bh) != 0) {
		panic("%s: can't map IOC tags", self->dv_xname);
	}
	sc->sc_ioctags = (u_int32_t *)bh;

	if (bus_space_map2(ia->iom_bustag,
			  (bus_type_t)ia->iom_reg[1].ior_iospace,
			  (bus_addr_t)ia->iom_reg[1].ior_pa+VME_IOC_FLUSHOFFSET,
			  VME_IOC_SIZE,
			  BUS_SPACE_MAP_LINEAR,
			  0, &bh) != 0) {
		panic("%s: can't map IOC flush registers", self->dv_xname);
	}
	sc->sc_iocflush = (u_int32_t *)bh;

/*XXX*/	sparc_vme_bus_tag.cookie = sc->sc_reg;

	/*
	 * Get "range" property.
	 */
	if (getprop(node, "ranges", sizeof(struct rom_range),
		    &sc->sc_nrange, (void **)&sc->sc_range) != 0) {
		panic("%s: can't get ranges property", self->dv_xname);
	}

	vmebus_sc = sc;
	vmeerr_handler = sparc_vme_error;

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
#endif
}

#if defined(SUN4M)
static int
sparc_vme_error()
{
	struct vmebus_softc *sc = vmebus_sc;
	u_int32_t afsr, afpa;
	char bits[64];

	afsr = sc->sc_reg->vmebus_afsr,
	afpa = sc->sc_reg->vmebus_afar;
	printf("VME error:\n\tAFSR %s\n",
		bitmask_snprintf(afsr, VMEBUS_AFSR_BITS, bits, sizeof(bits)));
	printf("\taddress: 0x%x%x\n", afsr, afpa);
	return (0);
}
#endif

int
vmebus_translate(sc, mod, addr, btp, bap)
	struct vmebus_softc *sc;
	vme_mod_t	mod;
	vme_addr_t	addr;
	bus_type_t	*btp;
	bus_addr_t	*bap;
{
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {

		if (sc->sc_range[i].cspace != mod)
			continue;

		/* We've found the connection to the parent bus */
		*bap = sc->sc_range[i].poffset + addr;
		*btp = sc->sc_range[i].pspace;
		return (0);
	}
	return (ENOENT);
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
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
	bus_type_t iospace;
	bus_addr_t paddr;

	if (vmebus_translate(sc, mod, addr, &iospace, &paddr) != 0)
		return (0);

	return (bus_space_probe(sc->sc_bustag, iospace, paddr, size, offset,
				0, callback, arg));
}

int
sparc_vme_map(cookie, addr, size, mod, tag, hp)
	void *cookie;
	vme_addr_t addr;
	vme_size_t size;
	int mod;
	bus_space_tag_t tag;
	bus_space_handle_t *hp;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(sc, mod, addr, &iospace, &paddr);
	if (error != 0)
		return (error);

	return (bus_space_map2(sc->sc_bustag, iospace, paddr, size, 0, 0, hp));
}

int
sparc_vme_mmap_cookie(cookie, addr, mod, tag, hp)
	void *cookie;
	vme_addr_t addr;
	int mod;
	bus_space_tag_t tag;
	bus_space_handle_t *hp;
{
	struct vmebus_softc *sc = (struct vmebus_softc *)cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(sc, mod, addr, &iospace, &paddr);
	if (error != 0)
		return (error);

	return (bus_space_mmap(sc->sc_bustag, iospace, paddr, 0, hp));
}

#if defined(SUN4M)
void
sparc_vme4m_barrier(t, h, offset, size, flags)
	bus_space_tag_t t;
	bus_space_handle_t h;
	bus_size_t offset;
	bus_size_t size;
	int flags;
{
	struct vmebusreg *vbp = (struct vmebusreg *)t->cookie;

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
		return (1); /* XXX - pretend we handled it, for now */
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
	bus_addr_t dvmaddr;
	bus_size_t sgsize;
	vaddr_t vaddr;
	pmap_t pmap;
	int pagesz = PAGE_SIZE;
	int error;

	error = extent_alloc(vme_dvmamap, round_page(buflen), NBPG,
			     map->_dm_boundary,
			     (flags & BUS_DMA_NOWAIT) == 0
					? EX_WAITOK
					: EX_NOWAIT,
			     (u_long *)&dvmaddr);
	if (error != 0)
		return (error);

	vaddr = (vaddr_t)buf;
	map->dm_mapsize = buflen;
	map->dm_nsegs = 1;
	map->dm_segs[0].ds_addr = dvmaddr + (vaddr & PGOFSET);
	map->dm_segs[0].ds_len = buflen;

	pmap = (p == NULL) ? pmap_kernel() : p->p_vmspace->vm_map.pmap;

	for (; buflen > 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		pa = pmap_extract(pmap, vaddr);

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = pagesz - ((u_long)vaddr & (pagesz - 1));
		if (buflen < sgsize)
			sgsize = buflen;

#ifdef notyet
		if (have_iocache)
			curaddr |= PG_IOC;
#endif
		pmap_enter(pmap_kernel(), dvmaddr,
			   (pa & ~(pagesz-1)) | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, 1);

		dvmaddr += pagesz;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	/* Adjust DVMA address to VME view */
	map->dm_segs[0].ds_addr -= VME4_DVMA_BASE;
	return (0);
}

void
sparc_vme4_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_addr_t addr;
	bus_size_t len;

	/* Go from VME to CPU view */
	map->dm_segs[0].ds_addr += VME4_DVMA_BASE;

	addr = map->dm_segs[0].ds_addr & ~PGOFSET;
	len = round_page(map->dm_segs[0].ds_len);

	/* Remove double-mapping in DVMA space */
	pmap_remove(pmap_kernel(), addr, addr + len);

	/* Release DVMA space */
	if (extent_free(vme_dvmamap, addr, len, EX_NOWAIT) != 0)
		printf("warning: %ld of DVMA space lost\n", len);

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
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
	bus_addr_t dvmaddr;
	struct pglist *mlist;
	vm_page_t m;
	paddr_t pa;
	int error;

	size = round_page(size);
	error = _bus_dmamem_alloc_common(t, size, alignment, boundary,
					 segs, nsegs, rsegs, flags);
	if (error != 0)
		return (error);

	if (extent_alloc(vme_dvmamap, size, alignment, boundary,
			 (flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT,
			 (u_long *)&dvmaddr) != 0)
		return (ENOMEM);

	/*
	 * Compute the location, size, and number of segments actually
	 * returned by the VM code.
	 */
	segs[0].ds_addr = dvmaddr - VME4_DVMA_BASE;
	segs[0].ds_len = size;
	*rsegs = 1;

	/* Map memory into DVMA space */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		pa = VM_PAGE_TO_PHYS(m);

#ifdef notyet
		if (have_iocache)
			pa |= PG_IOC;
#endif
		pmap_enter(pmap_kernel(), dvmaddr, pa | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, 1);
		dvmaddr += PAGE_SIZE;
	}

	return (0);
}

void
sparc_vme4_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{
	bus_addr_t addr;
	bus_size_t len;

	addr = segs[0].ds_addr + VME4_DVMA_BASE;
	len = round_page(segs[0].ds_len);

	/* Remove DVMA kernel map */
	pmap_remove(pmap_kernel(), addr, addr + len);

	/* Release DVMA address range */
	if (extent_free(vme_dvmamap, addr, len, EX_NOWAIT) != 0)
		printf("warning: %ld of DVMA space lost\n", len);

	/*
	 * Return the list of pages back to the VM system.
	 */
	_bus_dmamem_free_common(t, segs, nsegs);
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
	 *     Currently the cache is flushed in bus_dma_load()...
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
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;
	int error;

	/* XXX - todo: allocate DVMA addresses from assigned ranges:
		 upper 8MB for A32 space; upper 1MB for A24 space */
	error = bus_dmamap_create(sc->sc_dmatag, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error != 0)
		return (error);

#if 0
	/* VME DVMA addresses must always be 8K aligned */
	(*dmamp)->_dm_align = 8192;
#endif

	return (0);
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
	error = bus_dmamap_load(sc->sc_dmatag, map, buf, buflen, p, flags);
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

	bus_dmamap_unload(sc->sc_dmatag, map);
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
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;
	int error;

	error = bus_dmamem_alloc(sc->sc_dmatag, size, alignmnt, boundary,
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
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;

	bus_dmamem_free(sc->sc_dmatag, segs, nsegs);
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

int
sparc_vme_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct vmebus_softc	*sc = (struct vmebus_softc *)t->_cookie;

	return (bus_dmamem_map(sc->sc_dmatag, segs, nsegs, size, kvap, flags));
}
