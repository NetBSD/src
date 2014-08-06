/*	$NetBSD: nouveau_subdev_devinit_nvc0.c,v 1.1.1.1 2014/08/06 12:36:30 riastradh Exp $	*/

/*
 * Copyright 2013 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nouveau_subdev_devinit_nvc0.c,v 1.1.1.1 2014/08/06 12:36:30 riastradh Exp $");

#include "nv50.h"

int
nvc0_devinit_pll_set(struct nouveau_devinit *devinit, u32 type, u32 freq)
{
	struct nv50_devinit_priv *priv = (void *)devinit;
	struct nouveau_bios *bios = nouveau_bios(priv);
	struct nvbios_pll info;
	int N, fN, M, P;
	int ret;

	ret = nvbios_pll_parse(bios, type, &info);
	if (ret)
		return ret;

	ret = nva3_pll_calc(nv_subdev(devinit), &info, freq, &N, &fN, &M, &P);
	if (ret < 0)
		return ret;

	switch (info.type) {
	case PLL_VPLL0:
	case PLL_VPLL1:
	case PLL_VPLL2:
	case PLL_VPLL3:
		nv_mask(priv, info.reg + 0x0c, 0x00000000, 0x00000100);
		nv_wr32(priv, info.reg + 0x04, (P << 16) | (N << 8) | M);
		nv_wr32(priv, info.reg + 0x10, fN << 16);
		break;
	default:
		nv_warn(priv, "0x%08x/%dKhz unimplemented\n", type, freq);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static u64
nvc0_devinit_disable(struct nouveau_devinit *devinit)
{
	struct nv50_devinit_priv *priv = (void *)devinit;
	u32 r022500 = nv_rd32(priv, 0x022500);
	u64 disable = 0ULL;

	if (r022500 & 0x00000001)
		disable |= (1ULL << NVDEV_ENGINE_DISP);

	if (r022500 & 0x00000002) {
		disable |= (1ULL << NVDEV_ENGINE_VP);
		disable |= (1ULL << NVDEV_ENGINE_PPP);
	}

	if (r022500 & 0x00000004)
		disable |= (1ULL << NVDEV_ENGINE_BSP);
	if (r022500 & 0x00000008)
		disable |= (1ULL << NVDEV_ENGINE_VENC);
	if (r022500 & 0x00000100)
		disable |= (1ULL << NVDEV_ENGINE_COPY0);
	if (r022500 & 0x00000200)
		disable |= (1ULL << NVDEV_ENGINE_COPY1);

	return disable;
}

static int
nvc0_devinit_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
		  struct nouveau_oclass *oclass, void *data, u32 size,
		  struct nouveau_object **pobject)
{
	struct nv50_devinit_priv *priv;
	int ret;

	ret = nouveau_devinit_create(parent, engine, oclass, &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

	if (nv_rd32(priv, 0x022500) & 0x00000001)
		priv->base.post = true;
	return 0;
}

struct nouveau_oclass *
nvc0_devinit_oclass = &(struct nouveau_devinit_impl) {
	.base.handle = NV_SUBDEV(DEVINIT, 0xc0),
	.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nvc0_devinit_ctor,
		.dtor = _nouveau_devinit_dtor,
		.init = nv50_devinit_init,
		.fini = _nouveau_devinit_fini,
	},
	.pll_set = nvc0_devinit_pll_set,
	.disable = nvc0_devinit_disable,
}.base;
