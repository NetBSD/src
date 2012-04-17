/*	$NetBSD: sbus.c,v 1.75.2.1 2012/04/17 00:06:54 yamt Exp $ */

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbus.c,v 1.75.2.1 2012/04/17 00:06:54 yamt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <sparc/dev/sbusreg.h>
#include <dev/sbus/sbusvar.h>
#include <dev/sbus/xboxvar.h>

#include <sparc/sparc/iommuvar.h>

void sbusreset(int);

static int sbus_get_intr(struct sbus_softc *, int,
			 struct openprom_intr **, int *);
static void *sbus_intr_establish(
		bus_space_tag_t,
		int,			/* Sbus interrupt level */
		int,			/* `device class' priority */
		int (*)(void *),	/* handler */
		void *,			/* handler arg */
		void (*)(void));	/* fast handler */


/* autoconfiguration driver */
int	sbus_match_mainbus(device_t, struct cfdata *, void *);
int	sbus_match_iommu(device_t, struct cfdata *, void *);
int	sbus_match_xbox(device_t, struct cfdata *, void *);
void	sbus_attach_mainbus(device_t, device_t, void *);
void	sbus_attach_iommu(device_t, device_t, void *);
void	sbus_attach_xbox(device_t, device_t, void *);

static	int sbus_error(void);
int	(*sbuserr_handler)(void);

CFATTACH_DECL_NEW(sbus_mainbus, sizeof(struct sbus_softc),
    sbus_match_mainbus, sbus_attach_mainbus, NULL, NULL);

CFATTACH_DECL_NEW(sbus_iommu, sizeof(struct sbus_softc),
    sbus_match_iommu, sbus_attach_iommu, NULL, NULL);

CFATTACH_DECL_NEW(sbus_xbox, sizeof(struct sbus_softc),
    sbus_match_xbox, sbus_attach_xbox, NULL, NULL);

extern struct cfdriver sbus_cd;

static int sbus_mainbus_attached;

/* The "primary" Sbus */
struct sbus_softc *sbus_sc;

/* If the PROM does not provide the `ranges' property, we make up our own */
struct openprom_range sbus_translations[] = {
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
sbus_print(void *args, const char *busname)
{
	struct sbus_attach_args *sa = args;
	int i;

	if (busname)
		aprint_normal("%s at %s", sa->sa_name, busname);
	aprint_normal(" slot %d offset 0x%x", sa->sa_slot, sa->sa_offset);
	for (i = 0; i < sa->sa_nintr; i++) {
		uint32_t level = sa->sa_intr[i].oi_pri;
		struct sbus_softc *sc =
			(struct sbus_softc *) sa->sa_bustag->cookie;

		aprint_normal(" level %d", level & ~SBUS_INTR_COMPAT);
		if ((level & SBUS_INTR_COMPAT) == 0) {
			int ipl = sc->sc_intr2ipl[level];
			if (ipl != level)
				aprint_normal(" (ipl %d)", ipl);
		}
	}
	return (UNCONF);
}

int
sbus_match_mainbus(device_t parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (CPU_ISSUN4 || sbus_mainbus_attached)
		return (0);

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

int
sbus_match_iommu(device_t parent, struct cfdata *cf, void *aux)
{
	struct iommu_attach_args *ia = aux;

	if (CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_name, ia->iom_name) == 0);
}

int
sbus_match_xbox(device_t parent, struct cfdata *cf, void *aux)
{
	struct xbox_attach_args *xa = aux;

	if (CPU_ISSUN4)
		return (0);

	return (strcmp(cf->cf_name, xa->xa_name) == 0);
}

/*
 * Attach an Sbus.
 */
void
sbus_attach_mainbus(device_t parent, device_t self, void *aux)
{
	struct sbus_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int node = ma->ma_node;

	sbus_mainbus_attached = 1;

	sc->sc_dev = self;
	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

#if 0	/* sbus at mainbus (sun4c): `reg' prop is not control space */
	if (ma->ma_size == 0)
		printf("%s: no Sbus registers", device_xname(self));

	if (bus_space_map(ma->ma_bustag,
			  ma->ma_paddr,
			  ma->ma_size,
			  BUS_SPACE_MAP_LINEAR,
			  &sc->sc_bh) != 0) {
		panic("%s: can't map sbusbusreg", device_xname(self));
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
	sc->sc_clockfreq = prom_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_sc = sc;
	sbus_attach_common(sc, "sbus", node, NULL);
}


void
sbus_attach_iommu(device_t parent, device_t self, void *aux)
{
	struct sbus_softc *sc = device_private(self);
	struct iommu_attach_args *ia = aux;
	int node = ia->iom_node;

	sc->sc_dev = self;
	sc->sc_bustag = ia->iom_bustag;
	sc->sc_dmatag = ia->iom_dmatag;

	if (ia->iom_nreg == 0)
		panic("%s: no Sbus registers", device_xname(self));

	if (bus_space_map(ia->iom_bustag,
			  BUS_ADDR(ia->iom_reg[0].oa_space,
				   ia->iom_reg[0].oa_base),
			  (bus_size_t)ia->iom_reg[0].oa_size,
			  BUS_SPACE_MAP_LINEAR,
			  &sc->sc_bh) != 0) {
		panic("%s: can't map sbusbusreg", device_xname(self));
	}

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C ? intr_sbus2ipl_4c : intr_sbus2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = prom_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_sc = sc;
	sbuserr_handler = sbus_error;
	sbus_attach_common(sc, "sbus", node, NULL);
}

void
sbus_attach_xbox(device_t parent, device_t self, void *aux)
{
	struct sbus_softc *sc = device_private(self);
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
	sc->sc_clockfreq = prom_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbus_attach_common(sc, "sbus", node, NULL);
}

void
sbus_attach_common(struct sbus_softc *sc, const char *busname, int busnode,
		   const char * const *specials)
{
	int node0, node, error;
	const char *sp;
	const char *const *ssp;
	bus_space_tag_t sbt;
	struct sbus_attach_args sa;

	if ((sbt = bus_space_tag_alloc(sc->sc_bustag, sc)) == NULL) {
		printf("%s: attach: out of memory\n",
		    device_xname(sc->sc_dev));
		return;
	}
	sbt->sparc_intr_establish = sbus_intr_establish;

	/*
	 * Get the SBus burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = prom_getpropint(busnode, "burst-sizes", 0);


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
	error = prom_getprop(busnode, "ranges", sizeof(struct rom_range),
			&sbt->nranges, &sbt->ranges);
	switch (error) {
	case 0:
		break;
	case ENOENT:
		/* Fall back to our own `range' construction */
		sbt->ranges = sbus_translations;
		sbt->nranges =
			sizeof(sbus_translations)/sizeof(sbus_translations[0]);
		break;
	default:
		panic("%s: error getting ranges property",
		    device_xname(sc->sc_dev));
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
		(void) config_found(sc->sc_dev, (void *)&sa, sbus_print);
		sbus_destroy_attach_args(&sa);
	}

	for (node = node0; node; node = nextsibling(node)) {
		char *name = prom_getpropstring(node, "name");
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
		(void) config_found(sc->sc_dev, (void *)&sa, sbus_print);
		sbus_destroy_attach_args(&sa);
	}
}

int
sbus_setup_attach_args(struct sbus_softc *sc,
		       bus_space_tag_t bustag, bus_dma_tag_t dmatag, int node,
		       struct sbus_attach_args *sa)
{
	int n, error;

	memset(sa, 0, sizeof(struct sbus_attach_args));
	error = prom_getprop(node, "name", 1, &n, &sa->sa_name);
	if (error != 0)
		return (error);
	KASSERT(sa->sa_name[n-1] == '\0');

	sa->sa_bustag = bustag;
	sa->sa_dmatag = dmatag;
	sa->sa_node = node;
	sa->sa_frequency = sc->sc_clockfreq;

	error = prom_getprop(node, "reg", sizeof(struct openprom_addr),
			&sa->sa_nreg, &sa->sa_reg);
	if (error != 0) {
		char buf[32];
		if (error != ENOENT ||
		    !node_has_property(node, "device_type") ||
		    strcmp(prom_getpropstringA(node, "device_type", buf, sizeof buf),
			   "hierarchical") != 0)
			return (error);
	}
	for (n = 0; n < sa->sa_nreg; n++) {
		/* Convert to relative addressing, if necessary */
		uint32_t base = sa->sa_reg[n].oa_base;
		if (SBUS_ABS(base)) {
			sa->sa_reg[n].oa_space = SBUS_ABS_TO_SLOT(base);
			sa->sa_reg[n].oa_base = SBUS_ABS_TO_OFFSET(base);
		}
	}

	if ((error = sbus_get_intr(sc, node, &sa->sa_intr, &sa->sa_nintr)) != 0)
		return (error);

	error = prom_getprop(node, "address", sizeof(uint32_t),
			 &sa->sa_npromvaddrs, &sa->sa_promvaddrs);
	if (error != 0 && error != ENOENT)
		return (error);

	return (0);
}

void
sbus_destroy_attach_args(struct sbus_attach_args *sa)
{

	if (sa->sa_name != NULL)
		free(sa->sa_name, M_DEVBUF);

	if (sa->sa_nreg != 0)
		free(sa->sa_reg, M_DEVBUF);

	if (sa->sa_intr)
		free(sa->sa_intr, M_DEVBUF);

	if (sa->sa_promvaddrs)
		free(sa->sa_promvaddrs, M_DEVBUF);

	memset(sa, 0, sizeof(struct sbus_attach_args));/*DEBUG*/
}

bus_addr_t
sbus_bus_addr(bus_space_tag_t t, u_int btype, u_int offset)
{

	/* XXX: sbus_bus_addr should be g/c'ed */
	return (BUS_ADDR(btype, offset));
}


/*
 * Get interrupt attributes for an Sbus device.
 */
static int
sbus_get_intr(struct sbus_softc *sc, int node,
	      struct openprom_intr **ipp, int *np)
{
	int error, n;
	uint32_t *ipl = NULL;

	/*
	 * The `interrupts' property contains the Sbus interrupt level.
	 */
	if (prom_getprop(node, "interrupts", sizeof(int), np,
			 &ipl) == 0) {
		/* Change format to an `struct openprom_intr' array */
		struct openprom_intr *ip;
		ip = malloc(*np * sizeof(struct openprom_intr), M_DEVBUF,
		    M_NOWAIT);
		if (ip == NULL) {
			free(ipl, M_DEVBUF);
			return (ENOMEM);
		}
		for (n = 0; n < *np; n++) {
			ip[n].oi_pri = ipl[n];
			ip[n].oi_vec = 0;
		}
		free(ipl, M_DEVBUF);
		*ipp = ip;
		return (0);
	}

	/*
	 * Fall back on `intr' property.
	 */
	*ipp = NULL;
	error = prom_getprop(node, "intr", sizeof(struct openprom_intr),
			np, ipp);
	switch (error) {
	case 0:
		for (n = *np; n-- > 0;) {
			(*ipp)[n].oi_pri &= 0xf;
			(*ipp)[n].oi_pri |= SBUS_INTR_COMPAT;
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
static void *
sbus_intr_establish(bus_space_tag_t t, int pri, int level,
		    int (*handler)(void *), void *arg,
		    void (*fastvec)(void))
{
	struct sbus_softc *sc = t->cookie;
	struct intrhand *ih;
	int pil;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	/*
	 * Translate Sbus interrupt priority to CPU interrupt level
	 */
	if ((pri & SBUS_INTR_COMPAT) != 0)
		pil = pri & ~SBUS_INTR_COMPAT;
	else
		pil = sc->sc_intr2ipl[pri];

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(pil, level, ih, fastvec, false);
	return (ih);
}

static int
sbus_error(void)
{
	struct sbus_softc *sc = sbus_sc;
	bus_space_handle_t bh = sc->sc_bh;
	uint32_t afsr, afva;
	char bits[64];
static	int straytime, nstray;
	int timesince;

	afsr = bus_space_read_4(sc->sc_bustag, bh, SBUS_AFSR_REG);
	afva = bus_space_read_4(sc->sc_bustag, bh, SBUS_AFAR_REG);
	snprintb(bits, sizeof(bits), SBUS_AFSR_BITS, afsr);
	printf("sbus error:\n\tAFSR %s\n", bits);
	printf("\taddress: 0x%x%x\n", afsr & SBUS_AFSR_PAH, afva);

	/* For now, do the same dance as on stray interrupts */
	timesince = time_uptime - straytime;
	if (timesince <= 10) {
		if (++nstray > 9)
			panic("too many SBus errors");
	} else {
		straytime = time_uptime;
		nstray = 1;
	}

	/* Unlock registers and clear interrupt */
	bus_space_write_4(sc->sc_bustag, bh, SBUS_AFSR_REG, afsr);

	return (0);
}
