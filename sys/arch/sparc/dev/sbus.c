/*	$NetBSD: sbus.c,v 1.37 2000/06/29 07:40:08 mrg Exp $ */

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
 * Sbus stuff.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <sparc/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>
#include <dev/sbus/xboxvar.h>

#include <sparc/sparc/iommuvar.h>
#include <machine/autoconf.h>


void sbusreset __P((int));

static bus_space_tag_t sbus_alloc_bustag __P((struct sbus_softc *));
static int sbus_get_intr __P((struct sbus_softc *, int,
			      struct sbus_intr **, int *));
static int sbus_bus_mmap __P((bus_space_tag_t, bus_type_t, bus_addr_t,
			      int, bus_space_handle_t *));
static int _sbus_bus_map __P((
		bus_space_tag_t,
		bus_type_t,		/*slot*/
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,		/*preferred virtual address */
		bus_space_handle_t *));
static void *sbus_intr_establish __P((
		bus_space_tag_t,
		int,			/*level*/
		int,			/*flags*/
		int (*) __P((void *)),	/*handler*/
		void *));		/*handler arg*/


/* autoconfiguration driver */
int	sbus_match_mainbus __P((struct device *, struct cfdata *, void *));
int	sbus_match_iommu __P((struct device *, struct cfdata *, void *));
int	sbus_match_xbox __P((struct device *, struct cfdata *, void *));
void	sbus_attach_mainbus __P((struct device *, struct device *, void *));
void	sbus_attach_iommu __P((struct device *, struct device *, void *));
void	sbus_attach_xbox __P((struct device *, struct device *, void *));

static	int sbus_error __P((void));
int	(*sbuserr_handler) __P((void));

struct cfattach sbus_mainbus_ca = {
	sizeof(struct sbus_softc), sbus_match_mainbus, sbus_attach_mainbus
};
struct cfattach sbus_iommu_ca = {
	sizeof(struct sbus_softc), sbus_match_iommu, sbus_attach_iommu
};
struct cfattach sbus_xbox_ca = {
	sizeof(struct sbus_softc), sbus_match_xbox, sbus_attach_xbox
};

extern struct cfdriver sbus_cd;

/* The "primary" Sbus */
struct sbus_softc *sbus_sc;

/* If the PROM does not provide the `ranges' property, we make up our own */
struct sbus_range sbus_translations[] = {
	/* Assume a maximum of 4 Sbus slots, all mapped to on-board io space */
	{ 0, 0, PMAP_OBIO, SBUS_ADDR(0,0), 1 << 25 },
	{ 1, 0, PMAP_OBIO, SBUS_ADDR(1,0), 1 << 25 },
	{ 2, 0, PMAP_OBIO, SBUS_ADDR(2,0), 1 << 25 },
	{ 3, 0, PMAP_OBIO, SBUS_ADDR(3,0), 1 << 25 }
};

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

/* Translate Sbus interrupt level to processor IPL */
static int intr_sbus2ipl_4c[] = {
	0, 1, 2, 3, 5, 7, 8, 9
};
static int intr_sbus2ipl_4m[] = {
	0, 2, 3, 5, 7, 9, 11, 13
};

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
		printf("%s at %s", sa->sa_name, busname);
	printf(" slot %d offset 0x%x", sa->sa_slot, sa->sa_offset);
	for (i = 0; i < sa->sa_nintr; i++) {
		u_int32_t level = sa->sa_intr[i].sbi_pri;
		struct sbus_softc *sc =
			(struct sbus_softc *) sa->sa_bustag->cookie;

		printf(" level %d", level & ~SBUS_INTR_COMPAT);
		if ((level & SBUS_INTR_COMPAT) == 0) {
			int ipl = sc->sc_intr2ipl[level];
			if (ipl != level)
				printf(" (ipl %d)", ipl);
		}
	}
	return (UNCONF);
}

int
sbus_match_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
}

int
sbus_match_iommu(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iommu_attach_args *ia = aux;

	if (CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_driver->cd_name, ia->iom_name) == 0);
}

int
sbus_match_xbox(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct xbox_attach_args *xa = aux;

	if (CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_driver->cd_name, xa->xa_name) == 0);
}

/*
 * Attach an Sbus.
 */
void
sbus_attach_mainbus(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sbus_softc *sc = (struct sbus_softc *)self;
	struct mainbus_attach_args *ma = aux;
	int node = ma->ma_node;

	/*
	 * XXX there is only one Sbus, for now -- do not know how to
	 * address children on others
	 */
	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

#if 0	/* sbus at mainbus (sun4c): `reg' prop is not control space */
	if (ma->ma_size == 0)
		printf("%s: no Sbus registers", self->dv_xname);

	if (bus_space_map2(ma->ma_bustag,
			  (bus_type_t)ma->ma_iospace,
			  (bus_addr_t)ma->ma_paddr,
			  (bus_size_t)ma->ma_size,
			  BUS_SPACE_MAP_LINEAR,
			  0, &sc->sc_bh) != 0) {
		panic("%s: can't map sbusbusreg", self->dv_xname);
	}
#endif

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C
				? intr_sbus2ipl_4c
				: intr_sbus2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_sc = sc;
	sbus_attach_common(sc, "sbus", node, NULL);
}


void
sbus_attach_iommu(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sbus_softc *sc = (struct sbus_softc *)self;
	struct iommu_attach_args *ia = aux;
	int node = ia->iom_node;

	sc->sc_bustag = ia->iom_bustag;
	sc->sc_dmatag = ia->iom_dmatag;

	if (ia->iom_nreg == 0)
		panic("%s: no Sbus registers", self->dv_xname);

	if (bus_space_map2(ia->iom_bustag,
			  (bus_type_t)ia->iom_reg[0].ior_iospace,
			  (bus_addr_t)ia->iom_reg[0].ior_pa,
			  (bus_size_t)ia->iom_reg[0].ior_size,
			  BUS_SPACE_MAP_LINEAR,
			  0, &sc->sc_bh) != 0) {
		panic("%s: can't map sbusbusreg", self->dv_xname);
	}

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C ? intr_sbus2ipl_4c : intr_sbus2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_sc = sc;
	sbuserr_handler = sbus_error;
	sbus_attach_common(sc, "sbus", node, NULL);
}

void
sbus_attach_xbox(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sbus_softc *sc = (struct sbus_softc *)self;
	struct xbox_attach_args *xa = aux;
	int node = xa->xa_node;

	sc->sc_bustag = xa->xa_bustag;
	sc->sc_dmatag = xa->xa_dmatag;

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C ? intr_sbus2ipl_4c : intr_sbus2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_attach_common(sc, "sbus", node, NULL);
}

void
sbus_attach_common(sc, busname, busnode, specials)
	struct sbus_softc *sc;
	char *busname;
	int busnode;
	const char * const *specials;
{
	int node0, node, error;
	const char *sp;
	const char *const *ssp;
	bus_space_tag_t sbt;
	struct sbus_attach_args sa;

	sbt = sbus_alloc_bustag(sc);

	/*
	 * Get the SBus burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = getpropint(busnode, "burst-sizes", 0);


	if (CPU_ISSUN4M) {
		/*
		 * Some models (e.g. SS20) erroneously report 64-bit
		 * burst capability. We mask it out here for all SUN4Ms,
		 * since probably no member of that class supports
		 * 64-bit Sbus bursts.
		 */
		sc->sc_burst &= ~SBUS_BURST_64;
	}

	/*
	 * Collect address translations from the OBP.
	 */
	error = getprop(busnode, "ranges", sizeof(struct rom_range),
			&sc->sc_nrange, (void **)&sc->sc_range);
	switch (error) {
	case 0:
		break;
	case ENOENT:
		/* Fall back to our own `range' construction */
		sc->sc_range = sbus_translations;
		sc->sc_nrange =
			sizeof(sbus_translations)/sizeof(sbus_translations[0]);
		break;
	default:
		panic("%s: error getting ranges property", sc->sc_dev.dv_xname);
	}

	/*
	 * Loop through ROM children, fixing any relative addresses
	 * and then configuring each device.
	 * `specials' is an array of device names that are treated
	 * specially:
	 */
	node0 = firstchild(busnode);
	for (ssp = specials ; ssp != NULL && *(sp = *ssp) != 0; ssp++) {
		if ((node = findnode(node0, sp)) == 0) {
			panic("could not find %s amongst %s devices",
				sp, busname);
		}

		if (sbus_setup_attach_args(sc, sbt, sc->sc_dmatag,
					   node, &sa) != 0) {
			panic("sbus_attach: %s: incomplete", sp);
		}
		(void) config_found(&sc->sc_dev, (void *)&sa, sbus_print);
		sbus_destroy_attach_args(&sa);
	}

	for (node = node0; node; node = nextsibling(node)) {
		char *name = getpropstring(node, "name");
		for (ssp = specials, sp = NULL;
		     ssp != NULL && (sp = *ssp) != NULL;
		     ssp++)
			if (strcmp(name, sp) == 0)
				break;

		if (sp != NULL)
			/* Already configured as an "early" device */
			continue;

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
	int n, error;

	bzero(sa, sizeof(struct sbus_attach_args));
	error = getprop(node, "name", 1, &n, (void **)&sa->sa_name);
	if (error != 0)
		return (error);
	sa->sa_name[n] = '\0';

	sa->sa_bustag = bustag;
	sa->sa_dmatag = dmatag;
	sa->sa_node = node;

	error = getprop(node, "reg", sizeof(struct sbus_reg),
			&sa->sa_nreg, (void **)&sa->sa_reg);
	if (error != 0) {
		char buf[32];
		if (error != ENOENT ||
		    !node_has_property(node, "device_type") ||
		    strcmp(getpropstringA(node, "device_type", buf, sizeof buf),
			   "hierarchical") != 0)
			return (error);
	}
	for (n = 0; n < sa->sa_nreg; n++) {
		/* Convert to relative addressing, if necessary */
		u_int32_t base = sa->sa_reg[n].sbr_offset;
		if (SBUS_ABS(base)) {
			sa->sa_reg[n].sbr_slot = SBUS_ABS_TO_SLOT(base);
			sa->sa_reg[n].sbr_offset = SBUS_ABS_TO_OFFSET(base);
		}
	}

	if ((error = sbus_get_intr(sc, node, &sa->sa_intr, &sa->sa_nintr)) != 0)
		return (error);

	error = getprop(node, "address", sizeof(u_int32_t),
			 &sa->sa_npromvaddrs, (void **)&sa->sa_promvaddrs);
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
		free(sa->sa_promvaddrs, M_DEVBUF);

	bzero(sa, sizeof(struct sbus_attach_args));/*DEBUG*/
}


int
_sbus_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct sbus_softc *sc = t->cookie;
	int slot = btype;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr;
		bus_type_t iospace;

		if (sc->sc_range[i].cspace != slot)
			continue;

		/* We've found the connection to the parent bus */
		paddr = sc->sc_range[i].poffset + offset;
		iospace = sc->sc_range[i].pspace;
		return (bus_space_map2(sc->sc_bustag, iospace, paddr,
					size, flags, vaddr, hp));
	}

	return (EINVAL);
}

int
sbus_bus_mmap(t, btype, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t paddr;
	int flags;
	bus_space_handle_t *hp;
{
	int slot = (int)btype;
	int offset = (int)paddr;
	struct sbus_softc *sc = t->cookie;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr;
		bus_addr_t iospace;

		if (sc->sc_range[i].cspace != slot)
			continue;

		paddr = sc->sc_range[i].poffset + offset;
		iospace = (bus_addr_t)sc->sc_range[i].pspace;
		return (bus_space_mmap(sc->sc_bustag, iospace, paddr,
				       flags, hp));
	}

	return (-1);
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
 * Reset the given sbus. (???)
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
}


/*
 * Get interrupt attributes for an Sbus device.
 */
int
sbus_get_intr(sc, node, ipp, np)
	struct sbus_softc *sc;
	int node;
	struct sbus_intr **ipp;
	int *np;
{
	int error, n;
	u_int32_t *ipl = NULL;

	/*
	 * The `interrupts' property contains the Sbus interrupt level.
	 */
	if (getprop(node, "interrupts", sizeof(int), np, (void **)&ipl) == 0) {
		/* Change format to an `struct sbus_intr' array */
		struct sbus_intr *ip;
		ip = malloc(*np * sizeof(struct sbus_intr), M_DEVBUF, M_NOWAIT);
		if (ip == NULL)
			return (ENOMEM);
		for (n = 0; n < *np; n++) {
			ip[n].sbi_pri = ipl[n];
			ip[n].sbi_vec = 0;
		}
		free(ipl, M_DEVBUF);
		*ipp = ip;
		return (0);
	}

	/*
	 * Fall back on `intr' property.
	 */
	*ipp = NULL;
	error = getprop(node, "intr", sizeof(struct sbus_intr),
			np, (void **)ipp);
	switch (error) {
	case 0:
		for (n = *np; n-- > 0;) {
			(*ipp)[n].sbi_pri &= 0xf;
			(*ipp)[n].sbi_pri |= SBUS_INTR_COMPAT;
		}
		break;
	case ENOENT:
		error = 0;
		break;
	}

	return (error);
}


/*
 * Install an interrupt handler for an Sbus device.
 */
void *
sbus_intr_establish(t, level, flags, handler, arg)
	bus_space_tag_t t;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct sbus_softc *sc = t->cookie;
	struct intrhand *ih;
	int ipl;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	if ((flags & BUS_INTR_ESTABLISH_SOFTINTR) != 0)
		ipl = level;
	else if ((level & SBUS_INTR_COMPAT) != 0)
		ipl = level & ~SBUS_INTR_COMPAT;
	else
		ipl = sc->sc_intr2ipl[level];

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	if ((flags & BUS_INTR_ESTABLISH_FASTTRAP) != 0)
		intr_fasttrap(ipl, (void (*)__P((void)))handler);
	else
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
	sbt->sparc_bus_map = _sbus_bus_map;
	sbt->sparc_bus_mmap = sbus_bus_mmap;
	sbt->sparc_intr_establish = sbus_intr_establish;
	return (sbt);
}

int
sbus_error()
{
	struct sbus_softc *sc = sbus_sc;
	bus_space_handle_t bh = sc->sc_bh;
	u_int32_t afsr, afva;
	char bits[64];
static	int straytime, nstray;
	int timesince;

	afsr = bus_space_read_4(sc->sc_bustag, bh, SBUS_AFSR_REG);
	afva = bus_space_read_4(sc->sc_bustag, bh, SBUS_AFAR_REG);
	printf("sbus error:\n\tAFSR %s\n",
		bitmask_snprintf(afsr, SBUS_AFSR_BITS, bits, sizeof(bits)));
	printf("\taddress: 0x%x%x\n", afsr & SBUS_AFSR_PAH, afva);

	/* For now, do the same dance as on stray interrupts */
	timesince = time.tv_sec - straytime;
	if (timesince <= 10) {
		if (++nstray > 9)
			panic("too many SBus errors");
	} else {
		straytime = time.tv_sec;
		nstray = 1;
	}

	/* Unlock registers and clear interrupt */
	bus_space_write_4(sc->sc_bustag, bh, SBUS_AFSR_REG, afsr);

	return (0);
}
