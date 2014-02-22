/* $NetBSD: tsc.c,v 1.24 2014/02/22 18:42:47 martin Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: tsc.c,v 1.24 2014/02/22 18:42:47 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/sysarch.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#include "tsciic.h"

#ifdef DEC_6600
#include <alpha/pci/pci_6600.h>
#endif

#define tsc() { Generate ctags(1) key. }

static int tscmatch(device_t, cfdata_t, void *);
static void tscattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tsc, 0, tscmatch, tscattach, NULL, NULL);

extern struct cfdriver tsc_cd;

struct tsp_config tsp_configuration[4];

static int tscprint(void *, const char *pnp);

static int tspmatch(device_t, cfdata_t, void *);
static void tspattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tsp, 0, tspmatch, tspattach, NULL, NULL);

extern struct cfdriver tsp_cd;

static int tsp_bus_get_window(int, int,
	struct alpha_bus_space_translation *);

static int tsciicprint(void *, const char *pnp);

static int tsciicmatch(device_t, cfdata_t, void *);
static void tsciicattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tsciic, sizeof(struct tsciic_softc), tsciicmatch,
    tsciicattach, NULL, NULL);

#if NTSCIIC
extern struct cfdriver tsciic_cd;
#endif

/* There can be only one */
static int tscfound;

/* Which hose is the display console connected to? */
int tsp_console_hose;

static int
tscmatch(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	switch (cputype) {
	case ST_DEC_6600:
	case ST_DEC_TITAN:
		return strcmp(ma->ma_name, tsc_cd.cd_name) == 0 && !tscfound;
	default:
		return 0;
	}
}

static void
tscattach(device_t parent, device_t self, void * aux)
{
	int i;
	int nbus;
	uint64_t csc, aar;
	struct tsp_attach_args tsp;
	struct tsciic_attach_args tsciic;
	struct mainbus_attach_args *ma = aux;
	int titan = cputype == ST_DEC_TITAN;

	tscfound = 1;

	csc = LDQP(TS_C_CSC);

	nbus = 1 + (CSC_BC(csc) >= 2);
	printf(": 2127%c Core Logic Chipset, Cchip rev %d\n"
		"%s%d: %c Dchips, %d memory bus%s of %d bytes\n",
		titan ? '4' : '2', (int)MISC_REV(LDQP(TS_C_MISC)),
		ma->ma_name, ma->ma_slot, "2448"[CSC_BC(csc)],
		nbus, nbus > 1 ? "es" : "", 16 + 16 * ((csc & CSC_AW) != 0));
	printf("%s%d: arrays present: ", ma->ma_name, ma->ma_slot);
	for (i = 0; i < 4; ++i) {
		aar = LDQP(TS_C_AAR0 + i * TS_STEP);
		printf("%s%dMB%s", i ? ", " : "", (8 << AAR_ASIZ(aar)) & ~0xf,
		    aar & AAR_SPLIT ? " (split)" : "");
	}
	printf(", Dchip 0 rev %d\n", (int)LDQP(TS_D_DREV) & 0xf);

	memset(&tsp, 0, sizeof tsp);
	tsp.tsp_name = "tsp";
	tsp.tsp_slot = 0;

	config_found(self, &tsp, tscprint);
	if (titan) {
		tsp.tsp_slot += 2;
		config_found(self, &tsp, tscprint);
	}

	if (csc & CSC_P1P) {
		tsp.tsp_slot = 1;
		config_found(self, &tsp, tscprint);
		if (titan) {
			tsp.tsp_slot += 2;
			config_found(self, &tsp, tscprint);
		}
	}

	memset(&tsciic, 0, sizeof tsciic);
	tsciic.tsciic_name = "tsciic";

	config_found(self, &tsciic, tsciicprint);
}

static int
tscprint(void *aux, const char *p)
{
	struct tsp_attach_args *tsp = aux;

	if (p)
		aprint_normal("%s%d at %s", tsp->tsp_name, tsp->tsp_slot, p);
	return UNCONF;
}

static int
tsciicprint(void *aux, const char *p)
{
	struct tsciic_attach_args *tsciic = aux;

	if (p)
		aprint_normal("%s at %s\n", tsciic->tsciic_name, p);
	else
		aprint_normal("\n");
	return UNCONF;
}

#define tsp() { Generate ctags(1) key. }

static int
tspmatch(device_t parent, cfdata_t match, void *aux)
{
	struct tsp_attach_args *t = aux;

	switch (cputype) {
	case ST_DEC_6600:
	case ST_DEC_TITAN:
		return strcmp(t->tsp_name, tsp_cd.cd_name) == 0;
	default:
		return 0;
	}
}

static void
tspattach(device_t parent, device_t self, void *aux)
{
	struct pcibus_attach_args pba;
	struct tsp_attach_args *t = aux;
	struct tsp_config *pcp;

	printf("\n");
	pcp = tsp_init(1, t->tsp_slot);

	tsp_dma_init(pcp);

	/*
	 * Do PCI memory initialization that needs to be deferred until
	 * malloc is safe.  On the Tsunami, we need to do this after
	 * DMA is initialized, as well.
	 */
	tsp_bus_mem_init2(&pcp->pc_memt, pcp);

	pci_6600_pickintr(pcp);

	pba.pba_iot = &pcp->pc_iot;
	pba.pba_memt = &pcp->pc_memt;
	pba.pba_dmat =
	    alphabus_dma_get_tag(&pcp->pc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &pcp->pc_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

struct tsp_config *
tsp_init(int mallocsafe, int n)
	/* n:	 Pchip number */
{
	struct tsp_config *pcp;
	int titan = cputype == ST_DEC_TITAN;

	KASSERT(n >= 0 && n < __arraycount(tsp_configuration));
	pcp = &tsp_configuration[n];
	pcp->pc_pslot = n;
	pcp->pc_iobase = TS_Pn(n, 0);
	pcp->pc_csr = S_PAGE(TS_Pn(n & 1, P_CSRBASE));
	if (n & 2) {
		/* `A' port of PA Chip */
		pcp->pc_csr++;
	}
	if (titan) {
		/* same address on G and A ports */
		pcp->pc_tlbia = &pcp->pc_csr->port.g.tsp_tlbia.tsg_r;
	} else {
		pcp->pc_tlbia = &pcp->pc_csr->port.p.tsp_tlbia.tsg_r;
	}

	if (!pcp->pc_initted) {
		tsp_bus_io_init(&pcp->pc_iot, pcp);
		tsp_bus_mem_init(&pcp->pc_memt, pcp);

		alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_IO] = 1;
		alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_MEM] = 1;

		alpha_bus_get_window = tsp_bus_get_window;
	}
	pcp->pc_mallocsafe = mallocsafe;
	tsp_pci_init(&pcp->pc_pc, pcp);
	pcp->pc_initted = 1;
	return pcp;
}

static int
tsp_bus_get_window(int type, int window,
    struct alpha_bus_space_translation *abst)
{
	struct tsp_config *tsp = &tsp_configuration[tsp_console_hose];
	bus_space_tag_t st;
	int error;

	switch (type) {
	case ALPHA_BUS_TYPE_PCI_IO:
		st = &tsp->pc_iot;
		break;

	case ALPHA_BUS_TYPE_PCI_MEM:
		st = &tsp->pc_memt;
		break;

	default:
		panic("tsp_bus_get_window");
	}

	error = alpha_bus_space_get_window(st, window, abst);
	if (error)
		return error;

	abst->abst_sys_start = TS_PHYSADDR(abst->abst_sys_start);
	abst->abst_sys_end = TS_PHYSADDR(abst->abst_sys_end);

	return 0;
}

#define tsciic() { Generate ctags(1) key. }

static int
tsciicmatch(device_t parent, cfdata_t match, void *aux)
{
#if NTSCIIC
	struct tsciic_attach_args *t = aux;
#endif

	switch (cputype) {
	case ST_DEC_6600:
	case ST_DEC_TITAN:
#if NTSCIIC
		return strcmp(t->tsciic_name, tsciic_cd.cd_name) == 0;
#endif
	default:
		return 0;
	}
}

static void
tsciicattach(device_t parent, device_t self, void *aux)
{
#if NTSCIIC
	tsciic_init(self);
#endif
}

void
tsc_print_dir(unsigned int indent, unsigned long dir)
{
	char buf[60];

	snprintb(buf, 60,
		 "\177\20"
		 "b\77Internal Cchip asynchronous error\0"
		 "b\76Pchip 0 error\0"
		 "b\75Pchip 1 error\0"
		 "b\74Pchip 2 error\0"
		 "b\73Pchip 3 error\0",
		 dir);
	IPRINTF(indent, "DIR = %s\n", buf);
}

void
tsc_print_misc(unsigned int indent, unsigned long misc)
{
	unsigned long tmp = MISC_NXM_SRC(misc);

	if (!MISC_NXM(misc))
		return;

	IPRINTF(indent, "NXM address detected\n");
	IPRINTF(indent, "NXM source         = %s %lu\n",
		tmp <= 3 ? "CPU" : "Pchip", tmp <= 3 ? tmp : tmp - 4);
}
