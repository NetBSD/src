/*	$NetBSD: obio.c,v 1.2 1994/09/17 23:49:58 deraadt Exp $	*/

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
struct cfdriver obiocd = { NULL, "cgsix", obiomatch, obioattach,
	DV_DULL, sizeof(struct obio_softc)
};

void *		obio_map __P((void *, int));
void *		obio_tmp_map __P((void *));
void		obio_tmp_unmap __P((void));

int
obiomatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	/*
	 * This exists for machines that don't have OpenPROM.
	 */
	if (cputyp != CPU_SUN4)
		return 0;
	return 1;
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
		printf("%s at %s", ca->ca_ra.ra_name, obio);
	printf(" slot %d offset 0x%x", ca->ca_slot, ca->ca_offset);
	return (UNCONF);
}

void
obioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct obio_softc *sc = (struct obio_softc *)self;
	extern struct cfdata cfdata[];
	struct confargs ca;
	register short *p;
	struct cfdata *cf;

	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	for (cf = cfdata; cf->cf_driver; cf++) {
		if (cf->cf_fstate == FSTATE_FOUND)
			continue;
		for (p = cf->cf_parents; *p >= 0; p++)
			if (parent->dv_cfdata == &cfdata[*p]) {
				ca.ca_ra.ra_iospace = -1;
				ca.ca_ra.ra_paddr = (void *)cf->cf_loc[0];
				ca.ca_ra.ra_len = 0;
				ca.ca_ra.ra_vaddr = obio_tmp_map(ca.ca_ra.ra_paddr);
				ca.ca_ra.ra_intr[0].int_pri = cf->cf_loc[1];
				ca.ca_ra.ra_intr[0].int_vec = 0;
				ca.ca_ra.ra_nintr = 1;
				if ((*cf->cf_driver->cd_match)(self, cf, &ca) == 0)
					continue;

				if (ca.ca_ra.ra_len)
					ca.ca_ra.ra_vaddr =
					    obio_map(ca.ca_ra.ra_paddr,
					    ca.ca_ra.ra_len);
				ca.ca_bustype = BUS_OBIO;
				config_attach(self, cf, &ca, NULL);
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
	u_long	pf = (int)pa >> PGSHIFT;
	u_long	va, pte;

	for (va = MONSTART; va < MONEND; va += NBPG) {
		pte = getpte(va);
		if ((pte & PG_V) != 0 && (pte & PG_TYPE) == PG_OBIO &&
		    (pte & PG_PFNUM) == pf)
			return ((void *)va);
	}
	return mapiodev(pa, len);
}

void *
obio_tmp_map(pa)
	void *pa;
{
	pmap_enter(kernel_pmap, TMPMAP_VA,
	    (vm_offset_t)pa | PMAP_OBIO | PMAP_NC,
	    VM_PROT_READ | VM_PROT_WRITE, 1);
	return ((void *)TMPMAP_VA);
}

void
obio_tmp_unmap()
{
	pmap_remove(kernel_pmap, TMPMAP_VA, TMPMAP_VA+NBPG);
}
