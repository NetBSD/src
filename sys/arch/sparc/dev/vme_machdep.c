/*	$NetBSD: vme_machdep.c,v 1.31.4.1 2001/10/01 12:41:59 fvdl Exp $	*/

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
#include <sys/errno.h>

#include <sys/proc.h>
#include <sys/user.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#define _SPARC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <sparc/sparc/iommuvar.h>
#include <machine/autoconf.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/dev/vmereg.h>

struct sparcvme_softc {
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
};
struct  sparcvme_softc *sparcvme_sc;/*XXX*/

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


static int	sparc_vme_probe __P((void *, vme_addr_t, vme_size_t,
				vme_am_t, vme_datasize_t,
	int (*) __P((void *, bus_space_tag_t, bus_space_handle_t)), void *));
static int	sparc_vme_map __P((void *, vme_addr_t, vme_size_t, vme_am_t,
				   vme_datasize_t, vme_swap_t,
				   bus_space_tag_t *, bus_space_handle_t *,
				   vme_mapresc_t *));
static void	sparc_vme_unmap __P((void *, vme_mapresc_t));
static int	sparc_vme_intr_map __P((void *, int, int, vme_intr_handle_t *));
static const struct evcnt *sparc_vme_intr_evcnt __P((void *,
						     vme_intr_handle_t));
static void *	sparc_vme_intr_establish __P((void *, vme_intr_handle_t, int,
					      int (*) __P((void *)), void *));
static void	sparc_vme_intr_disestablish __P((void *, void *));

static int	vmebus_translate __P((struct sparcvme_softc *, vme_am_t,
				      vme_addr_t, bus_type_t *, bus_addr_t *));
#if defined(SUN4M)
static void	sparc_vme_iommu_barrier __P(( bus_space_tag_t, bus_space_handle_t,
					  bus_size_t, bus_size_t, int));

#endif

/*
 * DMA functions.
 */
static void	sparc_vct_dmamap_destroy __P((void *, bus_dmamap_t));

#if defined(SUN4)
static int	sparc_vct4_dmamap_create __P((void *, vme_size_t, vme_am_t,
		    vme_datasize_t, vme_swap_t, int, vme_size_t, vme_addr_t,
		    int, bus_dmamap_t *));
static int	sparc_vme4_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
static void	sparc_vme4_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme4_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));
#endif

#if defined(SUN4M)
static int	sparc_vct_iommu_dmamap_create __P((void *, vme_size_t, vme_am_t,
		    vme_datasize_t, vme_swap_t, int, vme_size_t, vme_addr_t,
		    int, bus_dmamap_t *));
static int	sparc_vme_iommu_dmamap_create __P((bus_dma_tag_t, bus_size_t,
		    int, bus_size_t, bus_size_t, int, bus_dmamap_t *));

static int	sparc_vme_iommu_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t,
		    void *, bus_size_t, struct proc *, int));
static void	sparc_vme_iommu_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme_iommu_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));
#endif

static int	sparc_vme_dmamem_map __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
#if 0
static void	sparc_vme_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
static void	sparc_vme_dmamem_unmap __P((bus_dma_tag_t, caddr_t, size_t));
static paddr_t	sparc_vme_dmamem_mmap __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int, off_t, int, int));
#endif

int sparc_vme_mmap_cookie __P((vme_addr_t, vme_am_t, bus_space_handle_t *));

struct cfattach vme_mainbus_ca = {
	sizeof(struct sparcvme_softc), vmematch_mainbus, vmeattach_mainbus
};

struct cfattach vme_iommu_ca = {
	sizeof(struct sparcvme_softc), vmematch_iommu, vmeattach_iommu
};

int	(*vmeerr_handler) __P((void));

#define VMEMOD_D32 0x40 /* ??? */

/* If the PROM does not provide the `ranges' property, we make up our own */
struct rom_range vmebus_translations[] = {
#define _DS (VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA)
	{ VME_AM_A16|_DS, 0, PMAP_VME16, 0xffff0000, 0 },
	{ VME_AM_A24|_DS, 0, PMAP_VME16, 0xff000000, 0 },
	{ VME_AM_A32|_DS, 0, PMAP_VME16, 0x00000000, 0 },
	{ VME_AM_A16|VMEMOD_D32|_DS, 0, PMAP_VME32, 0xffff0000, 0 },
	{ VME_AM_A24|VMEMOD_D32|_DS, 0, PMAP_VME32, 0xff000000, 0 },
	{ VME_AM_A32|VMEMOD_D32|_DS, 0, PMAP_VME32, 0x00000000, 0 }
#undef _DS
};

/*
 * The VME bus logic on sun4 machines maps DMA requests in the first MB
 * of VME space to the last MB of DVMA space. `vme_dvmamap' is used
 * for DVMA space allocations. The DMA addresses returned by
 * bus_dmamap_load*() must be relocated by -VME4_DVMA_BASE.
 */
struct extent *vme_dvmamap;

/*
 * The VME hardware on the sun4m IOMMU maps the first 8MB of 32-bit
 * VME space to the last 8MB of DVMA space and the first 1MB of
 * 24-bit VME space to the first 1MB of the last 8MB of DVMA space
 * (thus 24-bit VME space overlaps the first 1MB of of 32-bit space).
 * The following constants define subregions in the IOMMU DVMA map
 * for VME DVMA allocations.  The DMA addresses returned by
 * bus_dmamap_load*() must be relocated by -VME_IOMMU_DVMA_BASE.
 */
#define VME_IOMMU_DVMA_BASE		0xff800000
#define VME_IOMMU_DVMA_AM24_BASE	VME_IOMMU_DVMA_BASE
#define VME_IOMMU_DVMA_AM24_END		0xff900000
#define VME_IOMMU_DVMA_AM32_BASE	VME_IOMMU_DVMA_BASE
#define VME_IOMMU_DVMA_AM32_END		IOMMU_DVMA_END

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
	sparc_vme_map,
	sparc_vme_unmap,
	sparc_vme_probe,
	sparc_vme_intr_map,
	sparc_vme_intr_evcnt,
	sparc_vme_intr_establish,
	sparc_vme_intr_disestablish,
	0, 0, 0 /* bus specific DMA stuff */
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

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	sparc_vme_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};
#endif

#if defined(SUN4M)
struct sparc_bus_dma_tag sparc_vme_iommu_dma_tag = {
	NULL,	/* cookie */
	sparc_vme_iommu_dmamap_create,
	_bus_dmamap_destroy,
	sparc_vme_iommu_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	sparc_vme_iommu_dmamap_unload,
	sparc_vme_iommu_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	sparc_vme_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};
#endif


int
vmematch_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (!CPU_ISSUN4)
		return (0);

	return (strcmp("vme", ma->ma_name) == 0);
}

int
vmematch_iommu(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iommu_attach_args *ia = aux;

	return (strcmp("vme", ia->iom_name) == 0);
}


void
vmeattach_mainbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4)
	struct mainbus_attach_args *ma = aux;
	struct sparcvme_softc *sc = (struct sparcvme_softc *)self;
	struct vmebus_attach_args vba;

	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/* VME interrupt entry point */
	sc->sc_vmeintr = vmeintr4;

/*XXX*/	sparc_vme_chipset_tag.cookie = self;
/*XXX*/	sparc_vme_chipset_tag.vct_dmamap_create = sparc_vct4_dmamap_create;
/*XXX*/	sparc_vme_chipset_tag.vct_dmamap_destroy = sparc_vct_dmamap_destroy;
/*XXX*/	sparc_vme4_dma_tag._cookie = self;

#if 0
	sparc_vme_bus_tag.parent = ma->ma_bustag;
	vba.vba_bustag = &sparc_vme_bus_tag;
#endif
	vba.va_vct = &sparc_vme_chipset_tag;
	vba.va_bdt = &sparc_vme4_dma_tag;
	vba.va_slaveconfig = 0;

	/* Fall back to our own `range' construction */
	sc->sc_range = vmebus_translations;
	sc->sc_nrange =
		sizeof(vmebus_translations)/sizeof(vmebus_translations[0]);

	vme_dvmamap = extent_create("vmedvma", VME4_DVMA_BASE, VME4_DVMA_END,
				    M_DEVBUF, 0, 0, EX_NOWAIT);
	if (vme_dvmamap == NULL)
		panic("vme: unable to allocate DVMA map");

	printf("\n");
	(void)config_found(self, &vba, 0);

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
	struct sparcvme_softc *sc = (struct sparcvme_softc *)self;
	struct iommu_attach_args *ia = aux;
	struct vmebus_attach_args vba;
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
/*XXX*/	sparc_vme_chipset_tag.vct_dmamap_create = sparc_vct_iommu_dmamap_create;
/*XXX*/	sparc_vme_chipset_tag.vct_dmamap_destroy = sparc_vct_dmamap_destroy;
/*XXX*/	sparc_vme_iommu_dma_tag._cookie = self;
	sparc_vme_bus_tag.sparc_bus_barrier = sparc_vme_iommu_barrier;

#if 0
	vba.vba_bustag = &sparc_vme_bus_tag;
#endif
	vba.va_vct = &sparc_vme_chipset_tag;
	vba.va_bdt = &sparc_vme_iommu_dma_tag;
	vba.va_slaveconfig = 0;

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
	if (PROM_getprop(node, "ranges", sizeof(struct rom_range),
		    &sc->sc_nrange, (void **)&sc->sc_range) != 0) {
		panic("%s: can't get ranges property", self->dv_xname);
	}

	sparcvme_sc = sc;
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

	(void)config_found(self, &vba, 0);
#endif
}

#if defined(SUN4M)
static int
sparc_vme_error()
{
	struct sparcvme_softc *sc = sparcvme_sc;
	u_int32_t afsr, afpa;
	char bits[64];

	afsr = sc->sc_reg->vmebus_afsr;
	afpa = sc->sc_reg->vmebus_afar;
	printf("VME error:\n\tAFSR %s\n",
		bitmask_snprintf(afsr, VMEBUS_AFSR_BITS, bits, sizeof(bits)));
	printf("\taddress: 0x%x%x\n", afsr, afpa);
	return (0);
}
#endif

int
vmebus_translate(sc, mod, addr, btp, bap)
	struct sparcvme_softc *sc;
	vme_am_t	mod;
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

struct vmeprobe_myarg {
	int (*cb) __P((void *, bus_space_tag_t, bus_space_handle_t));
	void *cbarg;
	bus_space_tag_t tag;
	int res; /* backwards */
};

static int vmeprobe_mycb __P((void *, void *));
static int
vmeprobe_mycb(bh, arg)
	void *bh, *arg;
{
	struct vmeprobe_myarg *a = arg;

	a->res = (*a->cb)(a->cbarg, a->tag, (bus_space_handle_t)bh);
	return (!a->res);
}

int
sparc_vme_probe(cookie, addr, len, mod, datasize, callback, arg)
	void *cookie;
	vme_addr_t addr;
	vme_size_t len;
	vme_am_t mod;
	vme_datasize_t datasize;
	int (*callback) __P((void *, bus_space_tag_t, bus_space_handle_t));
	void *arg;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	bus_size_t size;
	struct vmeprobe_myarg myarg;
	int res, i;

	if (vmebus_translate(sc, mod, addr, &iospace, &paddr) != 0)
		return (EINVAL);

	size = (datasize == VME_D8 ? 1 : (datasize == VME_D16 ? 2 : 4));

	if (callback) {
		myarg.cb = callback;
		myarg.cbarg = arg;
		myarg.tag = sc->sc_bustag;
		myarg.res = 0;
		res = bus_space_probe(sc->sc_bustag, iospace, paddr, size, 0,
				      0, vmeprobe_mycb, &myarg);
		return (res ? 0 : (myarg.res ? myarg.res : EIO));
	}

	for (i = 0; i < len / size; i++) {
		myarg.res = 0;
		res = bus_space_probe(sc->sc_bustag, iospace, paddr, size, 0,
				      0, 0, 0);
		if (res == 0)
			return (EIO);
		paddr += size;
	}
	return (0);
}

int
sparc_vme_map(cookie, addr, size, mod, datasize, swap, tp, hp, rp)
	void *cookie;
	vme_addr_t addr;
	vme_size_t size;
	vme_am_t mod;
	vme_datasize_t datasize;
	vme_swap_t swap;
	bus_space_tag_t *tp;
	bus_space_handle_t *hp;
	vme_mapresc_t *rp;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(sc, mod, addr, &iospace, &paddr);
	if (error != 0)
		return (error);

	*tp = sc->sc_bustag;
	return (bus_space_map2(sc->sc_bustag, iospace, paddr, size, 0, 0, hp));
}

int
sparc_vme_mmap_cookie(addr, mod, hp)
	vme_addr_t addr;
	vme_am_t mod;
	bus_space_handle_t *hp;
{
	struct sparcvme_softc *sc = sparcvme_sc;
	bus_type_t iospace;
	bus_addr_t paddr;
	int error;

	error = vmebus_translate(sc, mod, addr, &iospace, &paddr);
	if (error != 0)
		return (error);

	return (bus_space_mmap(sc->sc_bustag, BUS_ADDR(iospace, paddr), 0, 
		0/*prot is ignored*/, 0));
}

#if defined(SUN4M)
void
sparc_vme_iommu_barrier(t, h, offset, size, flags)
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
	struct sparcvme_softc *sc;/*XXX*/
};

#if defined(SUN4)
int
vmeintr4(arg)
	void *arg;
{
	struct sparc_vme_intr_handle *ihp = (vme_intr_handle_t)arg;
	int level, vec;
	int rv = 0;

	level = (ihp->pri << 1) | 1;

	vec = ldcontrolb((caddr_t)(AC_VMEINTVEC | level));

	if (vec == -1) {
#ifdef DEBUG
		/*
		 * This seems to happen only with the i82586 based
		 * `ie1' boards.
		 */
		printf("vme: spurious interrupt at VME level %d\n", ihp->pri);
#endif
		return (1); /* XXX - pretend we handled it, for now */
	}

	for (; ihp; ihp = ihp->next)
		if (ihp->vec == vec && ihp->ih.ih_fun)
			rv |= (ihp->ih.ih_fun)(ihp->ih.ih_arg);

	return (rv);
}
#endif

#if defined(SUN4M)
int
vmeintr4m(arg)
	void *arg;
{
	struct sparc_vme_intr_handle *ihp = (vme_intr_handle_t)arg;
	int level, vec;
	int rv = 0;

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
#ifdef DEBUG
		/*
		 * This seems to happen only with the i82586 based
		 * `ie1' boards.
		 */
		printf("vme: spurious interrupt at VME level %d\n", ihp->pri);
		printf("    ICR_SI_PEND=0x%x; VME AFSR=0x%x; VME AFAR=0x%x\n",
			*((int*)ICR_SI_PEND),
			ihp->sc->sc_reg->vmebus_afsr,
			ihp->sc->sc_reg->vmebus_afar);
#endif
		return (1); /* XXX - pretend we handled it, for now */
	}

	for (; ihp; ihp = ihp->next)
		if (ihp->vec == vec && ihp->ih.ih_fun)
			rv |= (ihp->ih.ih_fun)(ihp->ih.ih_arg);

	return (rv);
}
#endif

int
sparc_vme_intr_map(cookie, level, vec, ihp)
	void *cookie;
	int level;
	int vec;
	vme_intr_handle_t *ihp;
{
	struct sparc_vme_intr_handle *ih;

	ih = (vme_intr_handle_t)
	    malloc(sizeof(struct sparc_vme_intr_handle), M_DEVBUF, M_NOWAIT);
	ih->pri = level;
	ih->vec = vec;
	ih->sc = cookie;/*XXX*/
	*ihp = ih;
	return (0);
}

const struct evcnt *
sparc_vme_intr_evcnt(cookie, vih)
	void *cookie;
	vme_intr_handle_t vih;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
sparc_vme_intr_establish(cookie, vih, pri, func, arg)
	void *cookie;
	vme_intr_handle_t vih;
	int pri;
	int (*func) __P((void *));
	void *arg;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;
	struct sparc_vme_intr_handle *svih =
			(struct sparc_vme_intr_handle *)vih;
	struct intrhand *ih;
	int level;

	/* XXX pri == svih->pri ??? */

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
sparc_vme_unmap(cookie, resc)
	void * cookie;
	vme_mapresc_t resc;
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

static void
sparc_vct_dmamap_destroy(cookie, map)
	void *cookie;
	bus_dmamap_t map;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;
	bus_dmamap_destroy(sc->sc_dmatag, map);
}

#if defined(SUN4)
static int
sparc_vct4_dmamap_create(cookie, size, am, datasize, swap, nsegments, maxsegsz,
			  boundary, flags, dmamp)
	void *cookie;
	vme_size_t size;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	int nsegments;
	vme_size_t maxsegsz;
	vme_addr_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;

	/* Allocate a base map through parent bus ops */
	return (bus_dmamap_create(sc->sc_dmatag, size, nsegments, maxsegsz,
				  boundary, flags, dmamp));
}

int
sparc_vme4_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_addr_t dva;
	bus_size_t sgsize;
	vaddr_t va, voff;
	pmap_t pmap;
	int pagesz = PAGE_SIZE;
	int error;

	cpuinfo.cache_flush(buf, buflen); /* XXX - move to bus_dma_sync */

	va = (vaddr_t)buf;
	voff = va & (pagesz - 1);
	va &= -pagesz;

	/*
	 * Allocate an integral number of pages from DVMA space
	 * covering the passed buffer.
	 */
	sgsize = (buflen + voff + pagesz - 1) & -pagesz;
	error = extent_alloc(vme_dvmamap, sgsize, pagesz,
			     map->_dm_boundary,
			     (flags & BUS_DMA_NOWAIT) == 0
					? EX_WAITOK
					: EX_NOWAIT,
			     (u_long *)&dva);
	if (error != 0)
		return (error);

	map->dm_mapsize = buflen;
	map->dm_nsegs = 1;
	/* Adjust DVMA address to VME view */
	map->dm_segs[0].ds_addr = dva + voff - VME4_DVMA_BASE;
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_sgsize = sgsize;

	pmap = (p == NULL) ? pmap_kernel() : p->p_vmspace->vm_map.pmap;

	for (; sgsize != 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		(void) pmap_extract(pmap, va, &pa);

#ifdef notyet
		if (have_iocache)
			pa |= PG_IOC;
#endif
		pmap_enter(pmap_kernel(), dva,
			   pa | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);

		dva += pagesz;
		va += pagesz;
		sgsize -= pagesz;
	}
	pmap_update(pmap_kernel());

	return (0);
}

void
sparc_vme4_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_dma_segment_t *segs = map->dm_segs;
	int nsegs = map->dm_nsegs;
	bus_addr_t dva;
	bus_size_t len;
	int i, s, error;

	for (i = 0; i < nsegs; i++) {
		/* Go from VME to CPU view */
		dva = segs[i].ds_addr + VME4_DVMA_BASE;
		dva &= -PAGE_SIZE;
		len = segs[i]._ds_sgsize;

		/* Remove double-mapping in DVMA space */
		pmap_remove(pmap_kernel(), dva, dva + len);

		/* Release DVMA space */
		s = splhigh();
		error = extent_free(vme_dvmamap, dva, len, EX_NOWAIT);
		splx(s);
		if (error != 0)
			printf("warning: %ld of DVMA space lost\n", len);
	}
	pmap_update(pmap_kernel());

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
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
sparc_vme_iommu_dmamap_create (t, size, nsegments, maxsegsz,
				boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{

	printf("sparc_vme_dmamap_create: please use `vme_dmamap_create'\n");
	return (EINVAL);
}

static int
sparc_vct_iommu_dmamap_create(cookie, size, am, datasize, swap, nsegments,
			      maxsegsz, boundary, flags, dmamp)
	void *cookie;
	vme_size_t size;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	int nsegments;
	vme_size_t maxsegsz;
	vme_addr_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sparcvme_softc *sc = (struct sparcvme_softc *)cookie;
	bus_dmamap_t map;
	int error;

	/* Allocate a base map through parent bus ops */
	error = bus_dmamap_create(sc->sc_dmatag, size, nsegments, maxsegsz,
				  boundary, flags, &map);
	if (error != 0)
		return (error);

	/*
	 * Each I/O cache line maps to a 8K section of VME DVMA space, so
	 * we must ensure that DVMA alloctions are always 8K aligned.
	 */
	map->_dm_align = VME_IOC_PAGESZ;

	/* Set map region based on Address Modifier */
	switch ((am & VME_AM_ADRSIZEMASK)) {
	case VME_AM_A16:
	case VME_AM_A24:
		/* 1 MB of DVMA space */
		map->_dm_ex_start = VME_IOMMU_DVMA_AM24_BASE;
		map->_dm_ex_end   = VME_IOMMU_DVMA_AM24_END;
		break;
	case VME_AM_A32:
		/* 8 MB of DVMA space */
		map->_dm_ex_start = VME_IOMMU_DVMA_AM32_BASE;
		map->_dm_ex_end   = VME_IOMMU_DVMA_AM32_END;
		break;
	}

	*dmamp = map;
	return (0);
}

int
sparc_vme_iommu_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct sparcvme_softc	*sc = (struct sparcvme_softc *)t->_cookie;
	volatile u_int32_t	*ioctags;
	int			error;

	/* Round request to a multiple of the I/O cache size */
	buflen = (buflen + VME_IOC_PAGESZ - 1) & -VME_IOC_PAGESZ;
	error = bus_dmamap_load(sc->sc_dmatag, map, buf, buflen, p, flags);
	if (error != 0)
		return (error);

	/* Allocate I/O cache entries for this range */
	ioctags = sc->sc_ioctags + VME_IOC_LINE(map->dm_segs[0].ds_addr);
	while (buflen > 0) {
		*ioctags = VME_IOC_IC | VME_IOC_W;
		ioctags += VME_IOC_LINESZ/sizeof(*ioctags);
		buflen -= VME_IOC_PAGESZ;
	}

	/*
	 * Adjust DVMA address to VME view.
	 * Note: the DVMA base address is the same for all
	 * VME address spaces.
	 */
	map->dm_segs[0].ds_addr -= VME_IOMMU_DVMA_BASE;
	return (0);
}


void
sparc_vme_iommu_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	struct sparcvme_softc	*sc = (struct sparcvme_softc *)t->_cookie;
	volatile u_int32_t	*flushregs;
	int			len;

	/* Go from VME to CPU view */
	map->dm_segs[0].ds_addr += VME_IOMMU_DVMA_BASE;

	/* Flush VME I/O cache */
	len = map->dm_segs[0]._ds_sgsize;
	flushregs = sc->sc_iocflush + VME_IOC_LINE(map->dm_segs[0].ds_addr);
	while (len > 0) {
		*flushregs = 0;
		flushregs += VME_IOC_LINESZ/sizeof(*flushregs);
		len -= VME_IOC_PAGESZ;
	}

	/*
	 * Start a read from `tag space' which will not complete until
	 * all cache flushes have finished
	 */
	(*sc->sc_ioctags);

	bus_dmamap_unload(sc->sc_dmatag, map);
}

void
sparc_vme_iommu_dmamap_sync(t, map, offset, len, ops)
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
	struct sparcvme_softc	*sc = (struct sparcvme_softc *)t->_cookie;

	return (bus_dmamem_map(sc->sc_dmatag, segs, nsegs, size, kvap, flags));
}
