/* $NetBSD: rk_cru_mux.c,v 1.1.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: rk_cru_mux.c,v 1.1.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/rockchip/rk_cru.h>

const char *
rk_cru_mux_get_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk)
{
	struct rk_cru_mux *mux = &clk->u.mux;
	const bool mux_grf = (mux->flags & RK_MUX_GRF) != 0;

	KASSERT(clk->type == RK_CRU_MUX);

	if (mux_grf && !HAS_GRF(sc))
		return NULL;

	const uint32_t val = mux_grf ? GRF_READ(sc, mux->reg) : CRU_READ(sc, mux->reg);
	const u_int index = __SHIFTOUT(val, mux->mask);

	return mux->parents[index];
}

int
rk_cru_mux_set_parent(struct rk_cru_softc *sc,
    struct rk_cru_clk *clk, const char *parent)
{
	struct rk_cru_mux *mux = &clk->u.mux;
	const bool mux_grf = (mux->flags & RK_MUX_GRF) != 0;

	KASSERT(clk->type == RK_CRU_MUX);

	if (mux_grf && !HAS_GRF(sc))
		return ENXIO;

	for (u_int index = 0; index < mux->nparents; index++) {
		if (strcmp(mux->parents[index], parent) == 0) {
			const uint32_t write_mask = mux->mask << 16;
			const uint32_t write_val = __SHIFTIN(index, mux->mask);

			if (mux_grf)
				GRF_WRITE(sc, mux->reg, write_mask | write_val);
			else
				CRU_WRITE(sc, mux->reg, write_mask | write_val);

			return 0;
		}
	}

	return EINVAL;
}
