/*	$NetBSD: upa.c,v 1.7.2.1 2001/10/01 12:42:25 fvdl Exp $ */

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

/* #define DEBUG_1 */

/*
 * UPA bus stuff.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <sparc64/dev/upavar.h>

#include <machine/autoconf.h>
#include <machine/ctlreg.h>
#include <machine/cpu.h>


void upareset __P((int));

static bus_space_tag_t upa_alloc_bustag __P((struct upa_softc *));
static int upa_get_intr __P((struct upa_softc *, int, int *));
static int upa_bus_mmap __P((bus_space_tag_t, bus_type_t, bus_addr_t,
			      int, bus_space_handle_t *));
static int _upa_bus_map __P((
		bus_space_tag_t,
		bus_type_t,		/*slot*/
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,		/*preferred virtual address */
		bus_space_handle_t *));
static void *upa_intr_establish __P((
		bus_space_tag_t,
		int,			/*level*/
		int,			/*flags*/
		int (*) __P((void *)),	/*handler*/
		void *));		/*handler arg*/


/* autoconfiguration driver */
int	upa_match_mainbus __P((struct device *, struct cfdata *, void *));
void	upa_attach_mainbus __P((struct device *, struct device *, void *));

struct cfattach upa_ca = {
	sizeof(struct upa_softc), upa_match, upa_attach
};

extern struct cfdriver upa_cd;


/*
 * Print the location of some upa-attached device (called just
 * before attaching that device).  If `upa' is not NULL, the
 * device was found but not configured; print the upa as well.
 * Return UNCONF (config_find ignores this if the device was configured).
 */
int
upa_print(args, busname)
	void *args;
	const char *busname;
{
	struct upa_attach_args *ua = args;

	if (busname)
		printf("%s at %s", ua->ua_name, busname);
	if (ua->ua_interrupts) {
		int level = ua->ua_pri;
		struct upa_softc *sc =
			(struct upa_softc *) ua->ua_bustag->cookie;

		printf(" interrupt %x", ua->ua_interrupts);
	}
	return (UNCONF);
}

int
upa_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0);
}

int
upa_match_iommu(parent, cf, aux)
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
upa_match_xbox(parent, cf, aux)
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
 * Attach an Upa.
 */
void
upa_attach_mainbus(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct upa_softc *sc = (struct upa_softc *)self;
	struct mainbus_attach_args *ma = aux;
	int node = ma->ma_node;

	/*
	 * XXX there is only one Upa, for now -- do not know how to
	 * address children on others
	 */
	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_dmatag = ma->ma_dmatag;

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C
				? intr_upa2ipl_4c
				: intr_upa2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = PROM_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	upa_attach(sc, "upa", node, NULL);
}

void
upa_attach_iommu(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct upa_softc *sc = (struct upa_softc *)self;
	struct iommu_attach_args *ia = aux;
	int node = ia->iom_node;

	sc->sc_bustag = ia->iom_bustag;
	sc->sc_dmatag = ia->iom_dmatag;

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C ? intr_upa2ipl_4c : intr_upa2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = PROM_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	upa_attach(sc, "upa", node, NULL);
}

void
upa_attach_xbox(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct upa_softc *sc = (struct upa_softc *)self;
	struct xbox_attach_args *xa = aux;
	int node = xa->xa_node;

	sc->sc_bustag = xa->xa_bustag;
	sc->sc_dmatag = xa->xa_dmatag;

	/* Setup interrupt translation tables */
	sc->sc_intr2ipl = CPU_ISSUN4C ? intr_upa2ipl_4c : intr_upa2ipl_4m;

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = PROM_getpropint(node, "clock-frequency", 25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	upa_attach(sc, "upa", node, NULL);
}

void
upa_attach(sc, busname, busnode, specials)
	struct upa_softc *sc;
	char *busname;
	int busnode;
	const char * const *specials;
{
	int node0, node, error;
	const char *sp;
	const char *const *ssp;
	bus_space_tag_t sbt;
	struct upa_attach_args ua;

	sbt = upa_alloc_bustag(sc);

	/*
	 * Get the Upa burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = PROM_getpropint(busnode, "burst-sizes", 0);

	/*
	 * Collect address translations from the OBP.
	 */
	error = PROM_getprop(busnode, "ranges", sizeof(struct rom_range),
			 &sc->sc_nrange, (void **)&sc->sc_range);
	switch (error) {
	case 0:
		break;
	case ENOENT:
		/* Fall back to our own `range' construction */
		sc->sc_range = upa_translations;
		sc->sc_nrange =
			sizeof(upa_translations)/sizeof(upa_translations[0]);
		break;
	default:
		panic("%s: error getting ranges property", sc->sc_dev.dv_xname);
	}

/* WARNING -- this stuff needs to be set somewhere */
	sc->sc_sysio = (struct sysioreg*) ra->ra_vaddr;		/* Use prom mapping for sysio. */
	sc->sc_ign = ra->ra_interrupt[0] & INTMAP_IGN;		/* Find interrupt group no */


	/*
	 * Setup the iommu.
	 *
	 * The sun4u iommu is part of the UPA controller so we will
	 * deal with it here.  We could try to fake a device node so
	 * we can eventually share it with the PCI bus run by psyco,
	 * but I don't want to get into that sort of cruft.
	 */
#ifdef NOTDEF_DEBUG
	{
		/* Probe the iommu */
		int64_t cr, tsb;

		printf("\niommu regs at: cr=%x tsb=%x flush=%x\n", &sc->sc_sysio->sys_iommu.iommu_cr,
		       &sc->sc_sysio->sys_iommu.iommu_tsb, &sc->sc_sysio->sys_iommu.iommu_flush);
		cr = sc->sc_sysio->sys_iommu.iommu_cr;
		tsb = sc->sc_sysio->sys_iommu.iommu_tsb;
		printf("iommu cr=%x:%x tsb=%x:%x\n", (long)(cr>>32), (long)cr, (long)(tsb>>32), (long)tsb);
		delay(1000000); /* 1 s */
	}
#endif
	
	/* 
	 * All IOMMUs will share the same TSB which is allocated in pmap_bootstrap.
	 *
	 * This makes device management easier.
	 */
	sc->sc_tsbsize = iotsbsize;
	sc->sc_tsb = iotsb;
	sc->sc_ptsb = iotsbp;
#if 0
	/* Need to do 64-bit stores */
	sc->sc_sysio->sys_iommu.iommu_cr = (IOMMUCR_TSB1K|IOMMUCR_8KPG|IOMMUCR_EN);
	sc->sc_sysio->sys_iommu.iommu_tsb = sc->sc_ptsb;
#else
	stxa(&sc->sc_sysio->sys_iommu.iommu_cr,ASI_NUCLEUS,(IOMMUCR_TSB1K|IOMMUCR_8KPG|IOMMUCR_EN));
	stxa(&sc->sc_sysio->sys_iommu.iommu_tsb,ASI_NUCLEUS,sc->sc_ptsb);
#endif
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

		if (upa_setup_attach_args(sc, sbt, sc->sc_dmatag,
					   node, &ua) != 0) {
			panic("upa_attach: %s: incomplete", sp);
		}
		(void) config_found(&sc->sc_dev, (void *)&ua, upa_print);
	}

	for (node = node0; node; node = nextsibling(node)) {
		char *name = PROM_getpropstring(node, "name");
		for (ssp = specials, sp = NULL;
		     ssp != NULL && (sp = *ssp) != NULL;
		     ssp++)
			if (strcmp(name, sp) == 0)
				break;

		if (sp != NULL)
			/* Already configured as an "early" device */
			continue;

<<<<<<<<<<<<<< variant A
		upa_translate(self, &oca);
		oca.ca_bustype = BUS_UPA;
		/* Now we need to enable this interrupt if a handler has been registered */
		if( config_found(&sc->sc_dev, (void *)&oca, upa_print) != NULL )
			for( i=0; i<oca.ca_ra.ra_ninterrupt; i++) {
#ifdef IRQEN_DEBUG
				printf("\nupa: intr[%d]%x: %x\n", i, oca.ca_ra.ra_interrupt[i], 
				       intrlev[oca.ca_ra.ra_interrupt[i]]);
#endif
				if( intrlev[oca.ca_ra.ra_interrupt[i]] ) {
					/* Hunt for proper register UGH! */
#ifdef IRQEN_DEBUG
					printf("Hunting for IRQ...\n");
#endif
					for( intrptr=&sc->sc_sysio->scsi_int_map;
					     intrptr < &sc->sc_sysio->reserved_int_map &&
						     ((intrmap=*intrptr)&INTMAP_INR) 
						     != oca.ca_ra.ra_interrupt[i]; 
					     intrptr++);
					if((intrmap&INTMAP_INR) == 
					   oca.ca_ra.ra_interrupt[i]) {
#ifdef IRQEN_DEBUG
						printf("Found %x IRQ as %x:%x in slot\n", 
						       oca.ca_ra.ra_interrupt[i], (int)(intrmap>>32), (int)intrmap, 
						       intrptr - &sc->sc_sysio->scsi_int_map);
#endif
						/* Enable the interrupt */
						intrmap |= INTMAP_V;
						stxa(intrptr, ASI_NUCLEUS, intrmap);
						/* Register the map and clear intr registers */
						intrlev[oca.ca_ra.ra_interrupt[i]]->ih_map = intrptr;
						intrlev[oca.ca_ra.ra_interrupt[i]]->ih_clr = 
							&sc->sc_sysio->scsi_clr_int + 
							(intrptr - &sc->sc_sysio->scsi_int_map);
					}
				}
			}
#ifdef IRQEN_DEBUG
		for( i=0; i<140000000; i++);
#endif
>>>>>>>>>>>>>> variant B
		if (upa_setup_attach_args(sc, sbt, sc->sc_dmatag,
					   node, bp, &ua) != 0) {
			printf("upa_attach: %s: incomplete\n", name);
			continue;
		}
		(void) config_found(&sc->sc_dev, (void *)&ua, upa_print);
======= end of combination
	}
}

int
upa_setup_attach_args(sc, bustag, dmatag, node, ua)
	struct upa_softc	*sc;
	bus_space_tag_t		bustag;
	bus_dma_tag_t		dmatag;
	int			node;
	struct upa_attach_args	*ua;
{
	struct	rom_reg romreg;
	int	base;
	int	error;

	bzero(ua, sizeof(struct upa_attach_args));
	ua->ua_name = PROM_getpropstring(node, "name");
	ua->ua_bustag = bustag;
	ua->ua_dmatag = dmatag;
	ua->ua_node = node;

	if ((error = PROM_getprop_reg1(node, &romreg)) != 0)
		return (error);

	/* We pass only the first "reg" property */
	base = (int)romreg.rr_paddr;
	if (UPA_ABS(base)) {
		ua->ua_slot = UPA_ABS_TO_SLOT(base);
		ua->ua_offset = UPA_ABS_TO_OFFSET(base);
	} else {
		ua->ua_slot = romreg.rr_iospace;
		ua->ua_offset = base;
	}
	ua->ua_size = romreg.rr_len;

	if ((error = upa_get_intr(sc, node, &ua->ua_pri)) != 0)
		return (error);

	if ((error = PROM_getprop_address1(node, &ua->ua_promvaddr)) != 0)
		return (error);

	return (0);
}

int
_upa_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct upa_softc *sc = t->cookie;
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
upa_bus_mmap(t, btype, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t paddr;
	int flags;
	bus_space_handle_t *hp;
{
	int slot = (int)btype;
	int offset = (int)paddr;
	struct upa_softc *sc = t->cookie;
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
 * Each attached device calls upa_establish after it initializes
 * its upadev portion.
 */
void
upa_establish(sd, dev)
	register struct upadev *sd;
	register struct device *dev;
{
	register struct upa_softc *sc;
	register struct device *curdev;

	/*
	 * We have to look for the upa by name, since it is not necessarily
	 * our immediate parent (i.e. sun4m /iommu/upa/espdma/esp)
	 * We don't just use the device structure of the above-attached
	 * upa, since we might (in the future) support multiple upa's.
	 */
	for (curdev = dev->dv_parent; ; curdev = curdev->dv_parent) {
		if (!curdev || !curdev->dv_xname)
			panic("upa_establish: can't find upa parent for %s",
			      sd->sd_dev->dv_xname
					? sd->sd_dev->dv_xname
					: "<unknown>" );

		if (strncmp(curdev->dv_xname, "upa", 4) == 0)
			break;
	}
	sc = (struct upa_softc *) curdev;

	sd->sd_dev = dev;
	sd->sd_bchain = sc->sc_sbdev;
	sc->sc_sbdev = sd;
}

/*
 * Reset the given upa. (???)
 */
void
upareset(upa)
	int upa;
{
	register struct upadev *sd;
	struct upa_softc *sc = upa_cd.cd_devs[upa];
	struct device *dev;

	printf("reset %s:", sc->sc_dev.dv_xname);
	for (sd = sc->sc_sbdev; sd != NULL; sd = sd->sd_bchain) {
		if (sd->sd_reset) {
			dev = sd->sd_dev;
			(*sd->sd_reset)(dev);
			printf(" %s", dev->dv_xname);
		}
	}
#if 0
	/* Reload iommu regs */
	sc->sc_sysio->sys_iommu.iommu_cr = (IOMMUCR_TSB1K|IOMMUCR_8KPG|IOMMUCR_EN);
	sc->sc_sysio->sys_iommu.iommu_tsb = sc->sc_ptsb;
#else
	/* Reload iommu regs */
	stxa(&sc->sc_sysio->sys_iommu.iommu_cr,ASI_NUCLEUS,(IOMMUCR_TSB1K|IOMMUCR_8KPG|IOMMUCR_EN));
	stxa(&sc->sc_sysio->sys_iommu.iommu_tsb,ASI_NUCLEUS,sc->sc_ptsb);
#endif
}

/*
 * Here are the iommu control routines. 
 */
void
upa_enter(va, pa)
	vaddr_t va;
	paddr_t pa;
{
	struct upa_softc *sc = upa_sc;
	int64_t tte;

#ifdef DIAGNOSTIC
	if (va < sc->sc_dvmabase)
		panic("upa_enter: va 0x%x not in DVMA space",va);
#endif

#ifdef 1
	/* Streaming */
	tte = MAKEIOTTE(pa, 1, 1, 1);
#else
	/* Consistent */
	tte = MAKEIOTTE(pa, 1, 1, 0);
#endif
	
	sc->sc_tsb[IOTSBSLOT(va,sc->sc_tsbsize)] = tte;
#if 0
	sc->sc_sysio->sys_iommu.iommu_flush = va;
#else
	stxa(&sc->sc_sysio->sys_iommu.iommu_flush,ASI_NUCLEUS,va);
#endif
#ifdef DEBUG_1
	printf("upa_enter: added xlation va %x pa %x:%x TSB[%x]=%x:%x\n",
	       va, (int)(pa>>32), (int)pa, IOTSBSLOT(va,sc->sc_tsbsize), (int)(tte>>32), (int)tte);
#endif
}
/*
 * upa_clear: clears mappings created by upa_enter
 */
void
upa_remove(va, len)
	register vaddr_t va;
	register u_int len;
{
	register struct upa_softc *sc = upa_sc;

#ifdef DIAGNOSTIC
	if (va < sc->sc_dvmabase)
		panic("upa_remove: va 0x%x not in DVMA space", va);
#endif

	while (len > 0) {
		static volatile int flushdone; 
		int flushtimeout;
		extern u_int ksegv;
		extern u_int64_t ksegp;

		/*
		 * Streaming buffer flushes:
		 * 
		 *   1 Tell strbuf to flush by storing va to strbuf_pgflush
		 * If we're not on a cache line boundary (64-bits):
		 *   2 Store 0 in flag
		 *   3 Store pointer to flag in flushsync
		 *   4 wait till flushsync becomes 0x1
		 *
		 * If it takes more than .5 sec, something went wrong.
		 */
#if 0
		sc->sc_sysio->sys_strbuf.strbuf_pgflush = va;
#else
		stxa(&(sc->sc_sysio->sys_strbuf.strbuf_pgflush), ASI_NUCLEUS, va);
#endif
		if( len < NBPG && ((va+len) & 0x3f) ) {
			flushdone = 0;
			/*
			 * KLUGE ALERT KLUGE ALERT
			 *
			 * In order not to bother with pmap_extract() to do the vtop 
			 * translation, flushdone is a static variable that resides in 
			 * the kernel's 4MB locked TTE.  This means that this routine
			 * is NOT re-entrant.  Since we're single-threaded and poll
			 * on this value, this is currently not a problem.
			 */
#ifdef DEBUG_1
			printf("upa_remove: flush = %x at va = %x pa = %x\n", flushdone, &flushdone, (long)(((long)&flushdone) - ksegv + ksegp));
#endif
#if 0
			sc->sc_sysio->sys_strbuf.strbuf_flushsync = (long)(((long)&flushdone) - ksegv + ksegp);
#else
			stxa(&sc->sc_sysio->sys_strbuf.strbuf_flushsync, ASI_NUCLEUS, (long)(((long)&flushdone) - ksegv + ksegp));
#endif
			flushtimeout = 250000000; /* 1 sec on a 250MHz machine */
			while( !flushdone && flushtimeout--) membar_sync();
#ifdef DIAGNOSTIC
			if( !flushdone )
				printf("upa_remove: flush timeout %x at %x\n", flushdone, (long)(((long)&flushdone) - ksegv + ksegp)); /* panic? */
#endif
		}
		sc->sc_tsb[IOTSBSLOT(va,sc->sc_tsbsize)] = 0;
#if 0
		sc->sc_sysio->sys_iommu.iommu_flush = va;
#else
		stxa(&sc->sc_sysio->sys_iommu.iommu_flush, ASI_NUCLEUS, va);
#endif
		len -= NBPG;
		va += NBPG;
	}
}


/*
 * Get interrupt attributes for an Upa device.
 */
int
upa_get_intr(sc, node, ip)
	struct upa_softc *sc;
	int node;
	int *ip;
{
	struct rom_intr *rip;
	int *ipl;
	int n;

	/*
	 * The `interrupts' property contains the Upa interrupt level.
	 */
	ipl = NULL;
	if (PROM_getprop(node, "interrupts", sizeof(int), &n, (void **)&ipl) == 0) {
		*ip = ipl[0];
		free(ipl, M_DEVBUF);
		return (0);
	}

	/*
	 * Fall back on `intr' property.
	 */
	rip = NULL;
	switch (PROM_getprop(node, "intr", sizeof(*rip), &n, (void **)&rip)) {
	case 0:
		*ip = (rip[0].int_pri & 0xf) | UPA_INTR_COMPAT;
		free(rip, M_DEVBUF);
		return (0);
	case ENOENT:
		*ip = 0;
		return (0);
	}

	return (-1);
}


/*
 * Install an interrupt handler for an Upa device.
 */
void *
upa_intr_establish(t, level, flags, handler, arg)
	bus_space_tag_t t;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct upa_softc *sc = t->cookie;
	struct intrhand *ih;
	int ipl;

	ih = (struct intrhand *)
		malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	if ((flags & BUS_INTR_ESTABLISH_SOFTINTR) != 0)
		ipl = level;
	else if ((level & UPA_INTR_COMPAT) != 0)
		ipl = level & ~UPA_INTR_COMPAT;
	else
		ipl = sc->sc_intr2ipl[level];

	ih->ih_fun = handler;
	ih->ih_arg = arg;
	intr_establish(ipl, ih);
	return (ih);
}

static bus_space_tag_t
upa_alloc_bustag(sc)
	struct upa_softc *sc;
{
	bus_space_tag_t sbt;

	sbt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (sbt == NULL)
		return (NULL);

	bzero(sbt, sizeof *sbt);
	sbt->cookie = sc;
	sbt->parent = sc->sc_bustag;
	sbt->sparc_bus_map = _upa_bus_map;
	sbt->sparc_bus_mmap = upa_bus_mmap;
	sbt->sparc_intr_establish = upa_intr_establish;
	return (sbt);
}
