/*	$NetBSD: obio.c,v 1.39 1998/01/25 19:44:43 pk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1993, 1994 Theo de Raadt
 * Copyright (c) 1995, 1997 Paul Kranenburg
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Theo de Raadt.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/oldmon.h>
#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/dev/sbusvar.h>

struct bus_softc {
	union {
		struct	device scu_dev;		/* base device */
		struct	sbus_softc scu_sbus;	/* obio is another sbus slot */
	} bu;
};


/* autoconfiguration driver */
int		obioprint    __P((void *, const char *));
static int	obiomatch    __P((struct device *, struct cfdata *, void *));
static void	obioattach   __P((struct device *, struct device *, void *));

#if defined(SUN4)
static void	obioattach4  __P((struct device *, struct device *, void *));
int		obiosearch   __P((struct device *, struct cfdata *, void *));
#endif
#if defined(SUN4M)
static void	obioattach4m __P((struct device *, struct device *, void *));
#endif

struct cfattach obio_ca = {
	sizeof(struct bus_softc), obiomatch, obioattach
};

int
obiomatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	return (strcmp(cf->cf_driver->cd_name, ra->ra_name) == 0);
}

int
obioprint(args, name)
	void *args;
	const char *name;
{
	register struct confargs *ca = args;

	if (ca->ca_ra.ra_name == NULL)
		ca->ca_ra.ra_name = "<unknown>";

	if (name)
		printf("[%s at %s]", ca->ca_ra.ra_name, name);

	printf(" addr %p", ca->ca_ra.ra_paddr);

	return (UNCONF);
}

void
obioattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
#if defined(SUN4)
	if (CPU_ISSUN4)
		obioattach4(parent, self, args);
#endif
#if defined(SUN4M)
	if (CPU_ISSUN4M)
		obioattach4m(parent, self, args);
#endif
}

#if defined(SUN4)
void
obioattach4(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	printf("\n");
	(void)config_search(obiosearch, self, args);
	obio_bus_untmp();
}
#endif

#if defined(SUN4M)
void
obioattach4m(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	register struct bus_softc *sc = (struct bus_softc *)self;
	struct confargs oca, *ca = args;
	register struct romaux *ra = &ca->ca_ra;
	register int node0, node;
	register char *name;
	register const char *sp;
	const char *const *ssp;
	int rlen;
	extern int autoconf_nzs;

	static const char *const special4m[] = {
		/* find these first */
		"eeprom",
		"counter",
#if 0 /* Not all sun4m's have an `auxio' */
		"auxio",
#endif
		"",
		/* place device to ignore here */
		"interrupt",
		NULL
	};

	/*
	 * There is only one obio bus (it is in fact one of the Sbus slots)
	 */
	if (self->dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	printf("\n");

	if (ra->ra_bp != NULL && strcmp(ra->ra_bp->name, "obio") == 0)
		oca.ca_ra.ra_bp = ra->ra_bp + 1;
	else
		oca.ca_ra.ra_bp = NULL;

	node = ra->ra_node;
	rlen = getproplen(node, "ranges");
	if (rlen > 0) {
		sc->bu.scu_sbus.sc_nrange = rlen / sizeof(struct rom_range);
		sc->bu.scu_sbus.sc_range =
			(struct rom_range *)malloc(rlen, M_DEVBUF, M_NOWAIT);
		if (sc->bu.scu_sbus.sc_range == 0)
			panic("obio: PROM ranges too large: %d", rlen);
		(void)getprop(node, "ranges", sc->bu.scu_sbus.sc_range, rlen);
	}

	/*
	 * Loop through ROM children, fixing any relative addresses
	 * and then configuring each device.
	 * We first do the crucial ones, such as eeprom, etc.
	 */
	node0 = firstchild(ra->ra_node);
	for (ssp = special4m ; *(sp = *ssp) != 0; ssp++) {
		if ((node = findnode(node0, sp)) == 0) {
			printf("could not find %s amongst obio devices\n", sp);
			panic(sp);
		}
		if (!romprop(&oca.ca_ra, sp, node))
			continue;

		sbus_translate(self, &oca);
		oca.ca_bustype = BUS_OBIO;
		(void) config_found(self, (void *)&oca, obioprint);
	}

	for (node = node0; node; node = nextsibling(node)) {
		name = getpropstring(node, "name");
		for (ssp = special4m ; (sp = *ssp) != NULL; ssp++)
			if (strcmp(name, sp) == 0)
				break;

		if (sp != NULL || !romprop(&oca.ca_ra, name, node))
			continue;

		if (strcmp(name, "zs") == 0)
			/* XXX - see autoconf.c for this hack */
			autoconf_nzs++;

		/* Translate into parent address spaces */
		sbus_translate(self, &oca);
		oca.ca_bustype = BUS_OBIO;
		(void) config_found(self, (void *)&oca, obioprint);
	}
}
#endif


#if defined(SUN4)
int
obiosearch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	register struct confargs *ca = args;
	struct confargs oca;
	caddr_t tmp;


	/*
	 * Avoid sun4m entries which don't have valid PAs.
	 * no point in even probing them. 
	 */
	if (cf->cf_loc[0] == -1)
		return (0);

	/*
	 * On the 4/100 obio addresses must be mapped at
	 * 0x0YYYYYYY, but alias higher up (we avoid the
	 * alias condition because it causes pmap difficulties)
	 * XXX: We also assume that 4/[23]00 obio addresses
	 * must be 0xZYYYYYYY, where (Z != 0)
	 */
	if (cpuinfo.cpu_type == CPUTYP_4_100 &&
	    (cf->cf_loc[0] & 0xf0000000))
		return (0);
	if (cpuinfo.cpu_type != CPUTYP_4_100 &&
	    !(cf->cf_loc[0] & 0xf0000000))
		return (0);

	oca.ca_ra.ra_paddr = (void *)cf->cf_loc[0];
	oca.ca_ra.ra_len = 0;
	oca.ca_ra.ra_nreg = 1;
	oca.ca_ra.ra_iospace = PMAP_OBIO;

	if (oca.ca_ra.ra_paddr)
		tmp = (caddr_t)mapdev(oca.ca_ra.ra_reg, TMPMAP_VA, 0, NBPG);
	else
		tmp = NULL;
	oca.ca_ra.ra_vaddr = tmp;
	oca.ca_ra.ra_intr[0].int_pri = cf->cf_loc[1];
	oca.ca_ra.ra_intr[0].int_vec = -1;
	oca.ca_ra.ra_nintr = 1;
	oca.ca_ra.ra_name = cf->cf_driver->cd_name;

	if (ca->ca_ra.ra_bp != NULL &&
	    strcmp(ca->ca_ra.ra_bp->name, "obio") == 0)
		oca.ca_ra.ra_bp = ca->ca_ra.ra_bp + 1;
	else
		oca.ca_ra.ra_bp = NULL;

	oca.ca_bustype = BUS_OBIO;

	if ((*cf->cf_attach->ca_match)(parent, cf, &oca) == 0)
		return (0);

	/*
	 * Check if XXmatch routine replaced the temporary mapping with
	 * a real mapping.   If not, then make sure we don't pass the
	 * tmp mapping to the attach routine.
	 */
	if (oca.ca_ra.ra_vaddr == tmp)
		oca.ca_ra.ra_vaddr = NULL; /* wipe out tmp address */
	/*
	 * the match routine will set "ra_len" if it wants us to
	 * establish a mapping for it.
	 * (which won't be seen on future XXmatch calls,
	 * so not as useful as it seems.)
	 */
	if (oca.ca_ra.ra_len)
		oca.ca_ra.ra_vaddr =
		    obio_bus_map(oca.ca_ra.ra_reg, oca.ca_ra.ra_len);

	config_attach(parent, cf, &oca, obioprint);
	return (1);
}

#define	getpte(va)		lda(va, ASI_PTE)

/*
 * If we can find a mapping that was established by the rom, use it.
 * Else, create a new mapping.
 */
void *
obio_bus_map(pa, len)
	struct rom_reg *pa;
	int len;
{

	if (CPU_ISSUN4 && len <= NBPG) {
		u_long	pf = (u_long)(pa->rr_paddr) >> PGSHIFT;
		int pgtype = PMAP_T2PTE_4(pa->rr_iospace);
		u_long	va, pte;

		for (va = OLDMON_STARTVADDR; va < OLDMON_ENDVADDR; va += NBPG) {
			pte = getpte(va);
			if ((pte & PG_V) != 0 && (pte & PG_TYPE) == pgtype &&
			    (pte & PG_PFNUM) == pf)
				return ((void *)
				    (va | ((u_long)pa->rr_paddr & PGOFSET)) );
					/* note: preserve page offset */
		}
	}

	return mapiodev(pa, 0, len);
}
#endif /* SUN4 */

void
obio_bus_untmp()
{
	pmap_remove(pmap_kernel(), TMPMAP_VA, TMPMAP_VA+NBPG);
}
