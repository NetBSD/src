/*	$NetBSD: sbus.c,v 1.93.6.1 2017/12/03 11:36:44 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sbus.c,v 1.93.6.1 2017/12/03 11:36:44 jdolecek Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <sys/bus.h>
#include <machine/openfirm.h>

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

void sbusreset(int);

static bus_dma_tag_t sbus_alloc_dmatag(struct sbus_softc *);
static int sbus_get_intr(struct sbus_softc *, int, struct openprom_intr **,
	int *, int);
static int sbus_overtemp(void *);
static int _sbus_bus_map(
		bus_space_tag_t,
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,		/* XXX unused -- compat w/sparc */
		bus_space_handle_t *);
static void *sbus_intr_establish(
		bus_space_tag_t,
		int,			/*`device class' priority*/
		int,			/*Sbus interrupt level*/
		int (*)(void *),	/*handler*/
		void *,			/*handler arg*/
		void (*)(void));	/*optional fast trap*/


/* autoconfiguration driver */
int	sbus_match(device_t, cfdata_t, void *);
void	sbus_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(sbus, sizeof(struct sbus_softc),
    sbus_match, sbus_attach, NULL, NULL);

extern struct cfdriver sbus_cd;

/*
 * DVMA routines
 */
static int sbus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	bus_size_t, int, bus_dmamap_t *);

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
sbus_print(void *args, const char *busname)
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
sbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach an Sbus.
 */
void
sbus_attach(device_t parent, device_t self, void *aux)
{
	struct sbus_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	struct intrhand *ih;
	int ipl;
	char *name;
	int node = ma->ma_node;
	int node0, error;
	bus_space_tag_t sbt;
	struct sbus_attach_args sa;

	sc->sc_dev = self;
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
		aprint_error_dev(self, "cannot map registers\n");
		return;
	}
#endif

	/*
	 * Record clock frequency for synchronous SCSI.
	 * IS THIS THE CORRECT DEFAULT??
	 */
	sc->sc_clockfreq = prom_getpropint(node, "clock-frequency", 
		25*1000*1000);
	printf(": clock = %s MHz\n", clockfreq(sc->sc_clockfreq));

	sbt = bus_space_tag_alloc(sc->sc_bustag, sc);
	sbt->type = SBUS_BUS_SPACE;
	sbt->sparc_bus_map = _sbus_bus_map;
	sbt->sparc_intr_establish = sbus_intr_establish;

	sc->sc_dmatag = sbus_alloc_dmatag(sc);

	/*
	 * Get the SBus burst transfer size if burst transfers are supported
	 */
	sc->sc_burst = prom_getpropint(node, "burst-sizes", 0);

	/*
	 * Collect address translations from the OBP.
	 */
	error = prom_getprop(node, "ranges", sizeof(struct openprom_range),
			 &sbt->nranges, &sbt->ranges);
	if (error)
		panic("%s: error getting ranges property", device_xname(self));

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
	snprintf(name, 32, "%s dvma", device_xname(self));

	iommu_init(name, &sc->sc_is, 0, -1);

	/* Enable the over temp intr */
	ih = intrhand_alloc();
	ih->ih_map = &sc->sc_sysio->therm_int_map;
	ih->ih_clr = NULL; /* &sc->sc_sysio->therm_clr_int; */
	ih->ih_fun = sbus_overtemp;
	ipl = 1;
	ih->ih_pil = ipl;
	ih->ih_number = INTVEC(*(ih->ih_map));
	ih->ih_pending = 0;
	intr_establish(ipl, true, ih);
	*(ih->ih_map) |= INTMAP_V|(CPU_UPAID << INTMAP_TID_SHIFT);
	
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
		char *name1 = prom_getpropstring(node, "name");

		if (sbus_setup_attach_args(sc, sbt, sc->sc_dmatag,
					   node, &sa) != 0) {
			printf("sbus_attach: %s: incomplete\n", name1);
			continue;
		}
		(void) config_found(self, &sa, sbus_print);
		sbus_destroy_attach_args(&sa);
	}
}

int
sbus_setup_attach_args(struct sbus_softc *sc, bus_space_tag_t bustag,
	bus_dma_tag_t dmatag, int node, struct sbus_attach_args	*sa)
{
	/*struct	openprom_addr sbusreg;*/
	/*int	base;*/
	int	error;
	int n;

	memset(sa, 0, sizeof(struct sbus_attach_args));
	n = 0;
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

	if ((error = sbus_get_intr(sc, node, &sa->sa_intr, &sa->sa_nintr,
	    sa->sa_slot)) != 0)
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
		free((void *)sa->sa_promvaddrs, M_DEVBUF);

	memset(sa, 0, sizeof(struct sbus_attach_args)); /*DEBUG*/
}


int
_sbus_bus_map(bus_space_tag_t t, bus_addr_t addr, bus_size_t size, int flags,
	vaddr_t v, bus_space_handle_t *hp)
{
	int error;

	if (t->ranges != NULL) {
		if ((error = bus_space_translate_address_generic(
				t->ranges, t->nranges, &addr)) != 0)
			return (error);
	}

	/*
	 * BUS_SPACE_MAP_PREFETCHABLE doesn't work right through sbus, so weed
	 * it out for now until we know better
	 */

	flags &= ~BUS_SPACE_MAP_PREFETCHABLE;

	return (bus_space_map(t->parent, addr, size, flags, hp));
}


bus_addr_t
sbus_bus_addr(bus_space_tag_t t, u_int btype, u_int offset)
{
	int slot = btype;
	struct openprom_range *rp;
	int i;

	for (i = 0; i < t->nranges; i++) {
		rp = &t->ranges[i];
		if (rp->or_child_space != slot)
			continue;

		return BUS_ADDR(rp->or_parent_space,
				rp->or_parent_base + offset);
	}

	return (0);
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
sbus_overtemp(void *arg)
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
sbus_get_intr(struct sbus_softc *sc, int node, struct openprom_intr **ipp,
	int *np, int slot)
{
	int *ipl;
	int n, i;
	char buf[32];

	/*
	 * The `interrupts' property contains the Sbus interrupt level.
	 */
	ipl = NULL;
	if (prom_getprop(node, "interrupts", sizeof(int), np, &ipl) == 0) {
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
		prom_getpropstringA(node, "device_type", buf, sizeof buf);
		if (buf[0] == '\0')
			prom_getpropstringA(node, "name", buf, sizeof buf);

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
sbus_intr_establish(bus_space_tag_t t, int pri, int level,
	int (*handler)(void *), void *arg, void (*fastvec)(void))
{
	struct sbus_softc *sc = t->cookie;
	struct intrhand *ih;
	int ipl;
	long vec = pri;

	ih = intrhand_alloc();

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
				int64_t imap = *ih->ih_map;
				
				printf("SBUS %lx IRQ as %llx in slot %d\n", 
				       (long)vec, (long long)imap, slot);
				printf("\tmap addr %p clr addr %p\n",
				    ih->ih_map, ih->ih_clr);
			}
#endif
			/* Enable the interrupt */
			vec |= INTMAP_V | sc->sc_ign |
				(CPU_UPAID << INTMAP_TID_SHIFT);
			*(ih->ih_map) = vec;
		} else {
			int64_t *intrptr = &sc->sc_sysio->scsi_int_map;
			int64_t imap = 0;
			int i;

			/* Insert IGN */
			vec |= sc->sc_ign;
			for (i = 0; &intrptr[i] <=
			    (int64_t *)&sc->sc_sysio->reserved_int_map &&
			    INTVEC(imap = intrptr[i]) != INTVEC(vec); i++)
				;
			if (INTVEC(imap) == INTVEC(vec)) {
				DPRINTF(SDB_INTR,
				    ("OBIO %lx IRQ as %lx in slot %d\n", 
				    vec, (long)imap, i));
				/* Register the map and clear intr registers */
				ih->ih_map = &intrptr[i];
				intrptr = (int64_t *)&sc->sc_sysio->scsi_clr_int;
				ih->ih_clr = &intrptr[i];
				/* Enable the interrupt */
				imap |= INTMAP_V
				    |(CPU_UPAID << INTMAP_TID_SHIFT);
				/* XXXX */
				*(ih->ih_map) = imap;
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
	ih->ih_ivec = 0;
	ih->ih_pil = ipl;
	ih->ih_pending = 0;

	intr_establish(ipl, level != IPL_VM, ih);
	return (ih);
}

static bus_dma_tag_t
sbus_alloc_dmatag(struct sbus_softc *sc)
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
	sdt->_dmamap_create = sbus_dmamap_create;
	PCOPY(_dmamap_destroy);
	sdt->_dmamap_load = iommu_dvmamap_load;
	PCOPY(_dmamap_load_mbuf);
	PCOPY(_dmamap_load_uio);
	sdt->_dmamap_load_raw = iommu_dvmamap_load_raw;
	sdt->_dmamap_unload = iommu_dvmamap_unload;
	sdt->_dmamap_sync = iommu_dvmamap_sync;
	sdt->_dmamem_alloc = iommu_dvmamem_alloc;
	sdt->_dmamem_free = iommu_dvmamem_free;
	sdt->_dmamem_map = iommu_dvmamem_map;
	sdt->_dmamem_unmap = iommu_dvmamem_unmap;
	PCOPY(_dmamem_mmap);
#undef	PCOPY
	sc->sc_dmatag = sdt;
	return (sdt);
}

static int
sbus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
	bus_size_t maxsegsz, bus_size_t boundary, int flags,
	bus_dmamap_t *dmamp)
{
	struct sbus_softc *sc = t->_cookie;
	int error;

	error = bus_dmamap_create(t->_parent, size, nsegments, maxsegsz,
				  boundary, flags, dmamp);
	if (error == 0)
		(*dmamp)->_dm_cookie = &sc->sc_sb;
	return error;
}
