/* $NetBSD: meson_clk_mux.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2017-2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: meson_clk_mux.c,v 1.2 2019/02/25 19:30:17 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/amlogic/meson_clk.h>

const char *
meson_clk_mux_get_parent(struct meson_clk_softc *sc, struct meson_clk_clk *clk)
{
	struct meson_clk_mux *mux = &clk->u.mux;
	uint32_t val;
	u_int sel;

	KASSERT(clk->type == MESON_CLK_MUX);

	CLK_LOCK(sc);
	val = CLK_READ(sc, mux->reg);
	CLK_UNLOCK(sc);

	sel = __SHIFTOUT(val, mux->sel);
	if (sel >= mux->nparents)
		return NULL;

	return mux->parents[sel];
}
