/*	$NetBSD: sbus.c,v 1.49.2.1 2002/03/26 17:20:11 eeh Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sbus.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1999 Eduardo Horvath
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


/*
 * Sbus stuff.
 */
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/properties.h>

#include <machine/bus.h>
#include <sparc64/sparc64/cache.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>

#include <uvm/uvm_prot.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/sparc64.h>
#include <machine/openfirm.h>

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
			      struct sbus_intr **, int *, int));
static int sbus_overtemp __P((void *));
static int _sbus_bus_map __P((
		bus_space_tag_t,
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,			/* XXX unused -- compat w/sparc */
		bus_space_handle_t *));
static void *_sbus_intr_establish __P((
		bus_space_tag_t,
		int,			/*Sbus interrupt level*/
		int,			/*`device class' priority*/
		int,			/*flags*/
		int (*) __P((void *)),	/*handler*/
		void *));		/*handler arg*/


/* autoconfiguration driver */
int	sbus_match __P((struct device *, struct cfdata *, void *));
void	sbus_attach __P((struct device *, struct device *, void *));


struct cfattach sbus_ca = {
	sizeof(struct sbus_softc), sbus_match, sbus_attach
};

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
		printf("%s at %s", sa->sa_name, busname);
	printf(" slot %ld offset 0x%lx", (long)sa->sa_slot, 
	       (u_long)sa->sa_offset);
	for (i = 0; i < sa->sa_nintr; i++) {
		struct sbus_intr *sbi = &sa->sa_intr[i];

		printf(" vector %lx ipl %ld", 
		       (u_long)sbi->sbi_vec, 
		       (long)INTLEV(sbi->sbi_pri));
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

	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
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
	size_t size;
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
	if (dev_getprop(self, "clock-frequency", &sc->sc_clockfreq,
		sizeof (sc->sc_clockfreq), NULL, 0) != 
		sizeof (sc->sc_clockfreq))
		sc->sc_clockfreq = 25*1000*1000;
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbt = sbus_alloc_bustag(sc);
	dev_setprop(self, "bustag", &sbt, sizeof(sbt), PROP_INT, 0);
	sc->sc_dmatag = sbus_alloc_dmatag(sc);
	dev_setprop(self, "dmatag", &sc->sc_dmatag, sizeof(sc->sc_dmatag), 
		PROP_INT, 0);

	/*
	 * Get the SBus burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = 0;
	dev_getprop(self, "burst-sizes", &sc->sc_burst,	sizeof (sc->sc_burst), 
		NULL, 0);

	/*
	 * Collect address translations from the OBP.
	 */
	size = dev_getprop(self, "ranges", NULL, 0, 0, 0);
#ifdef DIAGNOSTIC
	if (size == -1 || size % sizeof (struct sbus_range) != 0)
		panic("sbus_attach: bogus \"ranges\" size %lx", size);
#endif
	sc->sc_nrange = size / sizeof (struct sbus_range);
	sc->sc_range = malloc(size, M_DEVBUF, M_NOWAIT);
	error = dev_getprop(self, "ranges", sc->sc_range, size, NULL, 0);
	if (error != size)
		panic("%s: error getting ranges property", sc->sc_dev.dv_xname);

	/* initialize the IOMMU */

	/* punch in our copies */
	sc->sc_is.is_bustag = sc->sc_bustag;
	bus_space_subregion(sc->sc_bustag, sc->sc_bh, 
		(vaddr_t)&((struct sysioreg *)NULL)->sys_iommu, 
		sizeof (struct iommureg), &sc->sc_is.is_iommu);
	bus_space_subregion(sc->sc_bustag, sc->sc_bh, 
		(vaddr_t)&((struct sysioreg *)NULL)->sys_strbuf, 
		sizeof (struct iommu_strbuf), &sc->sc_is.is_sb[0]);
	sc->sc_is.is_sbvalid[0] = 1;
	sc->sc_is.is_sbvalid[1] = 0;

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
		    sc->sc_is.is_dvmabase, sc->sc_is.is_dvmabase + NBPG, NBPG,
		    NBPG, 0, EX_NOWAIT|EX_BOUNDZERO, (u_long *)&dummy) != 0)
			panic("sbus iommu: can't toss first dvma page");
	}

	/*
	 * Loop through ROM children, fixing any relative addresses
	 * and then configuring each device.
	 * `specials' is an array of device names that are treated
	 * specially:
	 */
	dev_setprop(self, "bus-tag", &sbt, sizeof(sbt), PROP_INT, 0);
	dev_setprop(self, "dma-tag", &sc->sc_dmatag, sizeof(sc->sc_dmatag), 
		PROP_INT, 0);

	node0 = OF_child(node);
	for (node = node0; node; node = OF_peer(node)) {
		struct device *child;

		if ((child = sbus_setup_attach_args(self, sbt, sc->sc_dmatag,
						    node, &sa)) == 0) {
			char name[32];

			OF_getprop(node, "name", name, sizeof(name));
			name[sizeof(name) -1] = 0;
			printf("sbus_attach: %s: incomplete\n", name);
			continue;
		}
		(void) config_found_sad(self, (void *)&sa, sbus_print, 
					NULL, child);
		sbus_destroy_attach_args(&sa);
	}
}

struct device *
sbus_setup_attach_args(parent, bustag, dmatag, node, sa)
	struct device		*parent;
	bus_space_tag_t		bustag;
	bus_dma_tag_t		dmatag;
	int			node;
	struct sbus_attach_args	*sa;
{
	struct sbus_softc	*sc = DEV_PRIVATE(parent);
	struct device *dev;
	int	error;
	int n;

	dev = dev_config_create(parent, 1);
	if (!dev) 
		return (NULL);
	bzero(sa, sizeof(struct sbus_attach_args));

	/* Get the "name" property. */
	error = PROM_getprop(node, "name", 1, &n, (void **)&sa->sa_name);
	if (error != 0)
		return (NULL);
	sa->sa_name[n] = '\0';

	sa->sa_bustag = bustag;
	sa->sa_dmatag = dmatag;
	sa->sa_node = node;
	/* Node must be registered since we need that to query the PROM */
	dev_setprop(dev, "node", &node, sizeof(node), PROP_INT, 0);
	sa->sa_frequency = sc->sc_clockfreq;

	/* Get the "reg" property */
	error = PROM_getprop(node, "reg", sizeof(struct sbus_reg),
			 &sa->sa_nreg, (void **)&sa->sa_reg);
	if (error != 0) {
		char buf[32];
		if (error != ENOENT ||
		    !node_has_property(node, "device_type") ||
		    strcmp(PROM_getpropstringA(node, "device_type", buf),
			   "hierarchical") != 0) {
			config_detach(dev, 0);
			return (NULL);
		}
	}
	for (n = 0; n < sa->sa_nreg; n++) {
		/* Convert to relative addressing, if necessary */
		u_int32_t base = sa->sa_reg[n].sbr_offset;
		if (SBUS_ABS(base)) {
			sa->sa_reg[n].sbr_slot = SBUS_ABS_TO_SLOT(base);
			sa->sa_reg[n].sbr_offset = SBUS_ABS_TO_OFFSET(base);
		}
	}
	/* Whole `reg' for those w/multiple registers */
#if 0
	dev_setprop(dev, "reg", (void *)sa->sa_reg, 
		    sa->sa_nreg * sizeof(struct sbus_reg), PROP_AGGREGATE, 0);
#endif

	dev_setprop(dev, "slot", (void *)&sa->sa_reg[0].sbr_slot, 
		    sizeof(sa->sa_reg[0].sbr_slot), PROP_INT, 0);
	dev_setprop(dev, "offset", (void *)&sa->sa_reg[0].sbr_offset, 
		    sizeof(sa->sa_reg[0].sbr_offset), PROP_INT, 0);
	dev_setprop(dev, "size", (void *)&sa->sa_reg[0].sbr_size, 
		    sizeof(sa->sa_reg[0].sbr_size), PROP_INT, 0);

	if ((error = sbus_get_intr(sc, node, &sa->sa_intr, &sa->sa_nintr,
				   sa->sa_slot)) != 0) {
		config_detach(dev, 0);
		return (NULL);
	}
	if (sa->sa_nintr > 0)
		dev_setprop(dev, "pri", (void *)&sa->sa_pri,
		    sizeof(&sa->sa_pri), PROP_INT, 0);

	error = PROM_getprop(node, "address", sizeof(u_int32_t),
		&sa->sa_npromvaddrs, (void **)&sa->sa_promvaddrs);
#if 0
	if (sa->sa_npromvaddrs)
		dev_setprop(dev, "address", (void *)sa->sa_promvaddrs,
			sa->sa_npromvaddrs * sizeof(u_int32_t), PROP_INT, 0);
#endif
	if (error != 0 && error != ENOENT) {
		config_detach(dev, 0);
		return (NULL);
	}

	return (dev);
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

		if (sc->sc_range[i].cspace != slot)
			continue;

		/* We've found the connection to the parent bus */
		paddr = sc->sc_range[i].poffset + offset;
		paddr |= ((bus_addr_t)sc->sc_range[i].pspace<<32);
		DPRINTF(SDB_DVMA,
("\n_sbus_bus_map: mapping paddr slot %lx offset %lx poffset %lx paddr %lx\n",
		    (long)slot, (long)offset, (long)sc->sc_range[i].poffset,
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
		if (sc->sc_range[i].cspace != slot)
			continue;

		baddr = sc->sc_range[i].poffset + offset;
		baddr |= ((bus_addr_t)sc->sc_range[i].pspace<<32);
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
	struct sbus_intr **ipp;
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
	if (PROM_getprop(node, "interrupts", sizeof(int), np, (void **)&ipl) == 
		0) {
		struct sbus_intr *ip;
		int pri;

		/* Default to interrupt level 2 -- otherwise unused */
		pri = INTLEVENCODE(2);

		/* Change format to an `struct sbus_intr' array */
		ip = malloc(*np * sizeof(struct sbus_intr), M_DEVBUF, M_NOWAIT);
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

			ip[n].sbi_pri = pri|ipl[n];
			ip[n].sbi_vec = ipl[n];
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
sbus_intr_establish(self, pri, index, handler, arg)
	struct device *self;
	int pri;	/* S/W priority */
	int index;	/* Index into "interrupts" property */
	int (*handler) __P((void *));
	void *arg;
{
	struct sbus_softc *sc = (struct sbus_softc *)
		DEV_PRIVATE(self->dv_parent);
	struct intrhand *ih;
	long vec;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	if (index != -1) {
		int interrupts[8];
		int err;

#ifdef DIAGNOSTIC
		if (index > 7) panic("sbus_intr_establish: index > 7");
#endif
		err = dev_getprop(self, "interrupts", &interrupts, 
			sizeof(interrupts), NULL, 0);
		if ((err == -1) || (index > (err / sizeof (int)))) {
			panic("sbus_intr_establish: cannot get interrupts: %x",
				err);
		}

		/* Decode and remove IPL */
		vec = interrupts[index];
		DPRINTF(SDB_INTR,
		    ("\nsbus: intr[%ld]%lx: %lx\nHunting for IRQ...\n",
		    (long)pri, (long)vec, (u_long)intrlev[vec]));
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
				intrptr = (int64_t *)
					&sc->sc_sysio->scsi_clr_int;
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
	ih->ih_pil = (1<<pri);
	intr_establish(pri, ih);
	return (ih);
}

/*
 * Install an interrupt handler for an Sbus device.
 */
void *
_sbus_intr_establish(t, pri, level, flags, handler, arg)
	bus_space_tag_t t;
	int pri;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct sbus_softc *sc = t->cookie;
	struct intrhand *ih;
	int ipl;
	long vec = pri; 

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	if ((flags & BUS_INTR_ESTABLISH_SOFTINTR) != 0)
		ipl = vec;
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
	sbt->sparc_intr_establish = _sbus_intr_establish;
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

	return (iommu_dvmamap_load(tag, &sc->sc_is, map, buf, buflen, p, flags));
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

	return (iommu_dvmamap_load_raw(tag, &sc->sc_is, map, segs, nsegs, flags, size));
}

void
sbus_dmamap_unload(tag, map)
	bus_dma_tag_t tag;
	bus_dmamap_t map;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamap_unload(tag, &sc->sc_is, map);
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
		iommu_dvmamap_sync(tag, &sc->sc_is, map, offset, len, ops);
	}
	if (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) {
		/* Flush the IOMMU then the CPU */
		iommu_dvmamap_sync(tag, &sc->sc_is, map, offset, len, ops);
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

	return (iommu_dvmamem_alloc(tag, &sc->sc_is, size, alignment, boundary,
	    segs, nsegs, rsegs, flags));
}

void
sbus_dmamem_free(tag, segs, nsegs)
	bus_dma_tag_t tag;
	bus_dma_segment_t *segs;
	int nsegs;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamem_free(tag, &sc->sc_is, segs, nsegs);
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

	return (iommu_dvmamem_map(tag, &sc->sc_is, segs, nsegs, size, kvap, flags));
}

void
sbus_dmamem_unmap(tag, kva, size)
	bus_dma_tag_t tag;
	caddr_t kva;
	size_t size;
{
	struct sbus_softc *sc = (struct sbus_softc *)tag->_cookie;

	iommu_dvmamem_unmap(tag, &sc->sc_is, kva, size);
}

