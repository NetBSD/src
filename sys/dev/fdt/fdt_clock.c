/* $NetBSD: fdt_clock.c,v 1.1.18.2 2017/12/03 11:37:01 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_clock.c,v 1.1.18.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

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

struct clk *
fdtbus_clock_get_index(int phandle, u_int index)
{
	struct fdtbus_clock_controller *cc;
	struct clk *clk = NULL;
	uint32_t *clocks = NULL;
	uint32_t *p;
	u_int n, clock_cells;
	int len, resid;

	len = OF_getproplen(phandle, "clocks");
	if (len <= 0)
		return NULL;

	clocks = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "clocks", clocks, len) != len) {
		kmem_free(clocks, len);
		return NULL;
	}

	p = clocks;
	for (n = 0, resid = len; resid > 0; n++) {
		const int cc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(cc_phandle, "#clock-cells", &clock_cells))
			break;
		if (n == index) {
			cc = fdtbus_get_clock_controller(cc_phandle);
			if (cc == NULL)
				goto done;
			clk = cc->cc_funcs->decode(cc->cc_dev,
			    clock_cells > 0 ? &p[1] : NULL, clock_cells * 4);
			break;
		}
		resid -= (clock_cells + 1) * 4;
		p += clock_cells + 1;
	}

done:
	if (clocks)
		kmem_free(clocks, len);

	return clk;
}

struct clk *
fdtbus_clock_get(int phandle, const char *clkname)
{
	struct clk *clk = NULL;
	char *clock_names = NULL;
	const char *p;
	u_int index;
	int len, resid;

	len = OF_getproplen(phandle, "clock-names");
	if (len <= 0)
		return NULL;

	clock_names = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "clock-names", clock_names, len) != len) {
		kmem_free(clock_names, len);
		return NULL;
	}

	p = clock_names;
	for (index = 0, resid = len; resid > 0; index++) {
		if (strcmp(p, clkname) == 0) {
			clk = fdtbus_clock_get_index(phandle, index);
			break;
		}
		resid -= strlen(p);
		p += strlen(p) + 1;
	}

	if (clock_names)
		kmem_free(clock_names, len);

	return clk;
}
