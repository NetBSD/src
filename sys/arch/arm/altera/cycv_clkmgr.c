/* $NetBSD: cycv_clkmgr.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $ */

/* This file is in the public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cycv_clkmgr.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <arm/altera/cycv_reg.h>
#include <arm/altera/cycv_var.h>

#include <dev/fdt/fdtvar.h>

#define CYCV_CLOCK_OSC1		25000000

static int cycv_clkmgr_match(device_t, cfdata_t, void *);
static void cycv_clkmgr_attach(device_t, device_t, void *);

static struct clk *cycv_clkmgr_clock_decode(device_t, int, const void *,
					    size_t);

static const struct fdtbus_clock_controller_func cycv_clkmgr_fdtclock_funcs = {
	.decode = cycv_clkmgr_clock_decode
};

static struct clk *cycv_clkmgr_clock_get(void *, const char *);
static void cycv_clkmgr_clock_put(void *, struct clk *);
static u_int cycv_clkmgr_clock_get_rate(void *, struct clk *);
static int cycv_clkmgr_clock_set_rate(void *, struct clk *, u_int);
static int cycv_clkmgr_clock_enable(void *, struct clk *);
static int cycv_clkmgr_clock_disable(void *, struct clk *);
static int cycv_clkmgr_clock_set_parent(void *, struct clk *, struct clk *);
static struct clk *cycv_clkmgr_clock_get_parent(void *, struct clk *);

static const struct clk_funcs cycv_clkmgr_clock_funcs = {
	.get = cycv_clkmgr_clock_get,
	.put = cycv_clkmgr_clock_put,
	.get_rate = cycv_clkmgr_clock_get_rate,
	.set_rate = cycv_clkmgr_clock_set_rate,
	.enable = cycv_clkmgr_clock_enable,
	.disable = cycv_clkmgr_clock_disable,
	.get_parent = cycv_clkmgr_clock_get_parent,
	.set_parent = cycv_clkmgr_clock_set_parent,
};

struct cycv_clk {
	struct clk base;

	int id;
	u_int refcnt;

	struct cycv_clk *parent;	/* cached and valid if not NULL */
	/* parent_id is not zero and filled with dtb if only one parent clock */
	int parent_id;

	int type;
#define CYCV_CLK_TYPE_PLL	0x0001
#define CYCV_CLK_TYPE_FIXED	0x0002
#define CYCV_CLK_TYPE_FIXED_DIV	0x0003
#define CYCV_CLK_TYPE_DIV	0x0004

	int flags;
#define CYCV_CLK_FLAG_HAVE_GATE	0x0001
#define CYCV_CLK_FLAG_IS_AVAIL	0x0002

	union {
		bus_addr_t pll_addr;
		uint32_t fixed_freq;
		uint32_t fixed_div;
		struct {
			bus_addr_t addr;
			uint32_t mask;
			int shift;
		} div;
	} u;

	bus_addr_t gate_addr;
	int gate_shift;
};

struct cycv_clkmgr_softc {
	device_t sc_dev;
	struct clk_domain sc_clkdom;

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct cycv_clk *sc_clocks;
	u_int sc_nclocks;
};

static void cycv_clkmgr_init(struct cycv_clkmgr_softc *, int);
static void cycv_clkmgr_clock_parse(struct cycv_clkmgr_softc *, int, u_int);
static u_int cycv_clkmgr_clocks_traverse(struct cycv_clkmgr_softc *, int,
	void (*)(struct cycv_clkmgr_softc *, int, u_int), u_int);
static struct cycv_clk_mux_info *cycv_clkmgr_get_mux_info(const char *);
static void cycv_clkmgr_clock_print(struct cycv_clkmgr_softc *,
	struct cycv_clk *);

CFATTACH_DECL_NEW(cycvclkmgr, sizeof (struct cycv_clkmgr_softc),
	cycv_clkmgr_match, cycv_clkmgr_attach, NULL, NULL);

static int
cycv_clkmgr_match(device_t parent, cfdata_t cf, void *aux)
{
	const char *compatible[] = { "altr,clk-mgr", NULL };
	struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
cycv_clkmgr_attach(device_t parent, device_t self, void *aux)
{
	struct cycv_clkmgr_softc *sc = device_private(self);
	struct fdt_attach_args *faa = aux;
	int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t) addr, error);
		return;
	}

	aprint_normal(": clock manager\n");

	sc->sc_clkdom.funcs = &cycv_clkmgr_clock_funcs;
	sc->sc_clkdom.priv = sc;

	cycv_clkmgr_init(sc, phandle);
}

static void
cycv_clkmgr_init(struct cycv_clkmgr_softc *sc, int clkmgr_handle)
{
	int clocks_handle;

	clocks_handle = of_find_firstchild_byname(clkmgr_handle, "clocks");
	if (clocks_handle == -1) {
		aprint_error_dev(sc->sc_dev, "no clocks property\n");
		return;
	}

	sc->sc_nclocks = cycv_clkmgr_clocks_traverse(sc, clocks_handle, NULL,
						     0);

	sc->sc_clocks = kmem_zalloc(sc->sc_nclocks * sizeof *sc->sc_clocks,
				    KM_NOSLEEP);
	if (sc->sc_clocks == NULL) {
		aprint_error_dev(sc->sc_dev, "no memory\n");
		sc->sc_nclocks = 0;
		return;
	}

	cycv_clkmgr_clocks_traverse(sc, clocks_handle, cycv_clkmgr_clock_parse,
				    0);

#if 1
	for (int i = 0; i < sc->sc_nclocks; i++)
		cycv_clkmgr_clock_print(sc, &sc->sc_clocks[i]);
#else
	(void) cycv_clkmgr_clock_print;
#endif
}

#define CYCV_CLK_MAX_PARENTS 3

static struct cycv_clk_mux_info {
	const char *name;
	const char *parents[CYCV_CLK_MAX_PARENTS];
	int nparents;
	bus_addr_t addr;
	uint32_t mask;
} cycv_clk_mux_tree[] = {
	{ "periph_pll", { "osc1", "osc2", "f2s_periph_ref_clk" }, 3,
		0x80, 0x00c00000 },
	{ "sdram_pll", { "osc1", "osc2", "f2s_sdram_ref_clk" }, 3,
		0xc0, 0x00c00000 },
	{ "l4_mp_clk", { "mainclk", "per_base_clk" }, 2, 0x70, 0x00000001 },
	{ "l4_sp_clk", { "mainclk", "per_base_clk" }, 2, 0x70, 0x00000002 },
	{ "sdmmc_clk",
	    { "f2s_periph_ref_clk", "main_nand_sdmmc_clk", "per_nand_mmc_clk" },
	    3, 0xac, 0x00000003 },
	{ "nand_x_clk",
	    { "f2s_periph_ref_clk", "main_nand_sdmmc_clk", "per_nand_mmc_clk" },
	    3, 0xac, 0x0000000c },
	{ "qspi_clk", { "f2s_periph_ref_clk", "main_qspi_clk", "per_qsi_clk" },
	    3, 0xac, 0x00000030 },

	/* Don't special case bypass */
	{ "dbg_base_clk", { "main_pll" }, 1, 0, 0 },
	/* Bug in dtb */
	{ "nand_clk", { "nand_x_clk" }, 1, 0, 0 },
};

static const char * const cycv_clkmgr_compat_fixed[] = { "fixed-clock", NULL };
static const char * const cycv_clkmgr_compat_pll[] = { "altr,socfpga-pll-clock",
	NULL };
static const char * const cycv_clkmgr_compat_perip[] = {
	"altr,socfpga-perip-clk",
	"altr,socfpga-gate-clk",
	NULL
};

static void
cycv_clkmgr_clock_parse(struct cycv_clkmgr_softc *sc, int handle, u_int clkno)
{
	struct cycv_clk *clk = &sc->sc_clocks[clkno];
	int flags = 0;
	const uint8_t *buf;
	int len;

	clk->base.domain = &sc->sc_clkdom;
	clk->base.name = fdtbus_get_string(handle, "name");
	clk->base.flags = 0;

	clk->id = handle;
	clk->parent = NULL;
	clk->parent_id = 0;
	clk->refcnt = 0;

	if (of_compatible(handle, cycv_clkmgr_compat_fixed) != -1) {
		clk->type = CYCV_CLK_TYPE_FIXED;
		if (of_getprop_uint32(handle, "clock-frequency",
				      &clk->u.fixed_freq) == 0) {
			flags |= CYCV_CLK_FLAG_IS_AVAIL;
		}
	} else if (of_compatible(handle, cycv_clkmgr_compat_pll) != -1) {
		if (fdtbus_get_reg(handle, 0, &clk->u.pll_addr, NULL) != 0)
			goto err;
		clk->type = CYCV_CLK_TYPE_PLL;
		flags |= CYCV_CLK_FLAG_IS_AVAIL;
	} else if (of_compatible(handle, cycv_clkmgr_compat_perip) != -1) {
		if (of_getprop_uint32(handle, "fixed-divider",
				      &clk->u.fixed_div) == 0) {
			clk->type = CYCV_CLK_TYPE_FIXED_DIV;
		} else if (fdtbus_get_reg(handle, 0, &clk->u.div.addr, NULL) ==
				0) {
			clk->type = CYCV_CLK_TYPE_DIV;
			clk->u.div.shift = 0;
			clk->u.div.mask = 0xff;
		} else if ((buf = fdtbus_get_prop(handle, "div-reg", &len)) !=
				NULL) {
			if (len != 3 * 4)
				goto err;

			clk->type = CYCV_CLK_TYPE_DIV;
			clk->u.div.addr = of_decode_int(buf);
			clk->u.div.shift = of_decode_int(buf + 4);
			clk->u.div.mask = ((1 << of_decode_int(buf + 8)) - 1) <<
				clk->u.div.shift;
		} else {
			/* Simply a gate and/or a mux */
			clk->type = CYCV_CLK_TYPE_FIXED_DIV;
			clk->u.fixed_div = 1;
		}
		flags |= CYCV_CLK_FLAG_IS_AVAIL;
	} else
		goto err;

	if ((buf = fdtbus_get_prop(handle, "clk-gate", &len)) != NULL) {
		clk->gate_addr = of_decode_int(buf);
		clk->gate_shift = of_decode_int(buf + 4);
		flags |= CYCV_CLK_FLAG_HAVE_GATE;
	}

	buf = fdtbus_get_prop(handle, "clocks", &len);
	if (buf != NULL && len == sizeof (uint32_t)) {
		clk->parent_id =
			fdtbus_get_phandle_from_native(of_decode_int(buf));
	}

	clk->flags = flags;

	fdtbus_register_clock_controller(sc->sc_dev, handle,
		&cycv_clkmgr_fdtclock_funcs);

	return;
err:
	aprint_debug_dev(sc->sc_dev, "(%s) error parsing phandle %d\n",
		clk->base.name, handle);
}

static u_int
cycv_clkmgr_clocks_traverse(struct cycv_clkmgr_softc *sc, int clocks_handle,
			    void func(struct cycv_clkmgr_softc *, int, u_int),
			    u_int nclocks)
{
	int clk_handle;

	for (clk_handle = OF_child(clocks_handle); clk_handle != 0;
					clk_handle = OF_peer(clk_handle)) {
		if (func != NULL)
			func(sc, clk_handle, nclocks);
		nclocks++;

		nclocks = cycv_clkmgr_clocks_traverse(sc, clk_handle, func,
						      nclocks);
	}

	return nclocks;
}

static struct cycv_clk_mux_info *
cycv_clkmgr_get_mux_info(const char *name) {
	size_t i;

	for (i = 0; i < __arraycount(cycv_clk_mux_tree); i++) {
		struct cycv_clk_mux_info *cand = &cycv_clk_mux_tree[i];
		if (strncmp(name, cand->name, strlen(cand->name)) == 0)
			return cand;
	}

	return NULL;
}

static struct cycv_clk *
cycv_clkmgr_clock_lookup_by_id(struct cycv_clkmgr_softc *sc, int id)
{
	size_t i;

	for (i = 0; i < sc->sc_nclocks; i++) {
		if (sc->sc_clocks[i].id == id)
			return &sc->sc_clocks[i];
	}

	return NULL;
}

static struct cycv_clk *
cycv_clkmgr_clock_lookup_by_name(struct cycv_clkmgr_softc *sc, const char *name)
{
	size_t i;

	for (i = 0; i < sc->sc_nclocks; i++) {
		struct cycv_clk *cand = &sc->sc_clocks[i];
		if (strncmp(cand->base.name, name, strlen(name)) == 0)
			return cand;
	}

	return NULL;
}

static void
cycv_clkmgr_clock_print(struct cycv_clkmgr_softc *sc, struct cycv_clk *clk) {
	uint32_t numer;
	uint32_t denom;
	uint32_t tmp;

	aprint_debug("clock %s, id %d, frequency %uHz:\n", clk->base.name,
		     clk->id, cycv_clkmgr_clock_get_rate(sc, &clk->base));
	if (clk->parent != NULL)
		aprint_debug("parent: %s", clk->parent->base.name);
	else
		aprint_debug("parent_id: %d", clk->parent_id);
	aprint_debug(", flags: %d\n", clk->flags);
	switch (clk->type) {
	case CYCV_CLK_TYPE_PLL:
		tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh, clk->u.pll_addr);
		numer = __SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_NUMER) + 1;
		denom = __SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_DENOM) + 1;
		aprint_debug(" PLL num = %u, den = %u\n", numer, denom);
		break;
	case CYCV_CLK_TYPE_FIXED:
		aprint_debug(" Fixed frequency = %u\n", clk->u.fixed_freq);
		break;
	case CYCV_CLK_TYPE_FIXED_DIV:
		aprint_debug(" Fixed divisor = %u\n", clk->u.fixed_div);
		break;
	case CYCV_CLK_TYPE_DIV:
		tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh, clk->u.div.addr);
		tmp = (tmp & clk->u.div.mask) >> clk->u.div.shift;
		if (__SHIFTOUT_MASK(clk->u.div.mask) > 0xf)
			tmp += 1;
		else
			tmp = (1 << tmp);
		aprint_debug(" Divisor = %u\n", tmp);
		break;
	default:
		aprint_debug(" Unknown!!!\n");
		break;
	}

	if (clk->flags & CYCV_CLK_FLAG_HAVE_GATE) {
		tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh, clk->gate_addr);
		tmp &= (1 << clk->gate_shift);
		aprint_debug(" Gate %s\n", tmp? "on" : "off");
	}


}

static struct clk *
cycv_clkmgr_clock_decode(device_t dev, int cc_phandle, const void *data,
			 size_t len)
{
	struct cycv_clkmgr_softc *sc = device_private(dev);
	struct cycv_clk *clk;

	if (data != NULL || len != 0) {
		aprint_debug_dev(dev, "can't decode clock entry\n");
		return NULL;
	}

	clk = cycv_clkmgr_clock_lookup_by_id(sc, cc_phandle);

	return clk == NULL? NULL : &clk->base;
}

static struct clk *
cycv_clkmgr_clock_get(void *priv, const char *name)
{
	aprint_debug("%s: called and not implemented\n", __func__);
	(void) cycv_clkmgr_get_mux_info;
	(void) cycv_clkmgr_clock_lookup_by_name;

	return NULL;
}

static void
cycv_clkmgr_clock_put(void *priv, struct clk *clk)
{
	aprint_debug("%s: called and not implemented\n", __func__);
}

static u_int
cycv_clkmgr_clock_get_rate(void *priv, struct clk *base_clk)
{
	struct cycv_clkmgr_softc *sc = priv;
	struct cycv_clk *clk = (struct cycv_clk *) base_clk;
	struct cycv_clk *parent;
	uint32_t parent_rate = 0;
	uint32_t divisor = 0;
	uint32_t numer;
	uint32_t tmp;

	if (clk->type == CYCV_CLK_TYPE_FIXED)
		return clk->u.fixed_freq;

	parent = (struct cycv_clk *)
		cycv_clkmgr_clock_get_parent(priv, base_clk);
	if (parent == NULL) {
		aprint_debug_dev(sc->sc_dev, "can't get parent of clock %s\n",
				 clk->base.name);
		return 0;
	}
	parent_rate = cycv_clkmgr_clock_get_rate(priv, &parent->base);

	if (strncmp(clk->base.name, "mpuclk@", strlen("mpuclk@")) == 0)
		parent_rate /= 2;
	else if (strncmp(clk->base.name, "mainclk@", strlen("mainclk@")) == 0)
		parent_rate /= 4;
	else if (strncmp(clk->base.name, "dbgatclk@", strlen("dbgatclk@")) == 0)
		parent_rate /= 4;

	switch (clk->type) {
	case CYCV_CLK_TYPE_FIXED_DIV:
		return parent_rate / clk->u.fixed_div;

	case CYCV_CLK_TYPE_DIV:
		divisor = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
					   clk->u.div.addr);
		divisor = (divisor & clk->u.div.mask) >> clk->u.div.shift;
		if (__SHIFTOUT_MASK(clk->u.div.mask) > 0xf)
			divisor += 1;
		else
			divisor = (1 << divisor);

		return parent_rate / divisor;

	case CYCV_CLK_TYPE_PLL:
		tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh, clk->u.pll_addr);
		numer = __SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_NUMER) + 1;
		divisor = __SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_DENOM) + 1;

		return (uint64_t) parent_rate * numer / divisor;
	}

	aprint_debug_dev(sc->sc_dev, "unknown clock type %d\n", clk->type);

	return 0;
}

static int
cycv_clkmgr_clock_set_rate(void *priv, struct clk *clk, u_int rate)
{
	aprint_debug("%s: called and not implemented\n", __func__);
	return EINVAL;
}

static int
cycv_clkmgr_clock_set(void *priv, struct clk *base_clk, int val) {
	struct cycv_clkmgr_softc *sc = priv;
	struct cycv_clk *clk = (struct cycv_clk *) base_clk;

	if (clk->flags & CYCV_CLK_FLAG_HAVE_GATE) {
		uint32_t tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
			clk->gate_addr);
		tmp &= ~(1 << clk->gate_shift);
		tmp |= val << clk->gate_shift;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, clk->gate_addr, tmp);
	} else
		/* XXX should iterate to the root of the clock domain */
		return 0;

	return 0;
}

static int
cycv_clkmgr_clock_enable(void *priv, struct clk *clk)
{
	return cycv_clkmgr_clock_set(priv, clk, 1);
}

static int
cycv_clkmgr_clock_disable(void *priv, struct clk *clk)
{
	return cycv_clkmgr_clock_set(priv, clk, 0);
}

static int
cycv_clkmgr_clock_set_parent(void *priv, struct clk *clk,
    struct clk *clk_parent)
{
	/* lookup clk in muxinfo table */
	/* if not found, parent is not settable */
	/* check if clk_parent can be a parent (by name) */
	/* beware of special case where there is only one parent in mux */
	/* enact reparenting h/w wise, update clk->parent */
	aprint_debug("%s: called and not implemented\n", __func__);
	return EINVAL;
}

static struct clk *
cycv_clkmgr_clock_get_parent(void *priv, struct clk *base_clk)
{
	struct cycv_clkmgr_softc *sc = priv;
	struct cycv_clk *clk = (struct cycv_clk *) base_clk;
	struct cycv_clk_mux_info *mux;
	struct cycv_clk *parent = clk->parent;
	int parent_index;
	uint32_t tmp;

	if (parent != NULL)
		goto update;

	if (clk->parent_id != 0) {
		parent = cycv_clkmgr_clock_lookup_by_id(sc, clk->parent_id);
		goto update;
	}

	mux = cycv_clkmgr_get_mux_info(clk->base.name);

	if (mux == NULL)
		goto update;

	if (mux->nparents == 1)
		parent_index = 0;
	else {
		tmp = bus_space_read_4(sc->sc_bst, sc->sc_bsh, mux->addr);
		parent_index = __SHIFTOUT(tmp, mux->mask);
	}

	if (parent_index >= mux->nparents) {
		aprint_error_dev(sc->sc_dev,
				 "clock %s parent has non existent index %d\n",
				 clk->base.name, parent_index);
		goto update;
	}

	parent = cycv_clkmgr_clock_lookup_by_name(sc,
						  mux->parents[parent_index]);

update:
	clk->parent = parent;
	return &parent->base;
}

/*
 * Functions called during early startup, possibly before autoconfiguration.
 */

uint32_t
cycv_clkmgr_early_get_mpu_clk(void) {
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t tmp;
	uint64_t vco;

	bus_space_map(bst, CYCV_CLKMGR_BASE, CYCV_CLKMGR_SIZE, 0, &bsh);

	tmp = bus_space_read_4(bst, bsh, CYCV_CLKMGR_MAIN_PLL_VCO);
	vco = (uint64_t) CYCV_CLOCK_OSC1 *
		(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_NUMER) + 1) /
		(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_DENOM) + 1);
	tmp = bus_space_read_4(bst, bsh, CYCV_CLKMGR_MAIN_PLL_MPUCLK);

	return vco / 2 / (__SHIFTOUT(tmp, CYCV_CLKMGR_MAIN_PLL_MPUCLK_CNT) + 1);
}

uint32_t
cycv_clkmgr_early_get_l4_sp_clk(void) {
	bus_space_tag_t bst = &armv7_generic_bs_tag;
	bus_space_handle_t bsh;
	uint32_t tmp;
	uint32_t res;
	uint64_t vco;

	bus_space_map(bst, CYCV_CLKMGR_BASE, CYCV_CLKMGR_SIZE, 0, &bsh);

	tmp = bus_space_read_4(bst, bsh, CYCV_CLKMGR_MAIN_PLL_L4SRC);
	if (__SHIFTOUT(tmp, CYCV_CLKMGR_MAIN_PLL_L4SRC_L4SP) == 0) {
		/* L4 SP clock driven by main clock */
		vco = (uint64_t) CYCV_CLOCK_OSC1 *
			(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_NUMER) + 1) /
			(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_DENOM) + 1);
		tmp = bus_space_read_4(bst, bsh, CYCV_CLKMGR_MAIN_PLL_MAINCLK);
		tmp = __SHIFTOUT(tmp, CYCV_CLKMGR_MAIN_PLL_MAINCLK_CNT);

		res = vco / 4 / (tmp + 1);
	} else {
		/* L4 SP clock driven by periph clock */
		vco = (uint64_t) CYCV_CLOCK_OSC1 *
			(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_NUMER) + 1) /
			(__SHIFTOUT(tmp, CYCV_CLKMGR_PLL_VCO_DENOM) + 1);
		tmp = bus_space_read_4(bst, bsh,
			CYCV_CLKMGR_PERI_PLL_PERBASECLK);
		tmp = __SHIFTOUT(tmp, CYCV_CLKMGR_PERI_PLL_PERBASECLK_CNT);

		res = vco / (tmp + 1);
	}

	tmp = bus_space_read_4(bst, bsh, CYCV_CLKMGR_MAIN_PLL_MAINDIV);
	tmp = __SHIFTOUT(tmp, CYCV_CLKMGR_MAIN_PLL_MAINDIV_L4SP);

	printf("%s: returning %u\n", __func__, (uint32_t)
		(res / (1 << tmp)));
	return res / (1 << tmp);
}
