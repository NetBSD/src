/* $NetBSD: imx_ccm_extclk.c,v 1.1.2.2 2020/01/17 21:47:24 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: imx_ccm_extclk.c,v 1.1.2.2 2020/01/17 21:47:24 ad Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <arm/imx/fdt/imx_ccm.h>

#include <dev/fdt/fdtvar.h>

int
imx_ccm_extclk_enable(struct imx_ccm_softc *sc, struct imx_ccm_clk *clk,
    int enable)
{
	struct clk *extclk;

	KASSERT(clk->type == IMX_CCM_EXTCLK);

	if ((extclk = fdtbus_clock_byname(clk->u.extclk)) == NULL)
		return ENXIO;

	return clk_enable(extclk);
}

u_int
imx_ccm_extclk_get_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct clk *extclk;

	KASSERT(clk->type == IMX_CCM_EXTCLK);

	if ((extclk = fdtbus_clock_byname(clk->u.extclk)) == NULL)
		return 0;

	return clk_get_rate(extclk);
}

int
imx_ccm_extclk_set_rate(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk, u_int rate)
{
	struct clk *extclk;

	KASSERT(clk->type == IMX_CCM_EXTCLK);

	if ((extclk = fdtbus_clock_byname(clk->u.extclk)) == NULL)
		return ENXIO;

	return clk_set_rate(extclk, rate);
}

const char *
imx_ccm_extclk_get_parent(struct imx_ccm_softc *sc,
    struct imx_ccm_clk *clk)
{
	struct clk *extclk, *clkp;

	KASSERT(clk->type == IMX_CCM_EXTCLK);

	if ((extclk = fdtbus_clock_byname(clk->u.extclk)) == NULL)
		return NULL;

	clkp = clk_get_parent(extclk);
	if (clkp == NULL)
		return NULL;

	return clkp->name;
}
