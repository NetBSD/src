/*	$NetBSD: sh5_pci.c,v 1.13.26.1 2007/03/12 05:50:16 rmind Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH-5 Host-PCI Bridge Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sh5_pci.c,v 1.13.26.1 2007/03/12 05:50:16 rmind Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#if defined(PCI_NETBSD_CONFIGURE)
#include <dev/pci/pciconf.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/superhywayvar.h>

#include <sh5/pci/sh5_pcivar.h>
#include <sh5/pci/sh5_pcireg.h>

#include "locators.h"


/*
 * XXX: The following is good enough for 1GB of RAM, starting
 * at physical address 0x80000000.
 *
 * Need to revist this to make it configurable by board-specific
 * code at runtime.
 */
#define	SH5PCI_RAM_PHYS_BASE	0x80000000


struct sh5pci_map {
	bus_addr_t m_start;
	bus_addr_t m_end;
	bus_addr_t m_pcibase;
};

struct sh5pci_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_csrh;
	bus_addr_t sc_base;
	struct sh5pci_map sc_map[SH5PCI_NUM_MBARS];
	const struct sh5pci_intr_hooks *sc_intr;
	void *sc_intr_arg;
	void *sc_ih_serr;
	void *sc_ih_err;
};

struct sh5pci_icookie {
	pci_intr_handle_t ic_ih;
	int (*ic_func)(void *);
	void *ic_arg;
	SLIST_ENTRY(sh5pci_icookie) ic_next;
};

static int	sh5pcimatch(struct device *, struct cfdata *, void *);
static void	sh5pciattach(struct device *, struct device *, void *);
static int	sh5pciprint(void *, const char *);

CFATTACH_DECL(sh5pci, sizeof(struct sh5pci_softc),
    sh5pcimatch, sh5pciattach, NULL, NULL);
extern struct cfdriver sh5pci_cd;

static int	sh5pci_dmamap_create(void *, bus_size_t, int, bus_size_t,
		    bus_size_t, int, bus_dmamap_t *);
static void	sh5pci_dmamap_destroy(void *, bus_dmamap_t);
static int	sh5pci_dmamap_load_direct(void *, bus_dmamap_t,
		    void *, bus_size_t, struct proc *, int);
static int	sh5pci_dmamap_load_mbuf(void *,
		    bus_dmamap_t, struct mbuf *, int);
static int	sh5pci_dmamap_load_uio(void *, bus_dmamap_t,
		    struct uio *, int);
static int	sh5pci_dmamap_load_raw(void *,
		    bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int);
static int	sh5pci_dmamap_load_common(struct sh5pci_softc *, bus_dmamap_t);
static void	sh5pci_dmamap_unload(void *, bus_dmamap_t);
static void	sh5pci_dmamap_sync(void *, bus_dmamap_t, bus_addr_t,
		    bus_size_t, int);
static int	sh5pci_dmamem_alloc(void *, bus_size_t, bus_size_t, bus_size_t,
		    bus_dma_segment_t *, int, int *, int);
static void	sh5pci_dmamem_free(void *, bus_dma_segment_t *, int);
static int	sh5pci_dmamem_map(void *, bus_dma_segment_t *, int, size_t,
		    void **, int);
static void	sh5pci_dmamem_unmap(void *, void *, size_t);
static paddr_t	sh5pci_dmamem_mmap(void *, bus_dma_segment_t *, int,
		    off_t, int, int);

static int	sh5pci_mem_map(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *);
static int	sh5pci_io_map(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *);

static void	sh5pci_attach_hook(void *, struct device *, struct device *,
		    struct pcibus_attach_args *);
static int	sh5pci_maxdevs(void *, int);
static pcitag_t	sh5pci_make_tag(void *, int, int, int);
static void	sh5pci_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t	sh5pci_conf_read(void *, pcitag_t, int);
static void	sh5pci_conf_write(void *, pcitag_t, int, pcireg_t);
static void	sh5pci_conf_interrupt(void *, int, int, int, int, int *);
static int	sh5pci_intr_map(void *, struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *sh5pci_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *sh5pci_intr_evcnt(void *, pci_intr_handle_t);
static void *	sh5pci_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	sh5pci_intr_disestablish(void *, void *);
static int	sh5pci_intr_dispatch(void *);

static void	sh5pci_bridge_init(struct sh5pci_softc *);
static int	sh5pci_check_master_abort(struct sh5pci_softc *);
static int	sh5pci_interrupt(void *);

/*
 * XXX: These should be allocated dynamically to allow multiple instances.
 */
static struct sh5_bus_space_tag sh5pci_mem_tag;
static struct sh5_bus_space_tag sh5pci_io_tag;
static struct sh5_bus_dma_tag sh5pci_dma_tag = {
	NULL,
	sh5pci_dmamap_create,
	sh5pci_dmamap_destroy,
	sh5pci_dmamap_load_direct,
	sh5pci_dmamap_load_mbuf,
	sh5pci_dmamap_load_uio,
	sh5pci_dmamap_load_raw,
	sh5pci_dmamap_unload,
	sh5pci_dmamap_sync,
	sh5pci_dmamem_alloc,
	sh5pci_dmamem_free,
	sh5pci_dmamem_map,
	sh5pci_dmamem_unmap,
	sh5pci_dmamem_mmap,
};
static struct sh5_pci_chipset_tag sh5pci_chipset_tag = {
	NULL,
	sh5pci_attach_hook,
	sh5pci_maxdevs,
	sh5pci_make_tag,
	sh5pci_decompose_tag,
	sh5pci_conf_read,
	sh5pci_conf_write,
	sh5pci_conf_interrupt,
	sh5pci_intr_map,
	sh5pci_intr_string,
	sh5pci_intr_evcnt,
	sh5pci_intr_establish,
	sh5pci_intr_disestablish
};

#define	sh5pci_csr_read(sc, reg) \
	    bus_space_read_4((sc)->sc_bust, (sc)->sc_csrh, (reg))
#define	sh5pci_csr_write(sc, reg, val) \
	    bus_space_write_4((sc)->sc_bust, (sc)->sc_csrh, (reg), (val))

/*ARGSUSED*/
static int
sh5pcimatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	bus_addr_t vcrbase;
	u_int64_t vcr;

	if (strcmp(sa->sa_name, sh5pci_cd.cd_name))
		return (0);

	sa->sa_pport = 0;

	vcrbase = SUPERHYWAY_PPORT_TO_BUSADDR(cf->cf_loc[SUPERHYWAYCF_PPORT]);

	bus_space_map(sa->sa_bust, vcrbase + SH5PCI_VCR_OFFSET,
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	if (SUPERHYWAY_VCR_MOD_ID(vcr) != SH5PCI_MODULE_ID)
		return (0);

	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
sh5pciattach(struct device *parent, struct device *self, void *args)
{
	struct sh5pci_softc *sc = (struct sh5pci_softc *)self;
	struct superhyway_attach_args *sa = args;
	struct pcibus_attach_args pba;
	bus_space_handle_t bh;
	u_int64_t vcr;
#if defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
	u_long cfg_ioaddr;
#endif

	sc->sc_bust = sa->sa_bust;
	sc->sc_dmat = sa->sa_dmat;
	sc->sc_base = SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport);

	/* Fetch the VCR */
	bus_space_map(sc->sc_bust, sc->sc_base + SH5PCI_VCR_OFFSET,
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sc->sc_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sc->sc_bust, bh, SUPERHYWAY_REG_SZ);

	/*
	 * Map the PCI CSR Registers
	 */
	bus_space_map(sa->sa_bust, sc->sc_base + SH5PCI_CSR_OFFSET,
	    SH5PCI_CSR_SIZE, 0, &sc->sc_csrh);

	printf(": SH-5 PCIbus Bridge, Version 0x%x\n",
	    (int)SUPERHYWAY_VCR_MOD_VERS(vcr));

#ifdef DEBUG
	printf("%s: CSR at %p\n", sc->sc_dev.dv_xname, (void *)sc->sc_csrh);
#endif

	/*
	 * Fix up the memory and i/o tags by copying our
	 * parent's tag, and modifying the cookie and bus_space_map fields.
	 *
	 * This is a wee bit naughty; we really shouldn't interpret our
	 * parent's tag, but it's a lot more efficient than writing a whole
	 * bunch of stubs which just call the parent's methods.
	 *
	 * XXX: We get away with this because we *know* our parent is
	 * the base-level bus_space(9) implementation; which doesn't
	 * interpret the "cookie" field of the tag...
	 */
	sh5pci_mem_tag = *sc->sc_bust;
	sh5pci_mem_tag.bs_cookie = sc;
	sh5pci_mem_tag.bs_map = sh5pci_mem_map;
	sh5pci_io_tag = *sc->sc_bust;
	sh5pci_io_tag.bs_cookie = sc;
	sh5pci_io_tag.bs_map = sh5pci_io_map;

	/*
	 * Initialise our DMA tag
	 */
	sh5pci_dma_tag.bd_cookie = sc;

	/*
	 * Initialise the cookie field of our chipset tag
	 */
	sh5pci_chipset_tag.ct_cookie = sc;

	/*
	 * Connect to, and initialise, the board-specific interrupt
	 * routing interface.
	 */
	sc->sc_intr = sh5pci_get_intr_hooks(&sh5pci_chipset_tag);
	sc->sc_intr_arg = (sc->sc_intr->ih_init)(&sh5pci_chipset_tag,
	    &sc->sc_ih_serr, sh5pci_interrupt, sc,
	    &sc->sc_ih_err, sh5pci_interrupt, sc);

	/*
	 * Initialise the host-pci hardware
	 */
	sh5pci_bridge_init(sc);

#if defined(PCI_NETBSD_CONFIGURE)
	/*
	 * Configure the devices on the PCIbus
	 */
	memext = extent_create("pcimem", sh5pci_csr_read(sc, SH5PCI_CSR_MBR),
	    sh5pci_csr_read(sc, SH5PCI_CSR_MBR) + (SH5PCI_MEMORY_SIZE - 1),
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	ioext  = extent_create("pciio", sh5pci_csr_read(sc, SH5PCI_CSR_IOBR),
	    sh5pci_csr_read(sc, SH5PCI_CSR_IOBR) + (SH5PCI_IO_SIZE - 1),
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	/*
	 * Reserve the lowest 256 bytes of i/o space. Some (older) PCI
	 * devices don't like to be assigned such low addresses...
	 */
	extent_alloc(ioext, 0x100, 0x100, 0, EX_NOWAIT, &cfg_ioaddr);

	/*
	 * The SH5 Host-PCI bridge appears to be unable to see its own
	 * configuration registers in PCI config space, so manually fix
	 * up some values.
	 */
	sh5pci_csr_write(sc, SH5PCI_CONF_IOBAR, 0x40000);
	{
		u_int32_t reg;
		reg = sh5pci_csr_read(sc, PCI_BHLC_REG);
		reg |= (0x80 << PCI_LATTIMER_SHIFT);
		sh5pci_csr_write(sc, PCI_BHLC_REG, reg);
	}

	/*
	 * Configure up the PCI bus
	 */
	pci_configure_bus(&sh5pci_chipset_tag, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);

	/* Clear any accumulated master aborts */
	(void) sh5pci_check_master_abort(sc);
#endif

	pba.pba_pc        = &sh5pci_chipset_tag;
	pba.pba_bus       = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags     = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	pba.pba_dmat      = &sh5pci_dma_tag;
	pba.pba_iot       = &sh5pci_io_tag;
	pba.pba_memt      = &sh5pci_mem_tag;
	config_found_ia(self, "pcibus", &pba, sh5pciprint);
}

/*ARGSUSED*/
static int
sh5pciprint(void *arg, const char *cp)
{
	if (cp == NULL)
		return (UNCONF);
	return (QUIET);
}

static int
sh5pci_dmamap_create(void *arg, bus_size_t size, int nsegs,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	struct sh5pci_softc *sc = arg;

	return (bus_dmamap_create(sc->sc_dmat, size, nsegs, maxsegsz,
	    boundary, flags, dmamp));
}

static void
sh5pci_dmamap_destroy(void *arg, bus_dmamap_t map)
{
	struct sh5pci_softc *sc = arg;

	bus_dmamap_destroy(sc->sc_dmat, map);
}

static int
sh5pci_dmamap_load_direct(void *arg, bus_dmamap_t map,
    void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	struct sh5pci_softc *sc = arg;
	int rv;

	rv = bus_dmamap_load(sc->sc_dmat, map, buf, buflen, p, flags);
	if (rv != 0)
		return (rv);

	return (sh5pci_dmamap_load_common(sc, map));
}

static int
sh5pci_dmamap_load_mbuf(void *arg, bus_dmamap_t map, struct mbuf *m, int flags)
{
	struct sh5pci_softc *sc = arg;
	int rv;

	rv = bus_dmamap_load_mbuf(sc->sc_dmat, map, m, flags);
	if (rv != 0)
		return (rv);

	return (sh5pci_dmamap_load_common(sc, map));
}

static int
sh5pci_dmamap_load_uio(void *arg, bus_dmamap_t map, struct uio *uio, int flags)
{
	struct sh5pci_softc *sc = arg;
	int rv;

	rv = bus_dmamap_load_uio(sc->sc_dmat, map, uio, flags);
	if (rv != 0)
		return (rv);

	return (sh5pci_dmamap_load_common(sc, map));
}

static int
sh5pci_dmamap_load_raw(void *arg, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	struct sh5pci_softc *sc = arg;
	int rv;

	rv = bus_dmamap_load_raw(sc->sc_dmat, map, segs, nsegs, size, flags);
	if (rv != 0)
		return (rv);

	return (sh5pci_dmamap_load_common(sc, map));
}

static int
sh5pci_dmamap_load_common(struct sh5pci_softc *sc, bus_dmamap_t map)
{
	bus_dma_segment_t *ds;
	u_int32_t mask;
	int rv, i;

	/*
	 * If all goes well, "rv" will be zero at the end of the loop.
	 * (It is decremented each time we successfully translate
	 * a segment).
	 */
	rv = map->dm_nsegs;

	/*
	 * Traverse the list of segments which make up this map, and
	 * convert the CPU-relative addresses therein to PCIbus addresses.
	 */
	for (ds = &map->dm_segs[0]; ds < &map->dm_segs[map->dm_nsegs]; ds++) {
		for (i = 0; i < SH5PCI_NUM_MBARS; i++) {
			if (sc->sc_map[i].m_start == ~0)
				continue;

			if (ds->_ds_cpuaddr < sc->sc_map[i].m_start ||
			    (ds->_ds_cpuaddr + ds->ds_len) >=
			     sc->sc_map[i].m_end)
				continue;

			mask = sc->sc_map[i].m_end - sc->sc_map[i].m_start;
			mask -= 1;

			/*
			 * Looks like we found the window through which this
			 * segment is visible. Convert to PCIbus address.
			 */
			ds->ds_addr = sc->sc_map[i].m_pcibase +
			    (ds->_ds_cpuaddr & mask);
			rv -= 1;
			break;
		}
	}

	return (rv ? 1 : 0);
}

static void
sh5pci_dmamap_unload(void *arg, bus_dmamap_t map)
{
	struct sh5pci_softc *sc = arg;

	/* XXX Deal with bounce buffers (for when we have > 1GB RAM) */

	bus_dmamap_unload(sc->sc_dmat, map);
}

static void
sh5pci_dmamap_sync(void *arg, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct sh5pci_softc *sc = arg;

	/* XXX Deal with bounce buffers (for when we have > 1GB RAM) */

	bus_dmamap_sync(sc->sc_dmat, map, offset, len, ops);
}

static int
sh5pci_dmamem_alloc(void *arg, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs,
    int *rsegs, int flags)
{
	struct sh5pci_softc *sc = arg;

	/* XXX Deal with systems with > 1GB RAM */

	/*
	 * Allocate physical memory.
	 *
	 * Note: This fills in the segments with CPU-relative physical
	 * addresses. A further call to bus_dmamap_load_raw() must be
	 * made before the addresses in the segments can be used.
	 * The segments of the DMA map will then contain PCIbus-relative
	 * physical addresses of the memory allocated here.
	 */
	return (bus_dmamem_alloc(sc->sc_dmat, size, alignment, boundary,
	    segs, nsegs, rsegs, flags));
}

static void
sh5pci_dmamem_free(void *arg, bus_dma_segment_t *segs, int nsegs)
{
	struct sh5pci_softc *sc = arg;

	bus_dmamem_free(sc->sc_dmat, segs, nsegs);
}

static int
sh5pci_dmamem_map(void *arg, bus_dma_segment_t *segs, int nsegs,
    size_t size, void **kvap, int flags)
{
	struct sh5pci_softc *sc = arg;

	return (bus_dmamem_map(sc->sc_dmat, segs, nsegs, size, kvap, flags));
}

static void
sh5pci_dmamem_unmap(void *arg, void *kva, size_t size)
{
	struct sh5pci_softc *sc = arg;

	bus_dmamem_unmap(sc->sc_dmat, kva, size);
}

static paddr_t
sh5pci_dmamem_mmap(void *arg, bus_dma_segment_t *segs, int nsegs,
    off_t off, int prot, int flags)
{
	struct sh5pci_softc *sc = arg;

	return (bus_dmamem_mmap(sc->sc_dmat, segs, nsegs, off, prot, flags));
}

static int
sh5pci_mem_map(void *arg, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bushp)
{
	struct sh5pci_softc *sc = arg;
	u_int32_t mbr;

	mbr = sh5pci_csr_read(sc, SH5PCI_CSR_MBR);

	/*
	 * Can we access the required PCIbus address range through
	 * our window?
	 */
	if (addr < mbr)
		return (EINVAL);

	/*
	 * Convert the PCIbus address to an offset within the window
	 */
	addr -= mbr;

	/*
	 * One final check that the address range fits inside the window.
	 */
	if ((addr + size) >= SH5PCI_MEMORY_SIZE)
		return (EINVAL);

	/*
	 * Convert to the correct offset into our SuperHyway address space
	 * before mapping in the normal way.
	 */
	addr += sc->sc_base + SH5PCI_MEMORY_OFFSET;

	return (bus_space_map(sc->sc_bust, addr, size, flags, bushp));
}

static int
sh5pci_io_map(void *arg, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bushp)
{
	struct sh5pci_softc *sc = arg;
	u_int32_t iobr;

	iobr = sh5pci_csr_read(sc, SH5PCI_CSR_IOBR);

	/*
	 * Can we access the required PCIbus address range through
	 * our window?
	 */
	if (addr < iobr)
		return (EINVAL);

	/*
	 * Convert the PCIbus address to an offset within the window
	 */
	addr -= iobr;

	/*
	 * One final check that the address range fits inside the window.
	 */
	if ((addr + size) >= SH5PCI_IO_SIZE)
		return (EINVAL);

	/*
	 * Convert to the correct offset into our SuperHyway address space
	 * before mapping in the normal way.
	 */
	addr += sc->sc_base + SH5PCI_IO_OFFSET;

	return (bus_space_map(sc->sc_bust, addr, size, flags, bushp));
}

/*ARGSUSED*/
static void
sh5pci_attach_hook(void *arg, struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

/*ARGSUSED*/
static int
sh5pci_maxdevs(void *arg, int busno)
{
	if (busno == 0)
		return (4);
	return (32);
}

/*ARGSUSED*/
static pcitag_t
sh5pci_make_tag(void *arg, int bus, int dev, int func)
{

	return ((pcitag_t)SH5PCI_CSR_PAR_MAKE(bus, dev, func, 0));
}

/*ARGSUSED*/
static void
sh5pci_decompose_tag(void *arg, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (int)SH5PCI_CSR_PAR_PCI_BN(tag);
	if (dp != NULL)
		*dp = (int)SH5PCI_CSR_PAR_PCI_DN(tag);
	if (fp != NULL)
		*fp = (int)SH5PCI_CSR_PAR_PCI_FN(tag);
}

static pcireg_t
sh5pci_conf_read(void *arg, pcitag_t tag, int reg)
{
	struct sh5pci_softc *sc = arg;
	pcireg_t rv;
	int s;

	KDASSERT((reg & 3) == 0);

	s = splhigh();
	sh5pci_csr_write(sc, SH5PCI_CSR_PAR, (u_int32_t)(tag | (pcitag_t)reg));
	rv = (pcireg_t)sh5pci_csr_read(sc, SH5PCI_CSR_PDR);
	splx(s);

	return (rv);
}

static void
sh5pci_conf_write(void *arg, pcitag_t tag, int reg, pcireg_t data)
{
	struct sh5pci_softc *sc = arg;
	int s;

	KDASSERT((reg & 3) == 0);

	s = splhigh();
	sh5pci_csr_write(sc, SH5PCI_CSR_PAR, (u_int32_t)(tag | (pcitag_t)reg));
	sh5pci_csr_write(sc, SH5PCI_CSR_PDR, (u_int32_t)data);
	splx(s);
}

static void
sh5pci_conf_interrupt(void *arg, int bus, int dev, int pin, int swiz, int *line)
{
	struct sh5pci_softc *sc = arg;

	(*sc->sc_intr->ih_intr_conf)(sc->sc_intr_arg, bus, dev, pin,
	    swiz, line);
}

static int
sh5pci_intr_map(void *arg, struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	struct sh5pci_softc *sc = arg;

	return ((*sc->sc_intr->ih_intr_map)(sc->sc_intr_arg, pa, ih));
}

static const char *
sh5pci_intr_string(void *arg, pci_intr_handle_t ih)
{
	struct sh5pci_softc *sc = arg;
	struct sh5pci_ihead *ihead;
	static char intstr[16];

	ihead = (*sc->sc_intr->ih_intr_ihead)(sc->sc_intr_arg, ih);
	if (ihead == NULL)
		return (NULL);

	if (ihead->ih_level)
		sprintf(intstr, "ipl %d", ihead->ih_level);
	else
		sprintf(intstr, "intevt 0x%x", ihead->ih_intevt);

	return (intstr);
}

static const struct evcnt *
sh5pci_intr_evcnt(void *arg, pci_intr_handle_t ih)
{
	struct sh5pci_softc *sc = arg;
	struct sh5pci_ihead *ihead;

	ihead = (*sc->sc_intr->ih_intr_ihead)(sc->sc_intr_arg, ih);
	if (ihead == NULL)
		return (NULL);

	return (ihead->ih_evcnt);
}

static void *
sh5pci_intr_establish(void *arg, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *fnarg)
{
	struct sh5pci_softc *sc = arg;
	struct sh5pci_ihead *ihead;
	struct sh5pci_icookie *ic;
	int s;

	ihead = (*sc->sc_intr->ih_intr_ihead)(sc->sc_intr_arg, ih);
	if (ihead == NULL)
		return (NULL);

	if ((ic = sh5_intr_alloc_handle(sizeof(*ic))) == NULL)
		return (NULL);

	s = splhigh();
	if (ihead->ih_cookie != NULL && ihead->ih_level != level) {
		splx(s);
		sh5_intr_free_handle(ic);
		printf("sh5pci_intr_establish: shared level mismatch\n");
		return (NULL);
	}

	ic->ic_ih = ih;
	ic->ic_func = func;
	ic->ic_arg = fnarg;
	SLIST_INSERT_HEAD(&ihead->ih_handlers, ic, ic_next);

	if (ihead->ih_cookie) {
		splx(s);
		return (ic);
	}

	/*
	 * First time through. Hook the real interrupt.
	 */
	ihead->ih_level = level;
	ihead->ih_cookie = (*sc->sc_intr->ih_intr_establish)(sc->sc_intr_arg,
	    ih, level, sh5pci_intr_dispatch, ihead);
	KDASSERT(ihead->ih_cookie);
	splx(s);
	return (ic);
}

static void
sh5pci_intr_disestablish(void *arg, void *cookie)
{
	struct sh5pci_softc *sc = arg;
	struct sh5pci_ihead *ihead;
	struct sh5pci_icookie *ic = cookie;
	int s;

	ihead = (*sc->sc_intr->ih_intr_ihead)(sc->sc_intr_arg, ic->ic_ih);
	if (ihead == NULL)
		return;		/* XXX: Panic instead? */

	KDASSERT(ihead->ih_cookie != NULL);

	s = splhigh();
	SLIST_REMOVE(&ihead->ih_handlers, ic, sh5pci_icookie, ic_next);

	/*
	 * If we're removing the last handler, unhook the interrupt.
	 */
	if (SLIST_EMPTY(&ihead->ih_handlers)) {
		(*sc->sc_intr->ih_intr_disestablish)(sc->sc_intr_arg,
		    ic->ic_ih, ihead->ih_cookie);
		/*
		 * Note that ihead is likely to be invalid now if the back-end
		 * does lazy-allocation of the sh5pci_ihead structures...
		 */
	}
	splx(s);

	sh5_intr_free_handle(ic);
}

static int
sh5pci_intr_dispatch(void *arg)
{
	struct sh5pci_ihead *ihead = arg;
	struct sh5pci_icookie *ic;
	int rv = 0;

	/*
	 * Call all the handlers registered for a particular interrupt pin
	 * and accumulate their "handled" status.
	 */
	SLIST_FOREACH(ic, &ihead->ih_handlers, ic_next) {
		if ((*ic->ic_func)(ic->ic_arg)) {
			if (ihead->ih_evcnt)
				ihead->ih_evcnt->ev_count++;
			rv++;
		}
	}

	return (rv);
}

static void
sh5pci_bridge_init(struct sh5pci_softc *sc)
{
	u_int32_t reg;
	int i;

	/* Disable the bridge */
	reg = sh5pci_csr_read(sc, SH5PCI_CSR_CR);
	reg &= ~SH5PCI_CSR_CR_PCI_CFINT_WR(1);
	reg |= SH5PCI_CSR_CR_PCI_CFINT_WR(0);
	sh5pci_csr_write(sc, SH5PCI_CSR_CR, reg);

	/*
	 * Disable snoop
	 */
	sh5pci_csr_write(sc, SH5PCI_CSR_CSCR0, SH5PCI_CSR_CSCR_SNPMD_DISABLED);
	sh5pci_csr_write(sc, SH5PCI_CSR_CSAR0, 0);
	sh5pci_csr_write(sc, SH5PCI_CSR_CSCR1, SH5PCI_CSR_CSCR_SNPMD_DISABLED);
	sh5pci_csr_write(sc, SH5PCI_CSR_CSAR1, 0);

	/*
	 * Disable general, arbiter, and power-management interrupts.
	 */
	sh5pci_csr_write(sc, SH5PCI_CSR_INTM, 0);
	sh5pci_csr_write(sc, SH5PCI_CSR_AINTM, 0);
	sh5pci_csr_write(sc, SH5PCI_CSR_PINTM, 0);

	/*
	 * Now enable the bridge.
	 */
	reg = sh5pci_csr_read(sc, SH5PCI_CSR_CR);
	reg |= SH5PCI_CSR_CR_PCI_FTO_WR(1);	/* TRDY and IRDY Enable */
	reg |= SH5PCI_CSR_CR_PCI_PFE_WR(1);	/* Pre-fetch Enable */
	reg |= SH5PCI_CSR_CR_PCI_BMAM_WR(1);	/* Round-robin arbitration */
	reg |= SH5PCI_CSR_CR_PCI_PFCS(1);	/* 32-bytes prefetching */
	sh5pci_csr_write(sc, SH5PCI_CSR_CR, reg);
	reg |= SH5PCI_CSR_CR_PCI_CFINT_WR(1);	/* Take bridge out of reset */
	sh5pci_csr_write(sc, SH5PCI_CSR_CR, reg);

	/*
	 * Enable Memory and I/O spaces, and enable the bridge to
	 * be a bus master.
	 */
	reg = sh5pci_csr_read(sc, PCI_COMMAND_STATUS_REG);
	reg &= ~(PCI_COMMAND_MASK << PCI_COMMAND_SHIFT);
	reg |= (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE |
		PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_STEPPING_ENABLE)
		<< PCI_COMMAND_SHIFT;
	sh5pci_csr_write(sc, PCI_COMMAND_STATUS_REG, reg);

	/*
	 * Specify the base addresses in PCI memory and I/O space to
	 * which SuperHyway accesses are mapped.
	 *
	 * For PCI memory space, we position this to the top 512MB
	 * of the PCIbus address space.
	 *
	 * For PCI i/o space, we position this at PCIbus address 0 to
	 * remain compatible with the PeeCee scheme of things.
	 */
	sh5pci_csr_write(sc, SH5PCI_CSR_MBMR,
	    SH5PCI_CSR_MBMR_PCI_MSBAMR(SH5PCI_MB2MSBAMR(512)));
	sh5pci_csr_write(sc, SH5PCI_CSR_MBR, (~SH5PCI_MEMORY_SIZE) + 1);

	sh5pci_csr_write(sc, SH5PCI_CSR_IOBMR,
	    SH5PCI_CSR_IOBMR_PCI_IOBAMR(SH5PCI_KB2IOBAMR(256)));
	sh5pci_csr_write(sc, SH5PCI_CSR_IOBR, 0);

	/*
	 * Set up the PCI target images such that other PCIbus Masters
	 * can access system memory.
	 *
	 * XXX: Really shouldn't hard-code these.
	 */
	sh5pci_csr_write(sc, SH5PCI_CSR_LSR(0),
	    SH5PCI_CSR_LSR_PCI_LSR_WR(SH5PCI_MB2LSR(512)) |
	    SH5PCI_CSR_LSR_PCI_MBARE);
	sh5pci_csr_write(sc, SH5PCI_CSR_LAR(0), SH5PCI_RAM_PHYS_BASE);
	sh5pci_csr_write(sc, SH5PCI_CONF_MBAR(0),
	    0x80000000 | PCI_MAPREG_TYPE_MEM |
	    PCI_MAPREG_MEM_TYPE_32BIT | PCI_MAPREG_MEM_PREFETCHABLE_MASK);

	sh5pci_csr_write(sc, SH5PCI_CSR_LSR(1),
	    SH5PCI_CSR_LSR_PCI_LSR_WR(SH5PCI_MB2LSR(512)) |
	    SH5PCI_CSR_LSR_PCI_MBARE);
	sh5pci_csr_write(sc, SH5PCI_CSR_LAR(1),
	    SH5PCI_RAM_PHYS_BASE + 0x20000000);
	sh5pci_csr_write(sc, SH5PCI_CONF_MBAR(1),
	    0xa0000000 | PCI_MAPREG_TYPE_MEM |
	    PCI_MAPREG_MEM_TYPE_32BIT | PCI_MAPREG_MEM_PREFETCHABLE_MASK);

	/*
	 * Fetch the mapping registers for the benefit of bus_dma mappings.
	 */
	for (i = 0; i < SH5PCI_NUM_MBARS; i++) {
		reg = sh5pci_csr_read(sc, SH5PCI_CSR_LSR(i));
		if (reg & SH5PCI_CSR_LSR_PCI_MBARE) {
			sc->sc_map[i].m_start =
			    sh5pci_csr_read(sc, SH5PCI_CSR_LAR(i));
			sc->sc_map[i].m_end = sc->sc_map[i].m_start +
			    SH5PCI_CSR_LSR_PCI_LSR_SIZE(reg);
			sc->sc_map[i].m_pcibase =
			    PCI_MAPREG_MEM_ADDR(sh5pci_csr_read(sc,
			    SH5PCI_CONF_MBAR(i)));
		} else
			sc->sc_map[i].m_start = ~0;
	}

	/*
	 * Enable PCI error/arbiter interrupts
	 */
	sh5pci_csr_write(sc, SH5PCI_CSR_INTM, ~0);
	sh5pci_csr_write(sc, SH5PCI_CSR_AINTM, ~0);
}

static int
sh5pci_check_master_abort(struct sh5pci_softc *sc)
{
	u_int32_t reg;
	int s, rv = 0;

	s = splhigh();

	reg = sh5pci_csr_read(sc, PCI_COMMAND_STATUS_REG);
	if ((reg & PCI_STATUS_MASTER_ABORT) != 0) {
		sh5pci_csr_write(sc, PCI_COMMAND_STATUS_REG, reg);
                sh5pci_csr_write(sc, SH5PCI_CSR_INT, SH5PCI_CSR_INT_PCI_MADIM);
		rv = 1;
	}

	splx(s);

	return (rv);
}

static int
sh5pci_interrupt(void *arg)
{
	struct sh5pci_softc *sc = arg;
	u_int32_t pci_int, pci_air, pci_cir;

	pci_int = sh5pci_csr_read(sc, SH5PCI_CSR_INT);
	pci_cir = sh5pci_csr_read(sc, SH5PCI_CSR_CIR);
	pci_air = sh5pci_csr_read(sc, SH5PCI_CSR_AIR);

	if (pci_int) {
		printf("%s: PCI IRQ: INT 0x%x, CIR 0x%x, AIR 0x%x\n",
		    sc->sc_dev.dv_xname, pci_int, pci_cir, pci_air);
		sh5pci_csr_write(sc, SH5PCI_CSR_INT, pci_int);
	}

	pci_int = sh5pci_csr_read(sc, SH5PCI_CSR_AINT);

	if (pci_int) {
		printf("%s: PCI Arbiter IRQ: AINT 0x%x, CIR 0x%x, AIR 0x%x\n",
		    sc->sc_dev.dv_xname, pci_int, pci_cir, pci_air);
		sh5pci_csr_write(sc, SH5PCI_CSR_AINT, pci_int);
	}

	return (1);
}
