/*	$NetBSD: sbus.c,v 1.62 2003/08/27 15:59:55 mrg Exp $ */

/*
 * Copyright (c) 1999-2002 Eduardo Horvath
 * All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Sbus stuff.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbus.c,v 1.62 2003/08/27 15:59:55 mrg Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/openfirm.h>

#include <sparc64/sparc64/cache.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/sparc64.h>

#ifdef DEBUG
#define SDB_DVMA	0x1
#define SDB_INTR	0x2
int sbus_debug = 0;
#define DPRINTF(l, s)   do { if (sbus_debug & l) printf s; } while (0)
#else
#define DPRINTF(l, s)
#endif

void sbusreset __P((int));

static bus_space_tag_t sbus_alloc_bustag __P((struct sbus_softc *));
static bus_dma_tag_t sbus_alloc_dmatag __P((struct sbus_softc *));
static int sbus_get_intr __P((struct sbus_softc *, int,
			      struct openprom_intr **, int *, int));
static int sbus_overtemp __P((void *));
static int _sbus_bus_map __P((
		bus_space_tag_t,
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,			/* XXX unused -- compat w/sparc */
		bus_space_handle_t *));
static void *sbus_intr_establish __P((
		bus_space_tag_t,
		int,			/*Sbus interrupt level*/
		int,			/*`device class' priority*/
		int (*) __P((void *)),	/*handler*/
		void *,			/*handler arg*/
		void (*) __P((void))));	/*optional fast trap*/


/* autoconfiguration driver */
int	sbus_match __P((struct device *, struct cfdata *, void *));
void	sbus_attach __P((struct device *, struct device *, void *));


CFATTACH_DECL(sbus, sizeof(struct sbus_softc),
    sbus_match, sbus_attach, NULL, NULL);

extern struct cfdriver sbus_cd;

/*
 * DVMA routines
 */
int sbus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
			  bus_size_t, struct proc *, int));
void sbus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
int sbus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
void sbus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
			   bus_size_t, int));
int sbus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
			   bus_size_t alignment, bus_size_t boundary,
			   bus_dma_segment_t *segs, int nsegs, int *rsegs,
			   int flags));
void sbus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
			   int nsegs));
int sbus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
			 int nsegs, size_t size, caddr_t *kvap, int flags));
void sbus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
			    size_t size));

/*
 * Child devices receive the Sbus interrupt level in their attach
 * arguments. We translate these to CPU IPLs using the following
 * tables. Note: obio bus interrupt levels are identical to the
 * processor IPL.
 *
 * The second set of tables is used when the Sbus interrupt level
 * cannot be had from the PROM as an `interrupt' property. We then
 * fall back on the `intr' property which contains the CPU IPL.
 */

/*
 * This value is or'ed into the attach args' interrupt level cookie
 * if the interrupt level comes from an `intr' property, i.e. it is
 * not an Sbus interrupt level.
 */
#define SBUS_INTR_COMPAT	0x80000000


/*
 * Print the location of some sbus-attached device (called just
 * before attaching that device).  If `sbus' is not NULL, the
 * device was found but not configured; print the sbus as well.
 * Return UNCONF (config_find ignores this if the device was configured).
 */
int
sbus_print(args, busname)
	void *args;
	const char *busname;
{
	struct sbus_attach_args *sa = args;
	int i;

	if (busname)
		aprint_normal("%s at %s", sa->sa_name, busname);
	aprint_normal(" slot %ld offset 0x%lx", (long)sa->sa_slot, 
	       (u_long)sa->sa_offset);
	for (i = 0; i < sa->sa_nintr; i++) {
		struct openprom_intr *sbi = &sa->sa_intr[i];

		aprint_normal(" vector %lx ipl %ld", 
		       (u_long)sbi->oi_vec, 
		       (long)INTLEV(sbi->oi_pri));
	}
	return (UNCONF);
}

int
sbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach an Sbus.
 */
void
sbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sbus_softc *sc = (struct sbus_softc *)self;
	struct mainbus_attach_args *ma = aux;
	struct intrhand *ih;
	int ipl;
	char *name;
	int node = ma->ma_node;
	int node0, error;
	bus_space_tag_t sbt;
	struct sbus_attach_args sa;

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;
	sc->sc_ign = ma->ma_interrupts[0] & INTMAP_IGN;		

	/* XXXX Use sysio PROM mappings for interrupt vector regs. */
	sparc_promaddr_to_handle(sc->sc_bustag,	ma->ma_address[0], &sc->sc_bh);
	sc->sc_sysio = (struct sysioreg *)bus_space_vaddr(sc->sc_bustag, 
		sc->sc_bh);

#ifdef _LP64
	/* 
	 * 32-bit kernels use virtual addresses for bus space operations
	 * so we may as well use the prom VA.
	 *
	 * 64-bit kernels use physical addresses for bus space operations
	 * so mapping this in again will reduce TLB thrashing.
	 */
	if (bus_space_map(sc->sc_bustag, ma->ma_reg[0].ur_paddr, 
		ma->ma_reg[0].ur_len, 0, &sc->sc_bh) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}
#endif

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = PROM_getpropint(node, "clock-frequency", 
		25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbt = sbus_alloc_bustag(sc);
	sc->sc_dmatag = sbus_alloc_dmatag(sc);

	/*
	 * Get the SBus burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = PROM_getpropint(node, "burst-sizes", 0);

	/*
	 * Collect address translations from the OBP.
	 */
	error = PROM_getprop(node, "ranges", sizeof(struct openprom_range),
			 &sc->sc_nrange, &sc->sc_range);
	if (error)
		panic("%s: error getting ranges property", sc->sc_dev.dv_xname);

	/* initialize the IOMMU */

	/* punch in our copies */
	sc->sc_is.is_bustag = sc->sc_bustag;
	bus_space_subregion(sc->sc_bustag, sc->sc_bh, 
		(vaddr_t)&((struct sysioreg *)NULL)->sys_iommu, 
		sizeof (struct iommureg), &sc->sc_is.is_iommu);
 
	/* initialize our strbuf_ctl */
	sc->sc_is.is_sb[0] = &sc->sc_sb;
	sc->sc_sb.sb_is = &sc->sc_is;
	bus_space_subregion(sc->sc_bustag, sc->sc_bh, 
		(vaddr_t)&((struct sysioreg *)NULL)->sys_strbuf, 
		sizeof (struct iommu_strbuf), &sc->sc_sb.sb_sb);
	/* Point sb_flush to our flush buffer. */
	sc->sc_sb.sb_flush = &sc->sc_flush;

	/* give us a nice name.. */
	name = (char *)malloc(32, M_DEVBUF, M_NOWAIT);
	if (name == 0)
		panic("couldn't malloc iommu name");
	snprintf(name, 32, "%s dvma", sc->sc_dev.dv_xname);

	iommu_init(name, &sc->sc_is, 0, -1);

	/* Enable the over temp intr */
	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	ih->ih_map = &sc->sc_sysio->therm_int_map;
	ih->ih_clr = NULL; /* &sc->sc_sysio->therm_clr_int; */
	ih->ih_fun = sbus_overtemp;
	ipl = 1;
	ih->ih_pil = (1<<ipl);
	ih->ih_number = INTVEC(*(ih->ih_map));
	intr_establish(ipl, ih);
	*(ih->ih_map) |= INTMAP_V;
	
	/*
	 * Note: the stupid SBUS IOMMU ignores the high bits of an address, so a
	 * NULL DMA pointer will be translated by the first page of the IOTSB.
	 * To avoid bugs we'll alloc and ignore the first entry in the IOTSB.
	 */
	{
		u_long dummy;

		if (extent_alloc_subregion(sc->sc_is.is_dvmamap,
		    sc->sc_is.is_dvmabase, sc->sc_is.is_dvmabase + PAGE_SIZE,
		    PAGE_SIZE, PAGE_SIZE, 0, EX_NOWAIT|EX_BOUNDZERO,
		    (u_long *)&dummy) != 0)
			panic("sbus iommu: can't toss first dvma page");
	}

	/*
	 * Loop through ROM children, fixing any relative addresses
	 * and then configuring each device.
	 * `specials' is an array of device names that are treated
	 * specially:
	 */
	node0 = OF_child(node);
	for (node = node0; node; node = OF_peer(node)) {
		char *name = PROM_getpropstring(node, "name");

		if (sbus_setup_attach_args(sc, sbt, sc->sc_dmatag,
					   node, &sa) != 0) {
			printf("sbus_attach: %s: incomplete\n", name);
			continue;
		}
		(void) config_found(&sc->sc_dev, (void *)&sa, sbus_print);
		sbus_destroy_attach_args(&sa);
	}
}

int
sbus_setup_attach_args(sc, bustag, dmatag, node, sa)
	struct sbus_softc	*sc;
	bus_space_tag_t		bustag;
	bus_dma_tag_t		dmatag;
	int			node;
	struct sbus_attach_args	*sa;
{
	/*struct	openprom_addr sbusreg;*/
	/*int	base;*/
	int	error;
	int n;

	bzero(sa, sizeof(struct sbus_attach_args));
	error = PROM_getprop(node, "name", 1, &n, &sa->sa_name);
	if (error != 0)
		return (error);
	sa->sa_name[n] = '\0';

	sa->sa_bustag = bustag;
	sa->sa_dmatag = dmatag;
	sa->sa_node = node;
	sa->sa_frequency = sc->sc_clockfreq;

	error = PROM_getprop(node, "reg", sizeof(struct openprom_addr),
			 &sa->sa_nreg, &sa->sa_reg);
	if (error != 0) {
		char buf[32];
		if (error != ENOENT ||
		    !node_has_property(node, "device_type") ||
		    strcmp(PROM_getpropstringA(node, "device_type", buf),
			   "hierarchical") != 0)
			return (error);
	}
	for (n = 0; n < sa->sa_nreg; n++) {
		/* Convert to relative addressing, if necessary */
		u_int32_t base = sa->sa_reg[n].oa_base;
		if (SBUS_ABS(base)) {
			sa->sa_reg[n].oa_space = SBUS_ABS_TO_SLOT(base);
			sa->sa_reg[n].oa_base = SBUS_ABS_TO_OFFSET(base);
		}
	}

	if ((error = sbus_get_intr(sc, node, &sa->sa_intr, &sa->sa_nintr,
	    sa->sa_slot)) != 0)
		return (error);

	error = PROM_getprop(node, "address", sizeof(u_int32_t),
			 &sa->sa_npromvaddrs, &sa->sa_promvaddrs);
	if (error != 0 && error != ENOENT)
		return (error);

	return (0);
}

void
sbus_destroy_attach_args(sa)
	struct sbus_attach_args	*sa;
{
	if (sa->sa_name != NULL)
		free(sa->sa_name, M_DEVBUF);

	if (sa->sa_nreg != 0)
		free(sa->sa_reg, M_DEVBUF);

	if (sa->sa_intr)
		free(sa->sa_intr, M_DEVBUF);

	if (sa->sa_promvaddrs)
		free((void *)sa->sa_promvaddrs, M_DEVBUF);

	bzero(sa, sizeof(struct sbus_attach_args)); /*DEBUG*/
}


int
_sbus_bus_map(t, addr, size, flags, v, hp)
	bus_space_tag_t t;
	bus_addr_t addr;
	bus_size_t size;
	int	flags;
	vaddr_t v;
	bus_space_handle_t *hp;
{
	struct sbus_softc *sc = t->cookie;
	int64_t slot = BUS_ADDR_IOSPACE(addr);
	int64_t offset = BUS_ADDR_PADDR(addr);
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr;

		if (sc->sc_range[i].or_child_space != slot)
			continue;

		/* We've found the connection to the parent bus */
		paddr = sc->sc_range[i].or_parent_base + offset;
		paddr |= ((bus_addr_t)sc->sc_range[i].or_parent_space<<32);
		DPRINTF(SDB_DVMA,
("\n_sbus_bus_map: mapping paddr slot %lx offset %lx poffset %lx paddr %lx\n",
		    (long)slot, (long)offset,
		    (long)sc->sc_range[i].or_parent_base,
		    (long)paddr));
		return (bus_space_map(sc->sc_bustag, paddr, size, flags, hp));
	}

	return (EINVAL);
}


bus_addr_t
sbus_bus_addr(t, btype, offset)
	bus_space_tag_t t;
	u_int btype;
	u_int offset;
{
	bus_addr_t baddr;
	int slot = btype;
	struct sbus_softc *sc = t->cookie;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		if (sc->sc_range[i].or_child_space != slot)
			continue;

		baddr = sc->sc_range[i].or_parent_base + offset;
		baddr |= ((bus_addr_t)sc->sc_range[i].or_parent_space<<32);
	}

	return (baddr);
}


/*
 * Each attached device calls sbus_establish after it initializes
 * its sbusdev portion.
 */
void
sbus_establish(sd, dev)
	register struct sbusdev *sd;
	register struct device *dev;
{
	register struct sbus_softc *sc;
	register struct device *curdev;

	/*
	 * We have to look for the sbus by name, since it is not necessarily
	 * our immediate parent (i.e. sun4m /iommu/sbus/espdma/esp)
	 * We don't just use the device structure of the above-attached
	 * sbus, since we might (in the future) support multiple sbus's.
	 */
	for (curdev = dev->dv_parent; ; curdev = curdev->dv_parent) {
		if (!curdev || !curdev->dv_xname)
			panic("sbus_establish: can't find sbus parent for %s",
			      sd->sd_dev->dv_xname
					? sd->sd_dev->dv_xname
					: "<unknown>" );

		if (strncmp(curdev->dv_xname, "sbus", 4) == 0)
			break;
	}
	sc = (struct sbus_softc *) curdev;

	sd->sd_dev = dev;
	sd->sd_bchain = sc->sc_sbdev;
	sc->sc_sbdev = sd;
}

/*
 * Reset the given sbus.
 */
void
sbusreset(sbus)
	int sbus;
{
	register struct sbusdev *sd;
	struct sbus_softc *sc = sbus_cd.cd_devs[sbus];
	struct device *dev;

	printf("reset %s:", sc->sc_dev.dv_xname);
	for (sd = sc->sc_sbdev; sd != NULL; sd = sd->sd_bchain) {
		if (sd->sd_reset) {
			dev = sd->sd_dev;
			(*sd->sd_reset)(dev);
			printf(" %s", dev->dv_xname);
		}
	}
	/* Reload iommu regs */
	iommu_reset(&sc->sc_is);
}

/*
 * Handle an overtemp situation.
 *
 * SPARCs have temperature sensors which generate interrupts
 * if the machine's temperature exceeds a certain threshold.
 * This handles the interrupt and powers off the machine.
 * The same needs to be done to PCI controller drivers.
 */
int
sbus_overtemp(arg)
	void *arg;
{
	/* Should try a clean shutdown first */
	printf("DANGER: OVER TEMPERATURE detected\nShutting down...\n");
	delay(20);
	cpu_reboot(RB_POWERDOWN|RB_HALT, NULL);
}

/*
 * Get interrupt attributes for an Sbus device.
 */
int
sbus_get_intr(sc, node, ipp, np, slot)
	struct sbus_softc *sc;
	int node;
	struct openprom_intr **ipp;
	int *np;
	int slot;
{
	int *ipl;
	int n, i;
	char buf[32];

	/*
	 * The `interrupts' property contains the Sbus interrupt level.
	 */
	ipl = NULL;
	if (PROM_getprop(node, "interrupts", sizeof(int), np, &ipl) == 0) {
		struct openprom_intr *ip;
		int pri;

		/* Default to interrupt level 2 -- otherwise unused */
		pri = INTLEVENCODE(2);

		/* Change format to an `struct sbus_intr' array */
		ip = malloc(*np * sizeof(struct openprom_intr), M_DEVBUF,
		    M_NOWAIT);
		if (ip == NULL)
			return (ENOMEM);

		/*
		 * Now things get ugly.  We need to take this value which is
		 * the interrupt vector number and encode the IPL into it
		 * somehow. Luckily, the interrupt vector has lots of free
		 * space and we can easily stuff the IPL in there for a while.
		 */
		PROM_getpropstringA(node, "device_type", buf);
		if (!buf[0])
			PROM_getpropstringA(node, "name", buf);

		for (i = 0; intrmap[i].in_class; i++) 
			if (strcmp(intrmap[i].in_class, buf) == 0) {
				pri = INTLEVENCODE(intrmap[i].in_lev);
				break;
			}

		/*
		 * Sbus card devices need the slot number encoded into
		 * the vector as this is generally not done.
		 */
		if ((ipl[0] & INTMAP_OBIO) == 0)
			pri |= slot << 3;

		for (n = 0; n < *np; n++) {
			/* 
			 * We encode vector and priority into sbi_pri so we 
			 * can pass them as a unit.  This will go away if 
			 * sbus_establish ever takes an sbus_intr instead 
			 * of an integer level.
			 * Stuff the real vector in sbi_vec.
			 */

			ip[n].oi_pri = pri|ipl[n];
			ip[n].oi_vec = ipl[n];
		}
		free(ipl, M_DEVBUF);
		*ipp = ip;
	}
	
	return (0);
}


/*
 * Install an interrupt handler for an Sbus device.
 */
void *
sbus_intr_establish(t, pri, level, handler, arg, fastvec)
	bus_space_tag_t t;
	int pri;
	int level;
	int (*handler) __P((void *));
	void *arg;
	void (*fastvec) __P((void));	/* ignored */
{
	struct sbus_softc *sc = t->cookie;
	struct intrhand *ih;
	int ipl;
	long vec = pri; 

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	if ((vec & SBUS_INTR_COMPAT) != 0)
		ipl = vec & ~SBUS_INTR_COMPAT;
	else {
		/* Decode and remove IPL */
		ipl = INTLEV(vec);
		vec = INTVEC(vec);
		DPRINTF(SDB_INTR,
		    ("\nsbus: intr[%ld]%lx: %lx\nHunting for IRQ...\n",
		    (long)ipl, (long)vec, (u_long)intrlev[vec]));
		if ((vec & INTMAP_OBIO) == 0) {
			/* We're in an SBUS slot */
			/* Register the map and clear intr registers */

			int slot = INTSLOT(pri);

			ih->ih_map = &(&sc->sc_sysio->sbus_slot0_int)[slot];
			ih->ih_clr = &sc->sc_sysio->sbus0_clr_int[vec];
#ifdef DEBUG
			if (sbus_debug & SDB_INTR) {
				int64_t intrmap = *ih->ih_map;
				
				printf("SBUS %lx IRQ as %llx in slot %d\n", 
				       (long)vec, (long long)intrmap, slot);
				printf("\tmap addr %p clr addr %p\n",
				    ih->ih_map, ih->ih_clr);
			}
#endif
			/* Enable the interrupt */
			vec |= INTMAP_V;
			/* Insert IGN */
			vec |= sc->sc_ign;
			/* XXXX */
			*(ih->ih_map) = vec;
		} else {
			int64_t *intrptr = &sc->sc_sysio->scsi_int_map;
			int64_t intrmap = 0;
			int i;

			/* Insert IGN */
			vec |= sc->sc_ign;
			for (i = 0; &intrptr[i] <=
			    (int64_t *)&sc->sc_sysio->reserved_int_map &&
			    INTVEC(intrmap = intrptr[i]) != INTVEC(vec); i++)
				;
			if (INTVEC(intrmap) == INTVEC(vec)) {
				DPRINTF(SDB_INTR,
				    ("OBIO %lx IRQ as %lx in slot %d\n", 
				    vec, (long)intrmap, i));
				/* Register the map and clear intr registers */
				ih->ih_map = &intrptr[i];
				intrptr = (int64_t *)&sc->sc_sysio->scsi_clr_int;
				ih->ih_clr = &intrptr[i];
				/* Enable the interrupt */
				intrmap |= INTMAP_V;
				/* XXXX */
				*(ih->ih_map) = intrmap;
			} else
				panic("IRQ not found!");
		}
	}
#ifdef DEBUG
	if (sbus_debug & SDB_INTR) { long i; for (i = 0; i < 400000000; i++); }
#endif

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	ih->ih_number = vec;
	ih->ih_pil = (1<<ipl);
	intr_establish(ipl, ih);
	return (ih);
}

static bus_space_tag_t
sbus_alloc_bustag(sc)
	struct sbus_softc *sc;
{
	bus_space_tag_t sbt;

	sbt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (sbt == NULL)
		return (NULL);

	bzero(sbt, sizeof *sbt);
	sbt->cookie = sc;
	sbt->parent = sc->sc_bustag;
	sbt->type = SBUS_BUS_SPACE;
	sbt->sparc_bus_map = _sbus_bus_map;
	sbt->sparc_bus_mmap = sc->sc_bustag->sparc_bus_mmap;
	sbt->sparc_intr_establish = sbus_intr_establish;
	return (sbt);
}


static bus_dma_tag_t
sbus_alloc_dmatag(sc)
	struct sbus_softc *sc;
{
	bus_dma_tag_t sdt, psdt = sc->sc_dmatag;

	sdt = (bus_dma_tag_t)
		malloc(sizeof(struct sparc_bus_dma_tag), M_DEVBUF, M_NOWAIT);
	if (sdt == NULL)
		/* Panic? */
		return (psdt);

	sdt->_cookie = sc;
	sdt->_parent = psdt;
#define PCOPY(x)	sdt->x = psdt->x
	PCOPY(_dmamap_create);
	PCOPY(_dmamap_destroy);
	sdt->_dmamap_load = sbus_dmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	sdt->_dmamap_load_raw = sbus_dmamap_load_raw;
	sdt->_dmamap_unload = sbus_dmamap_unload;
	sdt->_dmamap_sync = sbus_dmamap_sync;
	sdt->_dmamem_alloc = sbus_dmamem_alloc;
	sdt->_dmamem_free = sbus_dmamem_free;
	sdt->_dmamem_map = sbus_dmamem_map;
	sdt->_dmamem_unmap = sbus_dmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	sc->sc_dmatag = sdt;
	return (sdt);
}

int
sbus_dmamap_load(tag, map, buf, buflen, p, flags)
	bus_dma_tag_t tag;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	return (iommu_dvmamap_load(tag, &sc->sc_sb, map, buf, buflen, p, flags));
}

int
sbus_dmamap_load_raw(tag, map, segs, nsegs, size, flags)
	bus_dma_tag_t tag;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	return (iommu_dvmamap_load_raw(tag, &sc->sc_sb, map, segs, nsegs, flags, size));
}

void
sbus_dmamap_unload(tag, map)
	bus_dma_tag_t tag;
	bus_dmamap_t map;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamap_unload(tag, &sc->sc_sb, map);
}

void
sbus_dmamap_sync(tag, map, offset, len, ops)
	bus_dma_tag_t tag;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	if (ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) {
		/* Flush the CPU then the IOMMU */
		bus_dmamap_sync(tag->_parent, map, offset, len, ops);
		iommu_dvmamap_sync(tag, &sc->sc_sb, map, offset, len, ops);
	}
	if (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) {
		/* Flush the IOMMU then the CPU */
		iommu_dvmamap_sync(tag, &sc->sc_sb, map, offset, len, ops);
		bus_dmamap_sync(tag->_parent, map, offset, len, ops);
	}
}

int
sbus_dmamem_alloc(tag, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t tag;
	bus_size_t size;
	bus_size_t alignment;
	bus_size_t boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	return (iommu_dvmamem_alloc(tag, &sc->sc_sb, size, alignment, boundary,
	    segs, nsegs, rsegs, flags));
}

void
sbus_dmamem_free(tag, segs, nsegs)
	bus_dma_tag_t tag;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamem_free(tag, &sc->sc_sb, segs, nsegs);
}

int
sbus_dmamem_map(tag, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t tag;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	return (iommu_dvmamem_map(tag, &sc->sc_sb, segs, nsegs, size, kvap, flags));
}

void
sbus_dmamem_unmap(tag, kva, size)
	bus_dma_tag_t tag;
	caddr_t kva;
	size_t size;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamem_unmap(tag, &sc->sc_sb, kva, size);
}
