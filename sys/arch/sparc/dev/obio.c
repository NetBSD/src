/*	$NetBSD: obio.c,v 1.3 1994/10/02 22:00:29 deraadt Exp $	*/

/*
 * Copyright (c) 1993, 1994 Theo de Raadt
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
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/vaddrs.h>

struct obio_softc {
	struct	device sc_dev;		/* base device */
	int	nothing;
};

/* autoconfiguration driver */
static int	obiomatch(struct device *, struct cfdata *, void *);
static void	obioattach(struct device *, struct device *, void *);
struct cfdriver obiocd = { NULL, "obio", obiomatch, obioattach,
	DV_DULL, sizeof(struct obio_softc)
};

void *		obio_map __P((void *, int));
void *		obio_tmp_map __P((void *));
void		obio_tmp_unmap __P((void));

int
obiomatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct confargs *ca = aux;
	register struct romaux *ra = &ca->ca_ra;

	if (cputyp != CPU_SUN4)
		return (0);
	return (strcmp(cf->cf_driver->cd_name, ra->ra_name) == 0);
}

int
obio_print(args, obio)
	void *args;
	char *obio;
{
	register struct confargs *ca = args;

	if (ca->ca_ra.ra_name == NULL)
		ca->ca_ra.ra_name = "<unknown>";
	if (obio)
		printf("[%s at %s]", ca->ca_ra.ra_name, obio);
	printf(" addr %x", ca->ca_ra.ra_paddr);
	return (UNCONF);
}

void
obioattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	register struct obio_softc *sc = (struct obio_softc *)self;
	extern struct cfdata cfdata[];
	register struct confargs *ca = args;
	struct confargs oca;
	register short *p;
	struct cfdata *cf;
	caddr_t tmpmap;

	printf("\n");

	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	for (cf = cfdata; cf->cf_driver; cf++) {
		if (cf->cf_fstate == FSTATE_FOUND)
			continue;
		for (p = cf->cf_parents; *p >= 0; p++)
			if (self->dv_cfdata == &cfdata[*p]) {
				oca.ca_ra.ra_iospace = -1;
				oca.ca_ra.ra_paddr = (void *)cf->cf_loc[0];
				oca.ca_ra.ra_len = 0;
				tmpmap = NULL;
				if (oca.ca_ra.ra_paddr)
					tmpmap = obio_tmp_map(oca.ca_ra.ra_paddr);
				oca.ca_ra.ra_vaddr = tmpmap;
				oca.ca_ra.ra_intr[0].int_pri = cf->cf_loc[1];
				oca.ca_ra.ra_intr[0].int_vec = 0;
				oca.ca_ra.ra_nintr = 1;
				oca.ca_ra.ra_name = cf->cf_driver->cd_name;
				oca.ca_ra.ra_bp = ca->ca_ra.ra_bp;
				oca.ca_bustype = BUS_OBIO;

				if ((*cf->cf_driver->cd_match)(self, cf, &oca) == 0)
					continue;

				/*
				 * check if XXmatch routine replaced the
				 * temporary mapping with a real mapping.
				 */
				if (tmpmap == oca.ca_ra.ra_vaddr)
					oca.ca_ra.ra_vaddr = NULL;
				/*
				 * or if it has asked us to create a mapping..
				 * (which won't be seen on future XXmatch calls,
				 * so not as useful as it seems.)
				 */
				if (oca.ca_ra.ra_len)
					oca.ca_ra.ra_vaddr =
					    obio_map(oca.ca_ra.ra_paddr,
					    oca.ca_ra.ra_len);
			
				config_attach(self, cf, &oca, obio_print);
			}
	}
	obio_tmp_unmap();
}

#define	getpte(va)		lda(va, ASI_PTE)

/*
 * If we can find a mapping that was established by the rom, use it.
 * Else, create a new mapping.
 */
void *
obio_map(pa, len)
	void *pa;
	int len;
{
	u_long	pf = (u_long)pa >> PGSHIFT;
	u_long	va, pte;

	if (len <= NBPG) {
		for (va = OLDMON_STARTVADDR; va < OLDMON_ENDVADDR; va += NBPG) {
			pte = getpte(va);
			if ((pte & PG_V) != 0 && (pte & PG_TYPE) == PG_OBIO &&
			    (pte & PG_PFNUM) == pf)
				return ((void *)va);
		}
	}
	return mapiodev(pa, len);
}

void *
obio_tmp_map(pa)
	void *pa;
{
	vm_offset_t addr = (vm_offset_t)pa & ~PGOFSET;

	pmap_enter(kernel_pmap, TMPMAP_VA,
	    addr | PMAP_OBIO | PMAP_NC,
	    VM_PROT_READ | VM_PROT_WRITE, 1);
	return ((void *)TMPMAP_VA);
}

void
obio_tmp_unmap()
{
	pmap_remove(kernel_pmap, TMPMAP_VA, TMPMAP_VA+NBPG);
}
