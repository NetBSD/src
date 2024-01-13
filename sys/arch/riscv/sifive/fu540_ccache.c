/*	$NetBSD: fu540_ccache.c,v 1.1 2024/01/13 17:01:58 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * ``AS IS'' AND ANY EXPRESS OR IMPLinIED WARRANTIES, INCLUDING, BUT NOT LIMITED
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
__KERNEL_RCSID(0, "$NetBSD: fu540_ccache.c,v 1.1 2024/01/13 17:01:58 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <machine/cpufunc.h>

#define CCACHE_CONFIG			0x0000
#define  CCACHE_CONFIG_BANKS_MASK		__BITS( 7,  0)
#define  CCACHE_CONFIG_WAYS_MASK		__BITS(15,  8)
#define  CCACHE_CONFIG_LGSETS_MASK		__BITS(23, 16)
#define  CCACHE_CONFIG_LGBLOCKBYTES_MASK	__BITS(31, 24)
#define CCACHE_WAYENABLE		0x0008

#define CCACHE_ECCINJECTERROR		0x0040

#define CCACHE_DIRECCFIX_LOW		0x0100
#define CCACHE_DIRECCFIX_HIGH		0x0104
#define CCACHE_DIRECCFIX_COUNT 		0x0108

#define CCACHE_DIRECCFAIL_LOW		0x0120
#define CCACHE_DIRECCFAIL_HIGH	 	0x0124
#define CCACHE_DIRECCFAIL_COUNT 	0x0128

#define CCACHE_DATECCFIX_LOW		0x0140
#define CCACHE_DATECCFIX_HIGH		0x0144
#define CCACHE_DATECCFIX_COUNT		0x0148

#define CCACHE_DATECCFAIL_LOW		0x0160
#define CCACHE_DATECCFAIL_HIGH		0x0164
#define CCACHE_DATECCFAIL_COUNT		0x0168

#define CCACHE_FLUSH64			0x0200
#define CCACHE_FLUSH32			0x0250

#define CCACHE_MAX_ECCINTR		4

#define CCACHE_FLUSH64_LINE_LEN		64

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "sifive,fu540-c000-ccache" },
	{ .compat = "sifive,fu740-c000-ccache" },
	DEVICE_COMPAT_EOL
};

struct fu540_ccache_softc {
	device_t		 sc_dev;
	bus_space_tag_t		 sc_bst;
	bus_space_handle_t	 sc_bsh;

	uint32_t		 sc_line_size;
	uint32_t		 sc_size;
	uint32_t		 sc_sets;
	uint32_t		 sc_level;
};


static struct fu540_ccache_softc *fu540_ccache_sc;

#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define	WR8(sc, reg, val)						\
	bus_space_write_8((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


static void
fu540_ccache_cache_wbinv_range(vaddr_t va, paddr_t pa, psize_t len)
{
	struct fu540_ccache_softc * const sc = fu540_ccache_sc;

	KASSERT(powerof2(sc->sc_line_size));
	KASSERT(len != 0);

	const paddr_t spa = rounddown2(pa, sc->sc_line_size);
	const paddr_t epa = roundup2(pa + len, sc->sc_line_size);

	asm volatile ("fence iorw,iorw" ::: "memory");

	for (paddr_t fpa = spa; fpa < epa; fpa += sc->sc_line_size) {
#ifdef _LP64
		WR8(sc, CCACHE_FLUSH64, fpa);
#else
		WR4(sc, CCACHE_FLUSH32, fpa >> 4);
#endif
		asm volatile ("fence iorw,iorw" ::: "memory");
	}
}


static int
fu540_ccache_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
fu540_ccache_attach(device_t parent, device_t self, void *aux)
{
	struct fu540_ccache_softc * const sc = device_private(self);
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	int error;
	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	int ret;
	ret = of_getprop_uint32(phandle, "cache-block-size",
	    &sc->sc_line_size);
	if (ret < 0) {
		aprint_error(": can't get cache-block-size\n");
		return;
	}

	ret = of_getprop_uint32(phandle, "cache-level",
	    &sc->sc_level);
	if (ret < 0) {
		aprint_error(": can't get cache-level\n");
		return;
	}

	ret = of_getprop_uint32(phandle, "cache-sets",
	    &sc->sc_sets);
	if (ret < 0) {
		aprint_error(": can't get cache-sets\n");
		return;
	}
	ret = of_getprop_uint32(phandle, "cache-size",
	    &sc->sc_size);
	if (ret < 0) {
		aprint_error(": can't get cache-size\n");
		return;
	}

	if (!of_hasprop(phandle, "cache-unified")) {
		aprint_error(": can't get cache-unified\n");
		return;
	}

	uint32_t ways = sc->sc_size / (sc->sc_sets * sc->sc_line_size);

	aprint_naive("\n");
	aprint_normal(": L%u cache controller. %u KiB/%uB %u-way (%u set).\n",
	    sc->sc_level, sc->sc_size / 1024, sc->sc_line_size, ways,
	    sc->sc_sets);

	uint32_t l2config = RD4(sc, CCACHE_CONFIG);

	aprint_debug_dev(self,   "l2config        %#10" PRIx32 "\n",
	    l2config);
	aprint_verbose_dev(self, "No. of banks          %4" __PRIuBIT "\n",
	    __SHIFTOUT(l2config, CCACHE_CONFIG_BANKS_MASK));
	aprint_verbose_dev(self, "No. of ways per bank  %4" __PRIuBIT "\n",
	    __SHIFTOUT(l2config, CCACHE_CONFIG_WAYS_MASK));
	aprint_verbose_dev(self, "Sets per bank         %4lu\n",
	    1UL << __SHIFTOUT(l2config, CCACHE_CONFIG_LGSETS_MASK));
	aprint_verbose_dev(self, "Bytes per cache block %4lu\n",
	    1UL << __SHIFTOUT(l2config, CCACHE_CONFIG_LGBLOCKBYTES_MASK));

	uint32_t l2wayenable = RD4(sc, CCACHE_WAYENABLE);

	aprint_verbose_dev(self, "Largest way enabled   %4" PRIu32 "\n",
	    l2wayenable);

	fu540_ccache_sc = sc;

	cpu_sdcache_wbinv_range = fu540_ccache_cache_wbinv_range;
	cpu_sdcache_inv_range = fu540_ccache_cache_wbinv_range;
	cpu_sdcache_wb_range = fu540_ccache_cache_wbinv_range;
}

CFATTACH_DECL_NEW(fu540_ccache, sizeof(struct fu540_ccache_softc),
	fu540_ccache_match, fu540_ccache_attach, NULL, NULL);
