/* $NetBSD: sunxi_ccu_mux.c,v 1.1 2021/11/07 17:13:12 jmcneill Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_ccu_mux.c,v 1.1 2021/11/07 17:13:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/sunxi/sunxi_ccu.h>

int
sunxi_ccu_mux_set_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk, const char *name)
{
	struct sunxi_ccu_mux *mux = &clk->u.mux;
	uint32_t val;
	u_int index;

	KASSERT(clk->type == SUNXI_CCU_MUX);

	if (mux->sel == 0)
		return ENODEV;

	for (index = 0; index < mux->nparents; index++) {
		if (mux->parents[index] != NULL &&
		    strcmp(mux->parents[index], name) == 0)
			break;
	}
	if (index == mux->nparents)
		return EINVAL;

	val = CCU_READ(sc, mux->reg);
	val &= ~mux->sel;
	val |= __SHIFTIN(index, mux->sel);
	CCU_WRITE(sc, mux->reg, val);

	return 0;
}

const char *
sunxi_ccu_mux_get_parent(struct sunxi_ccu_softc *sc,
    struct sunxi_ccu_clk *clk)
{
	struct sunxi_ccu_mux *mux = &clk->u.mux;
	u_int index;
	uint32_t val;

	KASSERT(clk->type == SUNXI_CCU_MUX);

	val = CCU_READ(sc, mux->reg);
	index = __SHIFTOUT(val, mux->sel);

	return mux->parents[index];
}
