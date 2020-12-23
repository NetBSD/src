/* $NetBSD: imx_ccm_mux.c,v 1.1 2020/12/23 14:42:38 skrll Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_mux.c,v 1.1 2020/12/23 14:42:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/nxp/imx_ccm.h>

#include <dev/fdt/fdtvar.h>

const char *
imx_ccm_mux_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct imx_ccm_mux *mux = &clk->u.mux;

	KASSERT(clk->type == IMX_CCM_MUX);

	const uint32_t val = CCM_READ(sc, clk->regidx, mux->reg);
	const u_int sel = __SHIFTOUT(val, mux->sel);

	if (sel >= mux->nparents)
		return NULL;

	return mux->parents[sel];
}

int
imx_ccm_mux_set_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, const char *parent)
{
	struct imx_ccm_mux *mux = &clk->u.mux;
	uint32_t val;

	KASSERT(clk->type == IMX_CCM_MUX);

	for (u_int sel = 0; sel < mux->nparents; sel++) {
		if (strcmp(mux->parents[sel], parent) == 0) {
			val = CCM_READ(sc, clk->regidx, mux->reg);
			val &= ~mux->sel;
			val |= __SHIFTIN(sel, mux->sel);
			CCM_WRITE(sc, clk->regidx, mux->reg, val);
			return 0;
		}
	}

	return EINVAL;
}
