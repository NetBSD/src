/*	$NetBSD: nouveau_engine_disp_gm107.c,v 1.1.1.1 2014/08/06 12:36:24 riastradh Exp $	*/

/*
 * Copyright 2012 Red Hat Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nouveau_engine_disp_gm107.c,v 1.1.1.1 2014/08/06 12:36:24 riastradh Exp $");

#include <engine/software.h>
#include <engine/disp.h>

#include <core/class.h>

#include "nv50.h"

/*******************************************************************************
 * Base display object
 ******************************************************************************/

static struct nouveau_oclass
gm107_disp_sclass[] = {
	{ GM107_DISP_MAST_CLASS, &nvd0_disp_mast_ofuncs },
	{ GM107_DISP_SYNC_CLASS, &nvd0_disp_sync_ofuncs },
	{ GM107_DISP_OVLY_CLASS, &nvd0_disp_ovly_ofuncs },
	{ GM107_DISP_OIMM_CLASS, &nvd0_disp_oimm_ofuncs },
	{ GM107_DISP_CURS_CLASS, &nvd0_disp_curs_ofuncs },
	{}
};

static struct nouveau_oclass
gm107_disp_base_oclass[] = {
	{ GM107_DISP_CLASS, &nvd0_disp_base_ofuncs, nvd0_disp_base_omthds },
	{}
};

/*******************************************************************************
 * Display engine implementation
 ******************************************************************************/

static int
gm107_disp_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
	       struct nouveau_oclass *oclass, void *data, u32 size,
	       struct nouveau_object **pobject)
{
	struct nv50_disp_priv *priv;
	int heads = nv_rd32(parent, 0x022448);
	int ret;

	ret = nouveau_disp_create(parent, engine, oclass, heads,
				  "PDISP", "display", &priv);
	*pobject = nv_object(priv);
	if (ret)
		return ret;

	nv_engine(priv)->sclass = gm107_disp_base_oclass;
	nv_engine(priv)->cclass = &nv50_disp_cclass;
	nv_subdev(priv)->intr = nvd0_disp_intr;
	INIT_WORK(&priv->supervisor, nvd0_disp_intr_supervisor);
	priv->sclass = gm107_disp_sclass;
	priv->head.nr = heads;
	priv->dac.nr = 3;
	priv->sor.nr = 4;
	priv->dac.power = nv50_dac_power;
	priv->dac.sense = nv50_dac_sense;
	priv->sor.power = nv50_sor_power;
	priv->sor.hda_eld = nvd0_hda_eld;
	priv->sor.hdmi = nvd0_hdmi_ctrl;
	priv->sor.dp = &nvd0_sor_dp_func;
	return 0;
}

struct nouveau_oclass *
gm107_disp_oclass = &(struct nv50_disp_impl) {
	.base.base.handle = NV_ENGINE(DISP, 0x07),
	.base.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = gm107_disp_ctor,
		.dtor = _nouveau_disp_dtor,
		.init = _nouveau_disp_init,
		.fini = _nouveau_disp_fini,
	},
	.mthd.core = &nve0_disp_mast_mthd_chan,
	.mthd.base = &nvd0_disp_sync_mthd_chan,
	.mthd.ovly = &nve0_disp_ovly_mthd_chan,
	.mthd.prev = -0x020000,
}.base.base;
