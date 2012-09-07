/*	$NetBSD: pl310.c,v 1.4 2012/09/07 21:18:58 matt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl310.c,v 1.4 2012/09/07 21:18:58 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/pl310_reg.h>
#include <arm/cortex/pl310_var.h>

static int arml2cc_match(device_t, cfdata_t, void *);
static void arml2cc_attach(device_t, device_t, void *);

#define	L2CC_BASE	0x2000
#define	L2CC_SIZE	0x1000

struct arml2cc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
};

CFATTACH_DECL_NEW(arml2cc, sizeof(struct arml2cc_softc),
    arml2cc_match, arml2cc_attach, NULL, NULL);

static void arml2cc_disable(struct arml2cc_softc *);

static bool attached;

static inline uint32_t
arml2cc_read_4(struct arml2cc_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_memh, o);
}

static inline void
arml2cc_write_4(struct arml2cc_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_memt, sc->sc_memh, o, v);
}


/* ARGSUSED */
static int
arml2cc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mpcore_attach_args * const mpcaa = aux;

	if (attached)
		return 0;

	if (!CPU_ID_CORTEX_A9_P(curcpu()->ci_arm_cpuid))
		return 0;

	if (strcmp(mpcaa->mpcaa_name, cf->cf_name) != 0)
		return 0;

	/*
	 * This isn't present on UP A9s (since CBAR isn't present).
	 */
	uint32_t mpidr = armreg_mpidr_read();
	if (mpidr == 0 || (mpidr & MPIDR_U))
		return 0;

	return 1;
}

static const struct {
	uint8_t rev;
	uint8_t str[7];
} pl310_revs[] = {
	{ 0, " r0p0" },
	{ 2, " r1p0" },
	{ 4, " r2p0" },
	{ 5, " r3p0" },
	{ 6, " r3p1" },
	{ 8, " r3p2" },
	{ 9, " r3p3" },
};

static void
arml2cc_attach(device_t parent, device_t self, void *aux)
{
        struct arml2cc_softc * const sc = device_private(self);
	struct mpcore_attach_args * const mpcaa = aux;

	sc->sc_dev = self;
	sc->sc_memt = mpcaa->mpcaa_memt;

	bus_space_subregion(sc->sc_memt, mpcaa->mpcaa_memh, 
	    L2CC_BASE, L2CC_SIZE, &sc->sc_memh);

	uint32_t id = arml2cc_read_4(sc, L2C_CACHE_ID);
	u_int rev = __SHIFTOUT(id, CACHE_ID_REV);

	const char *revstr = "";
	for (size_t i = 0; i < __arraycount(pl310_revs); i++) {
		if (rev == pl310_revs[i].rev) {
			revstr = pl310_revs[i].str;
			break;
		}
	}

	const bool enabled_p = arml2cc_read_4(sc, L2C_CTL) != 0;

	aprint_naive("\n");
	aprint_normal(": ARM PL310%s L2 Cache Controller%s\n",
	    revstr, enabled_p ? "" : " (disabled)");

	if (enabled_p) {
		arml2cc_disable(sc);
		aprint_normal_dev(self, "cache %s\n",
		    arml2cc_read_4(sc, L2C_CTL) ? "enabled" : "disabled");
	}

	KASSERT(arm_pcache.dcache_line_size == arm_scache.dcache_line_size);
}

void
arml2cc_disable(struct arml2cc_softc *sc)
{
	uint32_t way_mask = __BIT(arm_scache.dcache_ways) - 1;
	register_t psw = cpsid(I32_bit);

	arml2cc_write_4(sc, L2C_CLEAN_INV_WAY, way_mask);
	while (arml2cc_read_4(sc, L2C_CLEAN_INV_WAY) != 0) {
		/* spin */
	}

	arml2cc_write_4(sc, L2C_CACHE_SYNC, 0);
	while (arml2cc_read_4(sc, L2C_CACHE_SYNC) & 1) {
		/* spin */
	}

	arml2cc_write_4(sc, L2C_CTL, 0);	// turn it off */
	cpsie(psw);
}

void
arml2cc_init(bus_space_tag_t bst, bus_space_handle_t bsh, bus_size_t o)
{
	struct arm_cache_info * const info = &arm_scache;

	uint32_t cfg = bus_space_read_4(bst, bsh, o + L2C_CACHE_TYPE);

	info->cache_type = __SHIFTOUT(cfg, CACHE_TYPE_CTYPE);
	info->cache_unified = __SHIFTOUT(cfg, CACHE_TYPE_HARVARD) == 0;
	u_int cfg_dsize = __SHIFTOUT(cfg, CACHE_TYPE_DSIZE);

	u_int d_waysize = 8192 << __SHIFTOUT(cfg_dsize, CACHE_TYPE_xWAYSIZE);
	info->dcache_ways = 8 << __SHIFTOUT(cfg_dsize, CACHE_TYPE_xASSOC);
	info->dcache_line_size = 32 << __SHIFTOUT(cfg_dsize, CACHE_TYPE_xLINESIZE);
	info->dcache_size = info->dcache_ways * d_waysize;

	if (info->cache_unified) {
		info->icache_ways = info->dcache_ways;
		info->icache_line_size = info->dcache_line_size;
		info->icache_size = info->dcache_size;
	} else {
		u_int cfg_isize = __SHIFTOUT(cfg, CACHE_TYPE_ISIZE);
		u_int i_waysize = 8192 << __SHIFTOUT(cfg_isize, CACHE_TYPE_xWAYSIZE);
		info->icache_ways = 8 << __SHIFTOUT(cfg_isize, CACHE_TYPE_xASSOC);
		info->icache_line_size = 32 << __SHIFTOUT(cfg_isize, CACHE_TYPE_xLINESIZE);
		info->icache_size = i_waysize * info->icache_ways;
	}
}

#if 0
static void
arml2cc_dcache_wb_range(vaddr_t va, vsize_t len)
{
	const size_t line_size = arm_pcache.dcache_line_size;
	const size_t line_mask = line_size - 1;
	size_t off = va & line_mask;
	size_t page_len = 0;
	paddr_t pa = 0;
	if (off) {
		len += off;
		va -= off;
	}
	for (const vaddr_t endva = va + roundup2(len, line_size);
	     va < endva;
	     va += line_size) {
		armreg_dccmvac(va);		/* this may fault */
		__asm __volatile("dsb");
		if (page_len == 0) {
			armreg_ats1cpr_write(va);
			pa = armreg_par_read();
			const psize = __predict_false(pa & 1) ? L1_SS_SIZE : L2_S_SIZE;
			pa &= -psize;
			endpa = pa + psize;
			page_len = (va & (psize - 1);
		}
		size_t set = va 
		arml2cc_write_4(sc, L2C_CLEAN_PA, pa);
		
		va += line_size;
	}
}
#endif
