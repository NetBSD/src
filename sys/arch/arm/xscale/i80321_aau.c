/*	$NetBSD: i80321_aau.c,v 1.11.6.1 2008/01/08 22:09:33 bouyer Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel i80321 I/O Processor application accelerator unit support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80321_aau.c,v 1.11.6.1 2008/01/08 22:09:33 bouyer Exp $");

#include <sys/param.h>
#include <sys/pool.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/uio.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <arm/xscale/iopaaureg.h>
#include <arm/xscale/iopaauvar.h>

struct aau321_softc {
	/* Shared AAU definitions. */
	struct iopaau_softc sc_iopaau;

	/* i80321-specific stuff. */
	void *sc_error_ih;
	void *sc_eoc_ih;
	void *sc_eot_ih;
};

static struct iopaau_function aau321_func_zero = {
	iopaau_func_zero_setup,
	NULL,
};

static struct iopaau_function aau321_func_fill8 = {
	iopaau_func_fill8_setup,
	NULL,
};

static struct iopaau_function aau321_func_xor_1_4 = {
	iopaau_func_xor_setup,
	NULL,
};

static struct iopaau_function aau321_func_xor_5_8 = {
	iopaau_func_xor_setup,
	NULL,
};

static const struct dmover_algdesc aau321_algdescs[] = {
	{
	  DMOVER_FUNC_ZERO,
	  &aau321_func_zero,
	  0
	},
	{
	  DMOVER_FUNC_FILL8,
	  &aau321_func_fill8,
	  0
	},
	{
	  DMOVER_FUNC_COPY,
	  &aau321_func_xor_1_4,
	  1
	},
	{
	  DMOVER_FUNC_XOR2,
	  &aau321_func_xor_1_4,
	  2
	},
	{
	  DMOVER_FUNC_XOR3,
	  &aau321_func_xor_1_4,
	  3
	},
	{
	  DMOVER_FUNC_XOR4,
	  &aau321_func_xor_1_4,
	  4
	},
	{
	  DMOVER_FUNC_XOR5,
	  &aau321_func_xor_5_8,
	  5
	},
	{
	  DMOVER_FUNC_XOR6,
	  &aau321_func_xor_5_8,
	  6
	},
	{
	  DMOVER_FUNC_XOR7,
	  &aau321_func_xor_5_8,
	  7
	},
	{
	  DMOVER_FUNC_XOR8,
	  &aau321_func_xor_5_8,
	  8
	},
};
#define	AAU321_ALGDESC_COUNT \
	(sizeof(aau321_algdescs) / sizeof(aau321_algdescs[0]))

static int
aau321_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct iopxs_attach_args *ia = aux;

	if (strcmp(match->cf_name, ia->ia_name) == 0)
		return (1);

	return (0);
}

static void
aau321_attach(struct device *parent, struct device *self, void *aux)
{
	struct aau321_softc *sc321 = (void *) self;
	struct iopaau_softc *sc = &sc321->sc_iopaau;
	struct iopxs_attach_args *ia = aux;
	int error;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_st = ia->ia_st;
	error = bus_space_subregion(sc->sc_st, ia->ia_sh,
	    ia->ia_offset, ia->ia_size, &sc->sc_sh);
	if (error) {
		aprint_error("%s: unable to subregion registers, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		return;
	}

	sc->sc_dmat = ia->ia_dmat;

	sc321->sc_error_ih = i80321_intr_establish(ICU_INT_AAUE, IPL_BIO,
	    iopaau_intr, sc);
	if (sc321->sc_error_ih == NULL) {
		aprint_error("%s: unable to register error interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc321->sc_eoc_ih = i80321_intr_establish(ICU_INT_AAU_EOC, IPL_BIO,
	    iopaau_intr, sc);
	if (sc321->sc_eoc_ih == NULL) {
		aprint_error("%s: unable to register EOC interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc321->sc_eot_ih = i80321_intr_establish(ICU_INT_AAU_EOT, IPL_BIO,
	    iopaau_intr, sc);
	if (sc321->sc_eoc_ih == NULL) {
		aprint_error("%s: unable to register EOT interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_dmb.dmb_name = sc->sc_dev.dv_xname;
	sc->sc_dmb.dmb_speed = 1638400;			/* XXX */
	sc->sc_dmb.dmb_cookie = sc;
	sc->sc_dmb.dmb_algdescs = aau321_algdescs;
	sc->sc_dmb.dmb_nalgdescs = AAU321_ALGDESC_COUNT;
	sc->sc_dmb.dmb_process = iopaau_process;

	iopaau_attach(sc);

	aau321_func_zero.af_desc_cache = iopaau_desc_4_cache;
	aau321_func_fill8.af_desc_cache = iopaau_desc_4_cache;
	aau321_func_xor_1_4.af_desc_cache = iopaau_desc_4_cache;
	aau321_func_xor_5_8.af_desc_cache = iopaau_desc_8_cache;
}

CFATTACH_DECL(iopaau, sizeof(struct aau321_softc),
    aau321_match, aau321_attach, NULL, NULL);
