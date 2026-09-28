/*	$NetBSD: ebc.c,v 1.1 2026/09/28 20:41:01 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * PPC 4xx External Bus Controller.
 *
 * Currently depends on the firmware programming EBC0_BnCR registers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ebc.c,v 1.1 2026/09/28 20:41:01 rkujawa Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/tlb.h>
#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/dev/ebcvar.h>

#define	EBC_NBANK_MAX		8

/* EBC0_BnCR */
#define	EBC_BCR_BAS		0xfff00000	/* base address */
#define	EBC_BCR_BS		0x000e0000	/* bank size, 1MB << n */
#define	EBC_BCR_BS_SHIFT	17
#define	EBC_BCR_BU		0x00018000	/* bank usage */
#define	EBC_BCR_BU_SHIFT	15
#define	EBC_BCR_BW		0x00006000	/* bus width, 8 << n */
#define	EBC_BCR_BW_SHIFT	13

struct ebc_softc {
	device_t		sc_dev;
	struct powerpc_bus_space sc_bst;
	char			sc_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
				    __aligned(8);
	int			sc_nbank;
};

static int	ebc_match(device_t, cfdata_t, void *);
static void	ebc_attach(device_t, device_t, void *);
static int	ebc_print(void *, const char *);
static uint32_t	ebc_read_bcr(int);

CFATTACH_DECL_NEW(ebc, sizeof(struct ebc_softc),
    ebc_match, ebc_attach, NULL, NULL);

static int
ebc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct plb_attach_args * const paa = aux;

	return strcmp(paa->plb_name, cf->cf_name) == 0;
}

static void
ebc_attach(device_t parent, device_t self, void *aux)
{
	struct ebc_softc * const sc = device_private(self);
	struct plb_attach_args * const paa = aux;
	struct ebc_attach_args eaa;
	int locs[EBCCF_NLOCS];
	int bank;

	sc->sc_dev = self;

	switch (mfpvr() >> 16) {
	case AMCC460EX:
		sc->sc_nbank = 6;
		break;
	default:
		sc->sc_nbank = EBC_NBANK_MAX;
		break;
	}

	aprint_naive("\n");
	aprint_normal(": External Bus Controller, %d banks\n", sc->sc_nbank);

	sc->sc_bst.pbs_flags = _BUS_SPACE_BIG_ENDIAN | _BUS_SPACE_MEM_TYPE;
	sc->sc_bst.pbs_base = 0;
	sc->sc_bst.pbs_limit = 0xffffffff;
	if (bus_space_init(&sc->sc_bst, "ebc", sc->sc_ex_storage,
	    sizeof(sc->sc_ex_storage)) != 0) {
		aprint_error_dev(self, "can't init bus space\n");
		return;
	}

	for (bank = 0; bank < sc->sc_nbank; bank++) {
		const uint32_t bcr = ebc_read_bcr(bank);

		eaa.ebc_usage = __SHIFTOUT(bcr, EBC_BCR_BU);
		if (eaa.ebc_usage == EBC_USAGE_DISABLED)
			continue;

		eaa.ebc_bt = &sc->sc_bst;
		eaa.ebc_dmat = paa->plb_dmat;
		eaa.ebc_bank = bank;
		eaa.ebc_addr = bcr & EBC_BCR_BAS;
		eaa.ebc_size = 0x00100000 << __SHIFTOUT(bcr, EBC_BCR_BS);
		eaa.ebc_width = 8 << __SHIFTOUT(bcr, EBC_BCR_BW);

		aprint_verbose_dev(self, "bank %d: 0x%08" PRIxBUSADDR
		    "-0x%08" PRIxBUSADDR ", %d-bit, %s\n",
		    bank, eaa.ebc_addr, eaa.ebc_addr + eaa.ebc_size - 1,
		    eaa.ebc_width,
		    eaa.ebc_usage == EBC_USAGE_RW ? "r/w" :
		    eaa.ebc_usage == EBC_USAGE_RO ? "r/o" : "w/o");

		/* Refuse banks we didn't pin. */
		if (ppc4xx_tlb_mapiodev(eaa.ebc_addr, PAGE_SIZE) == NULL) {
			aprint_verbose_dev(self,
			    "bank %d: no reserved mapping, skipped\n", bank);
			continue;
		}

		locs[EBCCF_BANK] = bank;
		config_found(self, &eaa, ebc_print,
		    CFARGS(.submatch = config_stdsubmatch,
			   .locators = locs));
	}
}

static int
ebc_print(void *aux, const char *pnp)
{
	struct ebc_attach_args * const eaa = aux;

	if (pnp != NULL)
		aprint_normal("device at %s", pnp);
	aprint_normal(" bank %d addr 0x%08" PRIxBUSADDR, eaa->ebc_bank,
	    eaa->ebc_addr);

	return UNCONF;
}

static uint32_t
ebc_read_bcr(int bank)
{

	mtdcr(DCR_EBC0_CFGADDR, DCR_EBC0_B0CR + bank);
	return mfdcr(DCR_EBC0_CFGDATA);
}
