/*	$NetBSD: omap_emifs.c,v 1.1.52.1 2008/06/02 13:21:55 mjf Exp $ */


/*
 * Autoconfiguration support for the Texas Instruments OMAP EMIFS bus.
 * Based on arm/xscale/pxa2x0.c which in turn was derived
 * from arm/sa11x0/sa11x0.c
 *
 * Copyright (c) 2002, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997,1998, 2001, The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro, Ichiro FUKUHARA and Paul Kranenburg.
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
 *
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_emifs.c,v 1.1.52.1 2008/06/02 13:21:55 mjf Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_emifs.h>

struct emifs_softc {
	struct device		sc_dev;
	bus_dma_tag_t		sc_dmac;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

enum which_clock { TC_CLK, REF_CLK };
enum which_reg { CCS, ACS };
typedef struct timing_parm_info {
	const char	*name;			/* name of parm */
	int		cf_ndx;			/* Index to get parm */
	enum		which_clock clk;	/* TC_CLK or REF_CLK */
	enum		which_reg reg;		/* CCS or ACS? */
	u_int		shift;			/* Field's register position */
	u_int		adder;			/* Number of cycles to add */
	u_int		max;			/* Maximum field value */
} timing_parm_info;

/* prototypes */
static int	emifs_match(struct device *, struct cfdata *, void *);
static void	emifs_attach(struct device *, struct device *, void *);
static u_int	emifs_cvt_nsec(const timing_parm_info *, u_int, int);
static void	emifs_set_timing(struct emifs_softc *, struct cfdata *);
static int 	emifs_search(struct device *, struct cfdata *,
			     const int *, void *);
static int	emifs_print(void *, const char *);

#ifndef OMAP_TC_CLOCK_FREQ
#error Specify the TC clock frequency in Hz with the OMAP_TC_CLOCK_FREQ option.
#endif

/* Encapsulate the device knowledge within this source. */
/* Register offsets, values and information needed to set them. */
#define EMIFS_CCS(cs)	(0x10+((cs)*4))	/* chip-select config */
#define EMIFS_ACS(cs)	(0x50+((cs)*4))	/* advanced chip-select config */
#define  EMIFS_ACS_BTMODE	(1<<9)
#define EMIFS_SIZE			256	/* per TRM */

static const timing_parm_info timing_parms[] = {
	{ "rdwst",   EMIFSCF_RDWST,   REF_CLK, CCS,  4, 2, 0xF },
	{ "oesetup", EMIFSCF_OESETUP, REF_CLK, ACS,  0, 0, 0xF },
	{ "oehold",  EMIFSCF_OEHOLD,  REF_CLK, ACS,  4, 0, 0xF },
	{ "wrwst",   EMIFSCF_WRWST,   REF_CLK, CCS,  8, 1, 0xF },
	{ "welen",   EMIFSCF_WELEN,   REF_CLK, CCS, 12, 1, 0xF },
	{ "advhold", EMIFSCF_ADVHOLD, REF_CLK, ACS,  8, 1, 0x1 },
	{ "btwst",   EMIFSCF_BTWST,   TC_CLK,  CCS, 23, 1, 0xF }
};

/* attach structures */
CFATTACH_DECL(emifs, sizeof(struct emifs_softc),
    emifs_match, emifs_attach, NULL, NULL);

static int emifs_attached;

static int
emifs_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (emifs_attached)
		return 0;
	return 1;
}

static void
emifs_attach(struct device *parent, struct device *self, void *aux)
{
	struct emifs_softc *sc = (struct emifs_softc *)self;
	struct mainbus_attach_args *mainbus = aux;

	/*
	 * mainbus->mb_iot always multiplies the offset by 4 and doesn't seem
	 * to be widely used, so I'm just going to use the omap bus.
	 */
	if (mainbus->mb_iobase != -1) {
		/*
		 * The address is only needed for modifying the timings, so
		 * don't make it mandatory.  We'll check to see if it was set
		 * when (and if) it is needed.
		 */
		sc->sc_iot = &omap_bs_tag;
		if (bus_space_map(sc->sc_iot,
				  mainbus->mb_iobase, EMIFS_SIZE,
				  0, &sc->sc_ioh))
			panic("%s: Cannot map registers", self->dv_xname);
	} else
		sc->sc_iot = NULL;

	emifs_attached = 1;

#if NOMAPDMAC > 0
#error DMA not implemented
	sc->sc_dmac = &omap_bus_dma_tag;
#else
	sc->sc_dmac = NULL;
#endif

	aprint_normal(": Extended Memory Interface Slow\n");
	aprint_naive("\n");

	/*
	 * Attach all our devices
	 */
	config_search_ia(emifs_search, self, "emifs", NULL);
}

static const u_int ns_per_sec = 1000000000;

static u_int
emifs_cvt_nsec(const timing_parm_info *tp, u_int source_freq, int nsec)
{
	u_int desired_freq, clocks, rval;

	/*
	 * It is easier to work with a frequency in Hz, instead of a
	 * period in nanoseconds.
	 */
	desired_freq = ns_per_sec / nsec;

	/*
	 * Then we can just divide the source frequency by the desired
	 * frequency to get the number of clocks.  If it doesn't divide
	 * evenly, round up to make sure the period is long enough.
	 */
	clocks = source_freq / desired_freq;
	if ((clocks * desired_freq) != source_freq)
		clocks++;

	/* Adjust for the number of cycles that are added. */
	rval = clocks - tp->adder;

	/* Limit to maximum */
	if (rval > tp->max) {
		aprint_error("EMIFS: %s: %d ns is too large.\n",
			     tp->name, nsec);
		rval = tp->max;
	}

	return rval;
}

static void
emifs_set_timing(struct emifs_softc *sc, struct cfdata *cf)
{
	static const u_int tc_freq = OMAP_TC_CLOCK_FREQ;
	/* We force REF to be the same frequency as TC. */
	static const u_int ref_freq = tc_freq;

	int cs, i;
	uint32_t ccs, acs;

	/*
	 * Ensure we have what everything we need to set the EMIFS bus
	 * timing parameters.
	 */
	cs = cf->cf_loc[EMIFSCF_CS];
	if (cs < 0 || cs > 3)
		panic("cs parameter must be in the range of 0 to 3.");
	if (sc->sc_iot == NULL)
		panic("Parent emifs device must have base specified.");

	ccs = 0;
	acs = 0;
	switch (cf->cf_loc[EMIFSCF_BTMODE]) {
	case -1:
	case 0:
		break;
	case 1:
		acs = EMIFS_ACS_BTMODE;
		break;
	default:
		panic("btmode must be 0, 1 or not given.");
	}

	aprint_verbose("EMIFS:  TC_CK period: %u ns\n", ns_per_sec / tc_freq);
	aprint_verbose("EMIFS: REF_CK period: %u ns\n", ns_per_sec / ref_freq);

	for (i = 0; i < __arraycount(timing_parms); i++) {
		const timing_parm_info *tp;
		int nsec;
		u_int source_freq, field_val;

		tp = &timing_parms[i];
		nsec = cf->cf_loc[tp->cf_ndx];

		/* Blow up on completely wrong parameters. */
		if (nsec < 0 || nsec > OMAP_TC_CLOCK_FREQ)
			panic("Invalid %s period of %d nsec.", tp->name, nsec);

		if (tp->clk == REF_CLK)
			source_freq = ref_freq;
		else
			source_freq = tc_freq;

		if (nsec == 0)
			/*
			 * Handle the zero case separately because: 1) it
			 * avoids a divide by zero case, 2) we already know
			 * the answer is zero, and 3) we know the field is
			 * already zeroed.
			 */
			field_val = 0;
		else {
			field_val = emifs_cvt_nsec(tp, source_freq, nsec);

			if (tp->reg == CCS)
				ccs |= (field_val << tp->shift);
			else
				acs |= (field_val << tp->shift);
		}

		/*
		 * Tell them what they got.  The ">> 5"'s are to prevent
		 * overflow.  We know ns_per_sec fits in a word, and we know
		 * that (field_val + tp->addr) is less than five bits.  If we
		 * shift the numerator and denominator the same amount, we'll
		 * get the same answer, but without overflow.
		 */
		aprint_verbose("EMIFS: %8s: Requested %4u ns.  Got %4u ns.\n",
			       tp->name, nsec,
			       ((field_val + tp->adder) * (ns_per_sec >> 5)
				/ (source_freq >> 5)));
	}

	/* Now tell the hardware what we figured out. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EMIFS_CCS(cs), ccs);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, EMIFS_ACS(cs), acs);
}

static int
emifs_search(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct emifs_softc *sc = (struct emifs_softc *)parent;
	struct emifs_attach_args aa;

	/* Set up the attach args. */
	if (cf->cf_loc[EMIFSCF_NOBYTEACC] == 1) {
		if (cf->cf_loc[EMIFSCF_MULT] == 1)
			aa.emifs_iot = &nobyteacc_bs_tag;
		else
			panic("nobyteacc specified for device with non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[EMIFSCF_MULT]) {
		case 1:
			aa.emifs_iot = &omap_bs_tag;
			break;
		case 2:
			aa.emifs_iot = &omap_a2x_bs_tag;
			break;
		case 4:
			aa.emifs_iot = &omap_a4x_bs_tag;
			break;
		default:
			panic("Unsupported EMIFS multiplier.");
			break;
		}
	}

	aa.emifs_dmac = sc->sc_dmac;
	aa.emifs_addr = cf->cf_loc[EMIFSCF_ADDR];
	aa.emifs_size = cf->cf_loc[EMIFSCF_SIZE];
	aa.emifs_intr = cf->cf_loc[EMIFSCF_INTR];

	/* Chip-select specified? */
	if (cf->cf_loc[EMIFSCF_CS] != -1)
		emifs_set_timing(sc, cf);

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, emifs_print);

	return 0;
}

static int
emifs_print(void *aux, const char *name)
{
	struct emifs_attach_args *sa = (struct emifs_attach_args*)aux;

	if (sa->emifs_addr != EMIFSCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->emifs_addr);
		if (sa->emifs_size > EMIFSCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx", sa->emifs_addr + sa->emifs_size-1);
	}
	if (sa->emifs_intr != EMIFSCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->emifs_intr);

	return (UNCONF);
}
