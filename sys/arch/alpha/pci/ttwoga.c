/* $NetBSD: ttwoga.c,v 1.14.2.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include "opt_dec_2100_a500.h"
#include "opt_dec_2100a_a500.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ttwoga.c,v 1.14.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/eisa/eisavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
#include <alpha/pci/pci_2100_a500.h>
#endif

#include "locators.h"

int	ttwogamatch(device_t, cfdata_t, void *);
void	ttwogaattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ttwoga, 0,
    ttwogamatch, ttwogaattach, NULL, NULL);

int	ttwogaprint(void *, const char *);

int	ttwopcimatch(device_t, cfdata_t, void *);
void	ttwopciattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ttwopci, 0,
    ttwopcimatch, ttwopciattach, NULL, NULL);

int	ttwosableioprint(void *, const char *);

/*
 * There can be only one, but it might have 2 primary PCI busses.
 */
int ttwogafound;
struct ttwoga_config ttwoga_configuration[2];

/* CBUS address bias for Gamma systems. */
bus_addr_t ttwoga_gamma_cbus_bias;

#define	GIGABYTE	(1024UL * 1024UL * 1024UL)
#define	MEGABYTE	(1024UL * 1024UL)

const struct ttwoga_sysmap ttwoga_sysmap[2] = {
/*	  Base			System size	Bus size	*/
	{ T2_PCI0_SIO_BASE,	256UL * MEGABYTE, 8UL * MEGABYTE,
	  T2_PCI0_SMEM_BASE,	4UL * GIGABYTE,	128UL * MEGABYTE,
	  T2_PCI0_DMEM_BASE,	1UL * GIGABYTE,	1UL * GIGABYTE,
	  T2_PCI0_CONF_BASE,
	},
	{ T2_PCI1_SIO_BASE,	256UL * MEGABYTE, 8UL * MEGABYTE,
	  T2_PCI1_SMEM_BASE,	2UL * GIGABYTE,	64UL * MEGABYTE,
	  T2_PCI1_DMEM_BASE,	2UL * GIGABYTE,	2UL * GIGABYTE,
	  T2_PCI1_CONF_BASE,
	},
};

#undef GIGABYTE
#undef MEGABYTE

int
ttwogamatch(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* Make sure that we're looking for a T2 Gate Array. */
	if (strcmp(ma->ma_name, match->cf_name) != 0)
		return (0);

	if (ttwogafound)
		return (0);

	return (1);
}

void
ttwogaattach(device_t parent, device_t self, void *aux)
{
	struct pcibus_attach_args pba;
	int hose;

	printf("\n");

	/* note that we've attached the chipset; can't have 2 T2s */
	ttwogafound = 1;

	/*
	 * Don't do much here; real work is deferred to the ttwopci
	 * layer.
	 */
	for (hose = 0; hose < 1 /* XXX */; hose++) {
#if 0 /* XXX */
		if (badaddr(_T2GA(hose, T2_IOCSR), sizeof(uint64_t)))
			continue;
#endif
		memset(&pba, 0, sizeof(pba));
		pba.pba_bus = hose;

		(void) config_found_ia(self, "ttwoga", &pba, ttwogaprint);
	}
}

int
ttwogaprint(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		aprint_normal("ttwopci at %s", pnp);
	aprint_normal(" hose %d", pba->pba_bus);
	return (UNCONF);
}

/*
 * Set up the chipset's function pointers.
 */
struct ttwoga_config *
ttwoga_init(int hose, int mallocsafe)
{
	struct ttwoga_config *tcp;

	if (hose < 0 || hose > 1)
		panic("ttwoga_init: bad hose # %d", hose);

	tcp = &ttwoga_configuration[hose];
	tcp->tc_hose = hose;

	tcp->tc_sysmap = &ttwoga_sysmap[hose];

	/*
	 * Sable-Gamma has a CBUS address bias.
	 */
	ttwoga_gamma_cbus_bias = (alpha_implver() == ALPHA_IMPLVER_EV5) ?
	    T2_GAMMA_CBUS_BIAS : 0;

	alpha_mb();

	tcp->tc_io_bus_start = (T2GA(tcp, T2_HAE0_2) & HAE0_2_PUA2) << 23;
	tcp->tc_d_mem_bus_start = (T2GA(tcp, T2_HAE0_4) & HAE0_4_PUA1) << 30;
	tcp->tc_s_mem_bus_start = (T2GA(tcp, T2_HAE0_1) & HAE0_1_PUA1) << 27;

	tcp->tc_rev  = (T2GA(tcp, T2_IOCSR) & IOCSR_TRN) >> IOCSR_TRN_SHIFT;

	if (tcp->tc_initted == 0) {
		/* don't do these twice since they set up extents */
		ttwoga_bus_io_init(&tcp->tc_iot, tcp);
		ttwoga_bus_mem_init(&tcp->tc_memt, tcp);
	}
	tcp->tc_mallocsafe = mallocsafe;

	ttwoga_pci_init(&tcp->tc_pc, tcp);

	tcp->tc_initted = 1;

	return (tcp);
}

int
ttwopcimatch(device_t parent, cfdata_t match, void *aux)
{
	struct pcibus_attach_args *pba = aux;

	if (match->cf_loc[PCIBUSCF_BUS] != PCIBUSCF_BUS_DEFAULT &&
	    match->cf_loc[PCIBUSCF_BUS] != pba->pba_bus)
		return (0);

	return (1);
}

void
ttwopciattach(device_t parent, device_t self, void *aux)
{
	struct pcibus_attach_args *pba = aux, npba;
	struct ttwoga_config *tcp;

	/*
	 * set up the chipset's info; done one at console init time
	 * (maybe), but doesn't hurt to do it twice.
	 */
	tcp = ttwoga_init(pba->pba_bus, 1);

	ttwoga_dma_init(tcp);

	aprint_normal(": %s Gate Array rev. %d\n",
	    (tcp->tc_rev < TRN_T3) ? "T2" : "T3 or T4",
	    tcp->tc_rev);

	if (tcp->tc_rev < 1)
		aprint_normal_dev(self, "WARNING: T2 NOT PASS2... NO BETS...\n");

	switch (cputype) {
#if defined(DEC_2100_A500) || defined(DEC_2100A_A500)
	case ST_DEC_2100_A500:
	case ST_DEC_2100A_A500:
		pci_2100_a500_pickintr(tcp);
		break;
#endif
	
	default:
		panic("ttwogaattach: shouldn't be here, really...");
	}

	npba.pba_iot = &tcp->tc_iot;
	npba.pba_memt = &tcp->tc_memt;
	npba.pba_dmat =
	    alphabus_dma_get_tag(&tcp->tc_dmat_direct, ALPHA_BUS_PCI);
	npba.pba_dmat64 = NULL;
	npba.pba_pc = &tcp->tc_pc;
	npba.pba_bus = 0;
	npba.pba_bridgetag = NULL;
	npba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;

	/*
	 * Hose 0 has the STDIO module.
	 */
	if (pba->pba_bus == 0) {
		(void) config_found_ia(self, "sableiobus", &npba,
				       ttwosableioprint);
	}

	(void) config_found_ia(self, "pcibus", &npba, pcibusprint);
}

int
ttwosableioprint(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		aprint_normal("sableio at %s", pnp);
	aprint_normal(" bus %d", pba->pba_bus);
	return (UNCONF);
}
