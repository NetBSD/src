/* $NetBSD: fdt_clock.c,v 1.1.20.1 2018/06/25 07:25:49 pgoyette Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_clock.c,v 1.1.20.1 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

struct fdtbus_clock_controller {
	device_t cc_dev;
	int cc_phandle;
	const struct fdtbus_clock_controller_func *cc_funcs;

	struct fdtbus_clock_controller *cc_next;
};

static struct fdtbus_clock_controller *fdtbus_cc = NULL;

int
fdtbus_register_clock_controller(device_t dev, int phandle,
    const struct fdtbus_clock_controller_func *funcs)
{
	struct fdtbus_clock_controller *cc;

	cc = kmem_alloc(sizeof(*cc), KM_SLEEP);
	cc->cc_dev = dev;
	cc->cc_phandle = phandle;
	cc->cc_funcs = funcs;

	cc->cc_next = fdtbus_cc;
	fdtbus_cc = cc;

	fdtbus_clock_assign(phandle);

	return 0;
}

static struct fdtbus_clock_controller *
fdtbus_get_clock_controller(int phandle)
{
	struct fdtbus_clock_controller *cc;

	for (cc = fdtbus_cc; cc; cc = cc->cc_next) {
		if (cc->cc_phandle == phandle) {
			return cc;
		}
	}

	return NULL;
}

static struct clk *
fdtbus_clock_get_index_prop(int phandle, u_int index, const char *prop)
{
	struct fdtbus_clock_controller *cc;
	struct clk *clk = NULL;
	const u_int *p;
	u_int n, clock_cells;
	int len, resid;

	p = fdtbus_get_prop(phandle, prop, &len);
	if (p == NULL)
		return NULL;

	for (n = 0, resid = len; resid > 0; n++) {
		const int cc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(cc_phandle, "#clock-cells", &clock_cells))
			break;
		if (n == index) {
			cc = fdtbus_get_clock_controller(cc_phandle);
			if (cc == NULL)
				break;
			clk = cc->cc_funcs->decode(cc->cc_dev,
			    clock_cells > 0 ? &p[1] : NULL, clock_cells * 4);
			break;
		}
		resid -= (clock_cells + 1) * 4;
		p += clock_cells + 1;
	}

	return clk;
}

struct clk *
fdtbus_clock_get_index(int phandle, u_int index)
{
	return fdtbus_clock_get_index_prop(phandle, index, "clocks");
}

static struct clk *
fdtbus_clock_get_prop(int phandle, const char *clkname, const char *prop)
{
	struct clk *clk = NULL;
	const char *p;
	u_int index;
	int len, resid;

	p = fdtbus_get_prop(phandle, prop, &len);
	if (p == NULL)
		return NULL;

	for (index = 0, resid = len; resid > 0; index++) {
		if (strcmp(p, clkname) == 0) {
			clk = fdtbus_clock_get_index(phandle, index);
			break;
		}
		resid -= strlen(p);
		p += strlen(p) + 1;
	}

	return clk;
}

static u_int
fdtbus_clock_count_prop(int phandle, const char *prop)
{
	u_int n, clock_cells;
	int len, resid;

	const u_int *p = fdtbus_get_prop(phandle, prop, &len);
	if (p == NULL)
		return 0;

	for (n = 0, resid = len; resid > 0; n++) {
		const int cc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(cc_phandle, "#clock-cells", &clock_cells))
			break;
		resid -= (clock_cells + 1) * 4;
		p += clock_cells + 1;
	}

	return n;
}

struct clk *
fdtbus_clock_get(int phandle, const char *clkname)
{
	return fdtbus_clock_get_prop(phandle, clkname, "clock-names");
}

/*
 * Search the DT for a clock by "clock-output-names" property.
 *
 * This should only be used by clk backends. Not for use by ordinary
 * clock consumers!
 */
struct clk *
fdtbus_clock_byname(const char *clkname)
{
	struct fdtbus_clock_controller *cc;
	u_int len, resid, index, clock_cells;
	const char *p;

	for (cc = fdtbus_cc; cc; cc = cc->cc_next) {
		if (!of_hasprop(cc->cc_phandle, "clock-output-names"))
			continue;
		p = fdtbus_get_prop(cc->cc_phandle, "clock-output-names", &len);
		for (index = 0, resid = len; resid > 0; index++) {
			if (strcmp(p, clkname) == 0) {
				if (of_getprop_uint32(cc->cc_phandle, "#clock-cells", &clock_cells))
					break;
				const u_int index_raw = htobe32(index);
				return cc->cc_funcs->decode(cc->cc_dev,
				    clock_cells > 0 ? &index_raw : NULL,
				    clock_cells > 0 ? 4 : 0);
			}
			resid -= strlen(p) + 1;
			p += strlen(p) + 1;
		}
	}

	return NULL;
}

/*
 * Apply assigned clock parents and rates.
 *
 * This is automatically called by fdtbus_register_clock_controller, so clock
 * drivers likely don't need to call this directly.
 */
void
fdtbus_clock_assign(int phandle)
{
	u_int index, rates_len;
	struct clk *clk, *clk_parent;
	int error;

	const u_int *rates = fdtbus_get_prop(phandle, "assigned-clock-rates", &rates_len);
	if (rates == NULL)
		rates_len = 0;

	const u_int nclocks = fdtbus_clock_count_prop(phandle, "assigned-clocks");
	const u_int nparents = fdtbus_clock_count_prop(phandle, "assigned-clock-parents");
	const u_int nrates = rates_len / sizeof(*rates);

	for (index = 0; index < nclocks; index++) {
		clk = fdtbus_clock_get_index_prop(phandle, index, "assigned-clocks");
		if (clk == NULL) {
			aprint_debug("clk: assigned clock (%u) not found, skipping...\n", index);
			continue;
		}

		if (index < nparents) {
			clk_parent = fdtbus_clock_get_index_prop(phandle, index, "assigned-clock-parents");
			if (clk_parent != NULL) {
				error = clk_set_parent(clk, clk_parent);
				if (error != 0) {
					aprint_error("clk: failed to set %s parent to %s, error %d\n",
					    clk->name, clk_parent->name, error);
				}
			} else {
				aprint_debug("clk: failed to set %s parent (not found)\n", clk->name);
			}
		}

		if (index < nrates) {
			const u_int rate = be32toh(rates[index]);
			if (rate != 0) {
				error = clk_set_rate(clk, rate);
				if (error != 0)
					aprint_error("clk: failed to set %s rate to %u Hz, error %d\n",
					    clk->name, rate, error);
			}
		}
	}
}
