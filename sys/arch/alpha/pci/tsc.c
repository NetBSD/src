/* $NetBSD: tsc.c,v 1.1.14.1 1999/12/27 18:31:29 wrstuden Exp $ */

/*-
 * Copyright (c) 1999 by Ross Harvey.  All rights reserved.
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
 *	This product includes software developed by Ross Harvey.
 * 4. The name of Ross Harvey may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROSS HARVEY ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURP0SE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ROSS HARVEY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_dec_6600.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: tsc.c,v 1.1.14.1 1999/12/27 18:31:29 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#ifdef DEC_6600
#include <alpha/pci/pci_6600.h>
#endif

#define tsc() { Generate ctags(1) key. }

int	tscmatch __P((struct device *, struct cfdata *, void *));
void	tscattach __P((struct device *, struct device *, void *));

struct cfattach tsc_ca = {
	sizeof(struct tsc_softc), tscmatch, tscattach,
};

extern struct cfdriver tsc_cd;

struct tsp_config tsp_configuration[2];

static int tscprint __P((void *, const char *pnp));

int	tspmatch __P((struct device *, struct cfdata *, void *));
void	tspattach __P((struct device *, struct device *, void *));

struct cfattach tsp_ca = {
	sizeof(struct tsp_softc), tspmatch, tspattach,
};

extern struct cfdriver tsp_cd;

static int tspprint __P((void *, const char *pnp));

/* There can be only one */

static int tscfound;

int
tscmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return cputype == ST_DEC_6600
	    && strcmp(ma->ma_name, tsc_cd.cd_name) == 0
	    && !tscfound;
}

void tscattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int i;
	int nbus;
	u_int64_t csc, aar;
	struct tsp_attach_args tsp;
	struct mainbus_attach_args *ma = aux;

	tscfound = 1;

	csc = LDQP(TS_C_CSC);

	nbus = 1 + (CSC_BC(csc) >= 2);
	printf(": 21272 Core Logic Chipset, Cchip rev %d\n"
		"%s%d: %c Dchips, %d memory bus%s of %d bytes\n",
		(int)MISC_REV(LDQP(TS_C_MISC)),
		ma->ma_name, ma->ma_slot, "2448"[CSC_BC(csc)],
		nbus, nbus > 1 ? "es" : "", 16 + 16 * ((csc & CSC_AW) != 0));
	printf("%s%d: arrays present: ", ma->ma_name, ma->ma_slot);
	for(i = 0; i < 4; ++i) {
		aar = LDQP(TS_C_AAR0 + i * TS_STEP);
		printf("%s%dMB%s", i ? ", " : "", (8 << AAR_ASIZ(aar)) & ~0xf,
		    aar & AAR_SPLIT ? " (split)" : "");
	}
	printf(", Dchip 0 rev %d\n", (int)LDQP(TS_D_DREV) & 0xf);

	bzero(&tsp, sizeof tsp);
	tsp.tsp_name = "tsp";
	config_found(self, &tsp, NULL);

	if(LDQP(TS_C_CSC) & CSC_P1P) {
		++tsp.tsp_slot;
		config_found(self, &tsp, tscprint);
	}
}

static int
tscprint(aux, p)
	void *aux;
	const char *p;
{
	register struct tsp_attach_args *tsp = aux;

	if(p)
		printf("%s%d at %s", tsp->tsp_name, tsp->tsp_slot, p);
	return UNCONF;
}

#define tsp() { Generate ctags(1) key. }

int
tspmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tsp_attach_args *t = aux;

	return  cputype == ST_DEC_6600
	    && strcmp(t->tsp_name, tsp_cd.cd_name) == 0;
}

void tspattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibus_attach_args pba;
	struct tsp_attach_args *t = aux;
	struct tsp_config *pcp;

	printf("\n");
	pcp = tsp_init(1, t->tsp_slot);
	tsp_dma_init(pcp);
	pci_6600_pickintr(pcp);
	pba.pba_busname = "pci";
	pba.pba_iot = &pcp->pc_iot;
	pba.pba_memt = &pcp->pc_memt;
	pba.pba_dmat =
	    alphabus_dma_get_tag(&pcp->pc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &pcp->pc_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	config_found(self, &pba, tspprint);
}

struct tsp_config *
tsp_init(mallocsafe, n)
	int mallocsafe;
	int n;	/* Pchip number */
{
	struct tsp_config *pcp;

	KASSERT((n | 1) == 1);
	pcp = &tsp_configuration[n];
	pcp->pc_pslot = n;
	pcp->pc_iobase = TS_Pn(n, 0);
	pcp->pc_csr = S_PAGE(TS_Pn(n, P_CSRBASE));
	if (!pcp->pc_initted) {
		tsp_bus_io_init(&pcp->pc_iot, pcp);
		tsp_bus_mem_init(&pcp->pc_memt, pcp);
	}
	pcp->pc_mallocsafe = mallocsafe;
	tsp_pci_init(&pcp->pc_pc, pcp);
	pcp->pc_initted = 1;
	return pcp;
}

static int
tspprint(aux, p)
	void *aux;
	const char *p;
{
	register struct pcibus_attach_args *pci = aux;

	if(p)
		printf("%s at %s", pci->pba_busname, p);
	printf(" bus %d", pci->pba_bus);
	return UNCONF;
}
