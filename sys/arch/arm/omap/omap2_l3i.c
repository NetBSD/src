/*	$Id: omap2_l3i.c,v 1.1.20.1 2008/05/18 12:31:37 yamt Exp $	*/

/* adapted from: */
/*	$NetBSD: omap2_l3i.c,v 1.1.20.1 2008/05/18 12:31:37 yamt Exp $ */


/*
 * Autoconfiguration support for the Texas Instruments OMAP "On Board" I/O.
 *
 * Based on arm/omap/omap_emifs.c which in turn was derived
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
 * Copyright (c) 1997, 1998, 2001, The NetBSD Foundation, Inc.
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

#include "opt_omap.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_l3i.c,v 1.1.20.1 2008/05/18 12:31:37 yamt Exp $");

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
#include <arm/omap/omap_var.h>

#if defined(OMAP2)
#include <arm/omap/omap2430reg.h>
#ifdef NOTYET
#include <arm/omap/omap2430var.h>
#endif
#else
/*
 * we have only used this with OMAP 2430 so far....
 * some of the 2430 stuff may generalize to other OMAP implementations,
 * or not.  Either generalize the include files accordingly, or
 * add your own implementation-specific includes.
 */
# error unknown OMAP L3 Interconnect implementation
#endif

struct L3i_softc {
	struct device		sc_dev;
	bus_dma_tag_t		sc_dmac;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};


/* prototypes */
static int	L3i_match(struct device *, struct cfdata *, void *);
static void	L3i_attach(struct device *, struct device *, void *);
void		L3i_target_agent_check(struct L3i_softc *, bus_addr_t, char *);
static void	L3i_decode_ta_COMPONENT(uint32_t);
static void	L3i_decode_ta_CORE(uint32_t);
static void	L3i_decode_ta_AGENT_CONTROL(uint32_t);
static void	L3i_decode_ta_AGENT_STATUS(uint32_t);
static void	L3i_decode_ta_ERROR_LOG(uint32_t);
static void	L3i_decode_ta_ERROR_LOG_ADDR(uint32_t);
#ifdef NOTYET
static int 	L3i_search(struct device *, struct cfdata *,
			     const int *, void *);
static int	L3i_print(void *, const char *);
#endif

#define TARGET_AGENT_REGS_ENTRY(reg) \
	{ .offset = OMAP2430_TA_ ## reg, \
	  .decode = L3i_decode_ta_ ## reg, \
	  .name = #reg }
struct {
	bus_size_t offset;
	void (*decode)(uint32_t);
	const char *name;
} target_agent_regs[] = {
	TARGET_AGENT_REGS_ENTRY(COMPONENT),
	TARGET_AGENT_REGS_ENTRY(CORE),
	TARGET_AGENT_REGS_ENTRY(AGENT_CONTROL),
	TARGET_AGENT_REGS_ENTRY(AGENT_STATUS),
	TARGET_AGENT_REGS_ENTRY(ERROR_LOG),
	TARGET_AGENT_REGS_ENTRY(ERROR_LOG_ADDR),
};
#define TARGET_AGENT_REGS_NENTRIES \
		(sizeof(target_agent_regs) / sizeof(target_agent_regs[0]))


/* attach structures */
CFATTACH_DECL(L3i, sizeof(struct L3i_softc),
	L3i_match, L3i_attach, NULL, NULL);

static int L3i_attached;	/* XXX assumes only 1 instance */

static int
L3i_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (L3i_attached != 0)
		return 0;

	return 1;
}

static void
L3i_attach(struct device *parent, struct device *self, void *aux)
{
	struct L3i_softc *sc = (struct L3i_softc *)self;

	sc->sc_iot = &omap_bs_tag;

	aprint_normal(": L3i Interconnect\n");

	sc->sc_ioh = 0;
#ifdef NOTYET
	sc->sc_dmac = &omap_bus_dma_tag;
#endif

	L3i_attached = 1;

#ifdef L3I_DEBUG
	L3i_target_agent_check(sc, (bus_addr_t)OMAP2430_TA_L4_CORE, "L4 core");
	L3i_target_agent_check(sc, (bus_addr_t)OMAP2430_TA_GPMC, "GPMC");
#endif

#ifdef NOTYET
	/*
	 * Attach all our devices
	 */
	config_search_ia(L3i_search, self, "L3i", NULL);
#endif
}

static void
L3i_decode_ta_COMPONENT(uint32_t val)
{
	aprint_normal("\tCODE %#" PRIxMAX "\n", TA_COMPONENT_CODE(val));
	aprint_normal("\tREV  %#" PRIxMAX "\n", TA_COMPONENT_REV(val));
}

static void
L3i_decode_ta_CORE(uint32_t val)
{
	aprint_normal("\tCODE %#" PRIxMAX "\n", TA_AGENT_CORE_CODE(val));
	aprint_normal("\tREV  %#" PRIxMAX "\n", TA_AGENT_CORE_REV(val));
}

static void
L3i_decode_ta_AGENT_CONTROL(uint32_t val)
{
	const uint core_timeout_base_tab[8] = {
		0, 1, 2, 3, 4, -1, -1, -1
	};
	uint core_timeout_ix;

	core_timeout_ix = (val & TA_AGENT_CONTROL_CORE_TIMEOUT_BASE)
				>> TA_AGENT_CONTROL_CORE_TIMEOUT_BASE_SHFT;

	aprint_normal("\tCORE_REQ_TIMEOUT_REP %d\n",
			((val & TA_AGENT_CONTROL_CORE_REQ_TIMEOUT_REP) != 0));
	aprint_normal("\tCORE_SERROR_REP %d\n",
			((val & TA_AGENT_CONTROL_CORE_SERROR_REP) != 0));
	aprint_normal("\tCORE_TIMEOUT_BASE %d\n",
			core_timeout_base_tab[core_timeout_ix]);
	aprint_normal("\tCORE_REJECT %d\n",
			((val & TA_AGENT_CONTROL_CORE_REJECT) != 0));
	aprint_normal("\tCORE_RESET %d\n",
			((val & TA_AGENT_CONTROL_CORE_RESET) != 0));
}

static void
L3i_decode_ta_AGENT_STATUS(uint32_t val)
{
	aprint_normal("\tSERROR %d\n",
			((val & TA_AGENT_STATUS_SERROR) != 0));
	aprint_normal("\tBURST_CLOSE %d\n",
			((val & TA_AGENT_STATUS_BURST_CLOSE) != 0));
	aprint_normal("\tTA_AGENT_STATUS_TIMEBASE %" PRIdMAX "\n",
			((val & TA_AGENT_STATUS_TIMEBASE) >> 12));
	aprint_normal("\tREQ_TIMEOUT %d\n",
			((val & TA_AGENT_STATUS_REQ_TIMEOUT) != 0));
	aprint_normal("\tREADEX %d\n",
			((val & TA_AGENT_STATUS_READEX) != 0));
	aprint_normal("\tBURST %d\n",
			((val & TA_AGENT_STATUS_BURST) != 0));
	aprint_normal("\tRESP_ACTIVE %d\n",
			((val & TA_AGENT_STATUS_RESP_ACTIVE) != 0));
	aprint_normal("\tREQ_WAITING %d\n",
			((val & TA_AGENT_STATUS_REQ_WAITING) != 0));
	aprint_normal("\tCORE_RESET %d\n",
			((val & TA_AGENT_STATUS_CORE_RESET) != 0));
}

static void
L3i_decode_ta_ERROR_LOG(uint32_t val)
{
	aprint_normal("\tCMD %" PRIdMAX "\n",
			(val & TA_ERROR_LOG_CMD));
	aprint_normal("\tINITID %" PRIdMAX "\n",
			(val & TA_ERROR_LOG_INITID) >> 8);
	aprint_normal("\tCODE %" PRIdMAX "\n",
			(val & TA_ERROR_LOG_CODE) >> 24);
	aprint_normal("\tMULTI %d\n",
			((val & TA_ERROR_LOG_MULTI) != 0));
}

static void
L3i_decode_ta_ERROR_LOG_ADDR(uint32_t val)
{
}


void
L3i_target_agent_check(struct L3i_softc *sc, bus_addr_t ba, char *agent)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh;
	uint32_t r;
	uint i;
	int err;

	err = bus_space_map(sc->sc_iot, ba, 1024, 0, &ioh);
	if (err != 0) {
		aprint_error("%s: cannot map L3 Target Agent %s at %#lx\n",
			sc->sc_dev.dv_xname, agent, ba);
		return;
	} 

	for (i=0; i < TARGET_AGENT_REGS_NENTRIES; i++) {
		r = bus_space_read_4(iot, ioh, target_agent_regs[i].offset);
		aprint_normal("%s: Agent %s Reg %s: %#x\n", sc->sc_dev.dv_xname,
			agent, target_agent_regs[i].name, r);
		target_agent_regs[i].decode(r);
			
	}

	bus_space_unmap(sc->sc_iot, ioh, 1024);
}


#ifdef NOTYET

static int
L3i_search(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct L3i_softc *sc = (struct L3i_softc *)parent;
	struct L3i_attach_args aa;

	/* Set up the attach args. */
	if (cf->cf_loc[L3iCF_NOBYTEACC] == 1) {
		if (cf->cf_loc[L3iCF_MULT] == 1)
			aa.L3i_iot = &nobyteacc_bs_tag;
		else
			panic("nobyteacc specified for device with "
				"non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[L3iCF_MULT]) {
		case 1:
			aa.L3i_iot = &omap_bs_tag;
			break;
		case 2:
			aa.L3i_iot = &omap_a2x_bs_tag;
			break;
		case 4:
			aa.L3i_iot = &omap_a4x_bs_tag;
			break;
		default:
			panic("Unsupported EMIFS multiplier.");
			break;
		}
	}

	aa.L3i_dmac = sc->sc_dmac;
	aa.L3i_addr = cf->cf_loc[L3iCF_ADDR];
	aa.L3i_size = cf->cf_loc[L3iCF_SIZE];
	aa.L3i_intr = cf->cf_loc[L3iCF_INTR];

#if defined(OMAP2)
	if ((aa.L3i_addr >= OMAP2430_L3i_BASE)
	&&  (aa.L3i_addr < (OMAP2430_L4_CORE_BASE + OMAP2430_L3i_SIZE))) {
		/* XXX
		 * if size was specified, then check it too
		 * otherwise just assume it is OK
		 */
		if ((aa.L3i_size != L3iCF_SIZE_DEFAULT)
		&&  ((aa.L3i_addr + aa.L3i_size)
			>= (OMAP2430_L4_CORE_BASE + OMAP2430_L3i_SIZE)))
				return 1;		/* NG */
		if (config_match(parent, cf, &aa)) {
			config_attach(parent, cf, &aa, L3i_print);
			return 0;			/* love it */
		}
	}
#endif

	return 1;	/* NG */
}

static int
L3i_print(void *aux, const char *name)
{
	struct L3i_attach_args *sa = (struct L3i_attach_args*)aux;

	if (sa->L3i_addr != L3iCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->L3i_addr);
		if (sa->L3i_size != L3iCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx",
				sa->L3i_addr + sa->L3i_size-1);
	}
	if (sa->L3i_intr != L3iCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->L3i_intr);

	return UNCONF;
}

#endif /* NOTYET */
