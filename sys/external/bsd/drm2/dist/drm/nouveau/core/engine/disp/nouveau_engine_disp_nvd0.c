/*	$NetBSD: nouveau_engine_disp_nvd0.c,v 1.2.2.1 2016/05/29 08:44:36 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_engine_disp_nvd0.c,v 1.2.2.1 2016/05/29 08:44:36 skrll Exp $");

#include <core/object.h>
#include <core/parent.h>
#include <core/handle.h>
#include <core/class.h>

#include <engine/disp.h>

#include <subdev/bios.h>
#include <subdev/bios/dcb.h>
#include <subdev/bios/disp.h>
#include <subdev/bios/init.h>
#include <subdev/bios/pll.h>
#include <subdev/devinit.h>
#include <subdev/fb.h>
#include <subdev/timer.h>

#include <asm/div64.h>	/* XXX */
#include <linux/ktime.h>	/* XXX */

#include "nv50.h"

/*******************************************************************************
 * EVO DMA channel base class
 ******************************************************************************/

static int
nvd0_disp_dmac_object_attach(struct nouveau_object *parent,
			     struct nouveau_object *object, u32 name)
{
	struct nv50_disp_base *base = (void *)parent->parent;
	struct nv50_disp_chan *chan = (void *)parent;
	u32 addr = nv_gpuobj(object)->node->offset;
	u32 data = (chan->chid << 27) | (addr << 9) | 0x00000001;
	return nouveau_ramht_insert(base->ramht, chan->chid, name, data);
}

static void
nvd0_disp_dmac_object_detach(struct nouveau_object *parent, int cookie)
{
	struct nv50_disp_base *base = (void *)parent->parent;
	nouveau_ramht_remove(base->ramht, cookie);
}

static int
nvd0_disp_dmac_init(struct nouveau_object *object)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_dmac *dmac = (void *)object;
	int chid = dmac->base.chid;
	int ret;

	ret = nv50_disp_chan_init(&dmac->base);
	if (ret)
		return ret;

	/* enable error reporting */
	nv_mask(priv, 0x610090, 0x00000001 << chid, 0x00000001 << chid);
	nv_mask(priv, 0x6100a0, 0x00000001 << chid, 0x00000001 << chid);

	/* initialise channel for dma command submission */
	nv_wr32(priv, 0x610494 + (chid * 0x0010), dmac->push);
	nv_wr32(priv, 0x610498 + (chid * 0x0010), 0x00010000);
	nv_wr32(priv, 0x61049c + (chid * 0x0010), 0x00000001);
	nv_mask(priv, 0x610490 + (chid * 0x0010), 0x00000010, 0x00000010);
	nv_wr32(priv, 0x640000 + (chid * 0x1000), 0x00000000);
	nv_wr32(priv, 0x610490 + (chid * 0x0010), 0x00000013);

	/* wait for it to go inactive */
	if (!nv_wait(priv, 0x610490 + (chid * 0x10), 0x80000000, 0x00000000)) {
		nv_error(dmac, "init: 0x%08x\n",
			 nv_rd32(priv, 0x610490 + (chid * 0x10)));
		return -EBUSY;
	}

	return 0;
}

static int
nvd0_disp_dmac_fini(struct nouveau_object *object, bool suspend)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_dmac *dmac = (void *)object;
	int chid = dmac->base.chid;

	/* deactivate channel */
	nv_mask(priv, 0x610490 + (chid * 0x0010), 0x00001010, 0x00001000);
	nv_mask(priv, 0x610490 + (chid * 0x0010), 0x00000003, 0x00000000);
	if (!nv_wait(priv, 0x610490 + (chid * 0x10), 0x001e0000, 0x00000000)) {
		nv_error(dmac, "fini: 0x%08x\n",
			 nv_rd32(priv, 0x610490 + (chid * 0x10)));
		if (suspend)
			return -EBUSY;
	}

	/* disable error reporting */
	nv_mask(priv, 0x610090, 0x00000001 << chid, 0x00000000);
	nv_mask(priv, 0x6100a0, 0x00000001 << chid, 0x00000000);

	return nv50_disp_chan_fini(&dmac->base, suspend);
}

/*******************************************************************************
 * EVO master channel object
 ******************************************************************************/

const struct nv50_disp_mthd_list
nvd0_disp_mast_mthd_base = {
	.mthd = 0x0000,
	.addr = 0x000000,
	.data = {
		{ 0x0080, 0x660080 },
		{ 0x0084, 0x660084 },
		{ 0x0088, 0x660088 },
		{ 0x008c, 0x000000 },
		{}
	}
};

const struct nv50_disp_mthd_list
nvd0_disp_mast_mthd_dac = {
	.mthd = 0x0020,
	.addr = 0x000020,
	.data = {
		{ 0x0180, 0x660180 },
		{ 0x0184, 0x660184 },
		{ 0x0188, 0x660188 },
		{ 0x0190, 0x660190 },
		{}
	}
};

const struct nv50_disp_mthd_list
nvd0_disp_mast_mthd_sor = {
	.mthd = 0x0020,
	.addr = 0x000020,
	.data = {
		{ 0x0200, 0x660200 },
		{ 0x0204, 0x660204 },
		{ 0x0208, 0x660208 },
		{ 0x0210, 0x660210 },
		{}
	}
};

const struct nv50_disp_mthd_list
nvd0_disp_mast_mthd_pior = {
	.mthd = 0x0020,
	.addr = 0x000020,
	.data = {
		{ 0x0300, 0x660300 },
		{ 0x0304, 0x660304 },
		{ 0x0308, 0x660308 },
		{ 0x0310, 0x660310 },
		{}
	}
};

static const struct nv50_disp_mthd_list
nvd0_disp_mast_mthd_head = {
	.mthd = 0x0300,
	.addr = 0x000300,
	.data = {
		{ 0x0400, 0x660400 },
		{ 0x0404, 0x660404 },
		{ 0x0408, 0x660408 },
		{ 0x040c, 0x66040c },
		{ 0x0410, 0x660410 },
		{ 0x0414, 0x660414 },
		{ 0x0418, 0x660418 },
		{ 0x041c, 0x66041c },
		{ 0x0420, 0x660420 },
		{ 0x0424, 0x660424 },
		{ 0x0428, 0x660428 },
		{ 0x042c, 0x66042c },
		{ 0x0430, 0x660430 },
		{ 0x0434, 0x660434 },
		{ 0x0438, 0x660438 },
		{ 0x0440, 0x660440 },
		{ 0x0444, 0x660444 },
		{ 0x0448, 0x660448 },
		{ 0x044c, 0x66044c },
		{ 0x0450, 0x660450 },
		{ 0x0454, 0x660454 },
		{ 0x0458, 0x660458 },
		{ 0x045c, 0x66045c },
		{ 0x0460, 0x660460 },
		{ 0x0468, 0x660468 },
		{ 0x046c, 0x66046c },
		{ 0x0470, 0x660470 },
		{ 0x0474, 0x660474 },
		{ 0x0480, 0x660480 },
		{ 0x0484, 0x660484 },
		{ 0x048c, 0x66048c },
		{ 0x0490, 0x660490 },
		{ 0x0494, 0x660494 },
		{ 0x0498, 0x660498 },
		{ 0x04b0, 0x6604b0 },
		{ 0x04b8, 0x6604b8 },
		{ 0x04bc, 0x6604bc },
		{ 0x04c0, 0x6604c0 },
		{ 0x04c4, 0x6604c4 },
		{ 0x04c8, 0x6604c8 },
		{ 0x04d0, 0x6604d0 },
		{ 0x04d4, 0x6604d4 },
		{ 0x04e0, 0x6604e0 },
		{ 0x04e4, 0x6604e4 },
		{ 0x04e8, 0x6604e8 },
		{ 0x04ec, 0x6604ec },
		{ 0x04f0, 0x6604f0 },
		{ 0x04f4, 0x6604f4 },
		{ 0x04f8, 0x6604f8 },
		{ 0x04fc, 0x6604fc },
		{ 0x0500, 0x660500 },
		{ 0x0504, 0x660504 },
		{ 0x0508, 0x660508 },
		{ 0x050c, 0x66050c },
		{ 0x0510, 0x660510 },
		{ 0x0514, 0x660514 },
		{ 0x0518, 0x660518 },
		{ 0x051c, 0x66051c },
		{ 0x052c, 0x66052c },
		{ 0x0530, 0x660530 },
		{ 0x054c, 0x66054c },
		{ 0x0550, 0x660550 },
		{ 0x0554, 0x660554 },
		{ 0x0558, 0x660558 },
		{ 0x055c, 0x66055c },
		{}
	}
};

static const struct nv50_disp_mthd_chan
nvd0_disp_mast_mthd_chan = {
	.name = "Core",
	.addr = 0x000000,
	.data = {
		{ "Global", 1, &nvd0_disp_mast_mthd_base },
		{    "DAC", 3, &nvd0_disp_mast_mthd_dac  },
		{    "SOR", 8, &nvd0_disp_mast_mthd_sor  },
		{   "PIOR", 4, &nvd0_disp_mast_mthd_pior },
		{   "HEAD", 4, &nvd0_disp_mast_mthd_head },
		{}
	}
};

static int
nvd0_disp_mast_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_display_mast_class *args = data;
	struct nv50_disp_dmac *mast;
	int ret;

	if (size < sizeof(*args))
		return -EINVAL;

	ret = nv50_disp_dmac_create_(parent, engine, oclass, args->pushbuf,
				     0, sizeof(*mast), (void **)&mast);
	*pobject = nv_object(mast);
	if (ret)
		return ret;

	nv_parent(mast)->object_attach = nvd0_disp_dmac_object_attach;
	nv_parent(mast)->object_detach = nvd0_disp_dmac_object_detach;
	return 0;
}

static int
nvd0_disp_mast_init(struct nouveau_object *object)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_dmac *mast = (void *)object;
	int ret;

	ret = nv50_disp_chan_init(&mast->base);
	if (ret)
		return ret;

	/* enable error reporting */
	nv_mask(priv, 0x610090, 0x00000001, 0x00000001);
	nv_mask(priv, 0x6100a0, 0x00000001, 0x00000001);

	/* initialise channel for dma command submission */
	nv_wr32(priv, 0x610494, mast->push);
	nv_wr32(priv, 0x610498, 0x00010000);
	nv_wr32(priv, 0x61049c, 0x00000001);
	nv_mask(priv, 0x610490, 0x00000010, 0x00000010);
	nv_wr32(priv, 0x640000, 0x00000000);
	nv_wr32(priv, 0x610490, 0x01000013);

	/* wait for it to go inactive */
	if (!nv_wait(priv, 0x610490, 0x80000000, 0x00000000)) {
		nv_error(mast, "init: 0x%08x\n", nv_rd32(priv, 0x610490));
		return -EBUSY;
	}

	return 0;
}

static int
nvd0_disp_mast_fini(struct nouveau_object *object, bool suspend)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_dmac *mast = (void *)object;

	/* deactivate channel */
	nv_mask(priv, 0x610490, 0x00000010, 0x00000000);
	nv_mask(priv, 0x610490, 0x00000003, 0x00000000);
	if (!nv_wait(priv, 0x610490, 0x001e0000, 0x00000000)) {
		nv_error(mast, "fini: 0x%08x\n", nv_rd32(priv, 0x610490));
		if (suspend)
			return -EBUSY;
	}

	/* disable error reporting */
	nv_mask(priv, 0x610090, 0x00000001, 0x00000000);
	nv_mask(priv, 0x6100a0, 0x00000001, 0x00000000);

	return nv50_disp_chan_fini(&mast->base, suspend);
}

struct nouveau_ofuncs
nvd0_disp_mast_ofuncs = {
	.ctor = nvd0_disp_mast_ctor,
	.dtor = nv50_disp_dmac_dtor,
	.init = nvd0_disp_mast_init,
	.fini = nvd0_disp_mast_fini,
	.rd32 = nv50_disp_chan_rd32,
	.wr32 = nv50_disp_chan_wr32,
};

/*******************************************************************************
 * EVO sync channel objects
 ******************************************************************************/

static const struct nv50_disp_mthd_list
nvd0_disp_sync_mthd_base = {
	.mthd = 0x0000,
	.addr = 0x000000,
	.data = {
		{ 0x0080, 0x661080 },
		{ 0x0084, 0x661084 },
		{ 0x0088, 0x661088 },
		{ 0x008c, 0x66108c },
		{ 0x0090, 0x661090 },
		{ 0x0094, 0x661094 },
		{ 0x00a0, 0x6610a0 },
		{ 0x00a4, 0x6610a4 },
		{ 0x00c0, 0x6610c0 },
		{ 0x00c4, 0x6610c4 },
		{ 0x00c8, 0x6610c8 },
		{ 0x00cc, 0x6610cc },
		{ 0x00e0, 0x6610e0 },
		{ 0x00e4, 0x6610e4 },
		{ 0x00e8, 0x6610e8 },
		{ 0x00ec, 0x6610ec },
		{ 0x00fc, 0x6610fc },
		{ 0x0100, 0x661100 },
		{ 0x0104, 0x661104 },
		{ 0x0108, 0x661108 },
		{ 0x010c, 0x66110c },
		{ 0x0110, 0x661110 },
		{ 0x0114, 0x661114 },
		{ 0x0118, 0x661118 },
		{ 0x011c, 0x66111c },
		{ 0x0130, 0x661130 },
		{ 0x0134, 0x661134 },
		{ 0x0138, 0x661138 },
		{ 0x013c, 0x66113c },
		{ 0x0140, 0x661140 },
		{ 0x0144, 0x661144 },
		{ 0x0148, 0x661148 },
		{ 0x014c, 0x66114c },
		{ 0x0150, 0x661150 },
		{ 0x0154, 0x661154 },
		{ 0x0158, 0x661158 },
		{ 0x015c, 0x66115c },
		{ 0x0160, 0x661160 },
		{ 0x0164, 0x661164 },
		{ 0x0168, 0x661168 },
		{ 0x016c, 0x66116c },
		{}
	}
};

static const struct nv50_disp_mthd_list
nvd0_disp_sync_mthd_image = {
	.mthd = 0x0400,
	.addr = 0x000400,
	.data = {
		{ 0x0400, 0x661400 },
		{ 0x0404, 0x661404 },
		{ 0x0408, 0x661408 },
		{ 0x040c, 0x66140c },
		{ 0x0410, 0x661410 },
		{}
	}
};

const struct nv50_disp_mthd_chan
nvd0_disp_sync_mthd_chan = {
	.name = "Base",
	.addr = 0x001000,
	.data = {
		{ "Global", 1, &nvd0_disp_sync_mthd_base },
		{  "Image", 2, &nvd0_disp_sync_mthd_image },
		{}
	}
};

static int
nvd0_disp_sync_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_display_sync_class *args = data;
	struct nv50_disp_priv *priv = (void *)engine;
	struct nv50_disp_dmac *dmac;
	int ret;

	if (size < sizeof(*args) || args->head >= priv->head.nr)
		return -EINVAL;

	ret = nv50_disp_dmac_create_(parent, engine, oclass, args->pushbuf,
				     1 + args->head, sizeof(*dmac),
				     (void **)&dmac);
	*pobject = nv_object(dmac);
	if (ret)
		return ret;

	nv_parent(dmac)->object_attach = nvd0_disp_dmac_object_attach;
	nv_parent(dmac)->object_detach = nvd0_disp_dmac_object_detach;
	return 0;
}

struct nouveau_ofuncs
nvd0_disp_sync_ofuncs = {
	.ctor = nvd0_disp_sync_ctor,
	.dtor = nv50_disp_dmac_dtor,
	.init = nvd0_disp_dmac_init,
	.fini = nvd0_disp_dmac_fini,
	.rd32 = nv50_disp_chan_rd32,
	.wr32 = nv50_disp_chan_wr32,
};

/*******************************************************************************
 * EVO overlay channel objects
 ******************************************************************************/

static const struct nv50_disp_mthd_list
nvd0_disp_ovly_mthd_base = {
	.mthd = 0x0000,
	.data = {
		{ 0x0080, 0x665080 },
		{ 0x0084, 0x665084 },
		{ 0x0088, 0x665088 },
		{ 0x008c, 0x66508c },
		{ 0x0090, 0x665090 },
		{ 0x0094, 0x665094 },
		{ 0x00a0, 0x6650a0 },
		{ 0x00a4, 0x6650a4 },
		{ 0x00b0, 0x6650b0 },
		{ 0x00b4, 0x6650b4 },
		{ 0x00b8, 0x6650b8 },
		{ 0x00c0, 0x6650c0 },
		{ 0x00e0, 0x6650e0 },
		{ 0x00e4, 0x6650e4 },
		{ 0x00e8, 0x6650e8 },
		{ 0x0100, 0x665100 },
		{ 0x0104, 0x665104 },
		{ 0x0108, 0x665108 },
		{ 0x010c, 0x66510c },
		{ 0x0110, 0x665110 },
		{ 0x0118, 0x665118 },
		{ 0x011c, 0x66511c },
		{ 0x0120, 0x665120 },
		{ 0x0124, 0x665124 },
		{ 0x0130, 0x665130 },
		{ 0x0134, 0x665134 },
		{ 0x0138, 0x665138 },
		{ 0x013c, 0x66513c },
		{ 0x0140, 0x665140 },
		{ 0x0144, 0x665144 },
		{ 0x0148, 0x665148 },
		{ 0x014c, 0x66514c },
		{ 0x0150, 0x665150 },
		{ 0x0154, 0x665154 },
		{ 0x0158, 0x665158 },
		{ 0x015c, 0x66515c },
		{ 0x0160, 0x665160 },
		{ 0x0164, 0x665164 },
		{ 0x0168, 0x665168 },
		{ 0x016c, 0x66516c },
		{ 0x0400, 0x665400 },
		{ 0x0408, 0x665408 },
		{ 0x040c, 0x66540c },
		{ 0x0410, 0x665410 },
		{}
	}
};

static const struct nv50_disp_mthd_chan
nvd0_disp_ovly_mthd_chan = {
	.name = "Overlay",
	.addr = 0x001000,
	.data = {
		{ "Global", 1, &nvd0_disp_ovly_mthd_base },
		{}
	}
};

static int
nvd0_disp_ovly_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_display_ovly_class *args = data;
	struct nv50_disp_priv *priv = (void *)engine;
	struct nv50_disp_dmac *dmac;
	int ret;

	if (size < sizeof(*args) || args->head >= priv->head.nr)
		return -EINVAL;

	ret = nv50_disp_dmac_create_(parent, engine, oclass, args->pushbuf,
				     5 + args->head, sizeof(*dmac),
				     (void **)&dmac);
	*pobject = nv_object(dmac);
	if (ret)
		return ret;

	nv_parent(dmac)->object_attach = nvd0_disp_dmac_object_attach;
	nv_parent(dmac)->object_detach = nvd0_disp_dmac_object_detach;
	return 0;
}

struct nouveau_ofuncs
nvd0_disp_ovly_ofuncs = {
	.ctor = nvd0_disp_ovly_ctor,
	.dtor = nv50_disp_dmac_dtor,
	.init = nvd0_disp_dmac_init,
	.fini = nvd0_disp_dmac_fini,
	.rd32 = nv50_disp_chan_rd32,
	.wr32 = nv50_disp_chan_wr32,
};

/*******************************************************************************
 * EVO PIO channel base class
 ******************************************************************************/

static int
nvd0_disp_pioc_create_(struct nouveau_object *parent,
		       struct nouveau_object *engine,
		       struct nouveau_oclass *oclass, int chid,
		       int length, void **pobject)
{
	return nv50_disp_chan_create_(parent, engine, oclass, chid,
				      length, pobject);
}

static void
nvd0_disp_pioc_dtor(struct nouveau_object *object)
{
	struct nv50_disp_pioc *pioc = (void *)object;
	nv50_disp_chan_destroy(&pioc->base);
}

static int
nvd0_disp_pioc_init(struct nouveau_object *object)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_pioc *pioc = (void *)object;
	int chid = pioc->base.chid;
	int ret;

	ret = nv50_disp_chan_init(&pioc->base);
	if (ret)
		return ret;

	/* enable error reporting */
	nv_mask(priv, 0x610090, 0x00000001 << chid, 0x00000001 << chid);
	nv_mask(priv, 0x6100a0, 0x00000001 << chid, 0x00000001 << chid);

	/* activate channel */
	nv_wr32(priv, 0x610490 + (chid * 0x10), 0x00000001);
	if (!nv_wait(priv, 0x610490 + (chid * 0x10), 0x00030000, 0x00010000)) {
		nv_error(pioc, "init: 0x%08x\n",
			 nv_rd32(priv, 0x610490 + (chid * 0x10)));
		return -EBUSY;
	}

	return 0;
}

static int
nvd0_disp_pioc_fini(struct nouveau_object *object, bool suspend)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_pioc *pioc = (void *)object;
	int chid = pioc->base.chid;

	nv_mask(priv, 0x610490 + (chid * 0x10), 0x00000001, 0x00000000);
	if (!nv_wait(priv, 0x610490 + (chid * 0x10), 0x00030000, 0x00000000)) {
		nv_error(pioc, "timeout: 0x%08x\n",
			 nv_rd32(priv, 0x610490 + (chid * 0x10)));
		if (suspend)
			return -EBUSY;
	}

	/* disable error reporting */
	nv_mask(priv, 0x610090, 0x00000001 << chid, 0x00000000);
	nv_mask(priv, 0x6100a0, 0x00000001 << chid, 0x00000000);

	return nv50_disp_chan_fini(&pioc->base, suspend);
}

/*******************************************************************************
 * EVO immediate overlay channel objects
 ******************************************************************************/

static int
nvd0_disp_oimm_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_display_oimm_class *args = data;
	struct nv50_disp_priv *priv = (void *)engine;
	struct nv50_disp_pioc *pioc;
	int ret;

	if (size < sizeof(*args) || args->head >= priv->head.nr)
		return -EINVAL;

	ret = nvd0_disp_pioc_create_(parent, engine, oclass, 9 + args->head,
				     sizeof(*pioc), (void **)&pioc);
	*pobject = nv_object(pioc);
	if (ret)
		return ret;

	return 0;
}

struct nouveau_ofuncs
nvd0_disp_oimm_ofuncs = {
	.ctor = nvd0_disp_oimm_ctor,
	.dtor = nvd0_disp_pioc_dtor,
	.init = nvd0_disp_pioc_init,
	.fini = nvd0_disp_pioc_fini,
	.rd32 = nv50_disp_chan_rd32,
	.wr32 = nv50_disp_chan_wr32,
};

/*******************************************************************************
 * EVO cursor channel objects
 ******************************************************************************/

static int
nvd0_disp_curs_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_display_curs_class *args = data;
	struct nv50_disp_priv *priv = (void *)engine;
	struct nv50_disp_pioc *pioc;
	int ret;

	if (size < sizeof(*args) || args->head >= priv->head.nr)
		return -EINVAL;

	ret = nvd0_disp_pioc_create_(parent, engine, oclass, 13 + args->head,
				     sizeof(*pioc), (void **)&pioc);
	*pobject = nv_object(pioc);
	if (ret)
		return ret;

	return 0;
}

struct nouveau_ofuncs
nvd0_disp_curs_ofuncs = {
	.ctor = nvd0_disp_curs_ctor,
	.dtor = nvd0_disp_pioc_dtor,
	.init = nvd0_disp_pioc_init,
	.fini = nvd0_disp_pioc_fini,
	.rd32 = nv50_disp_chan_rd32,
	.wr32 = nv50_disp_chan_wr32,
};

/*******************************************************************************
 * Base display object
 ******************************************************************************/

static int
nvd0_disp_base_scanoutpos(struct nouveau_object *object, u32 mthd,
			  void *data, u32 size)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv04_display_scanoutpos *args = data;
	const int head = (mthd & NV50_DISP_MTHD_HEAD);
	u32 blanke, blanks, total;

	if (size < sizeof(*args) || head >= priv->head.nr)
		return -EINVAL;

	total  = nv_rd32(priv, 0x640414 + (head * 0x300));
	blanke = nv_rd32(priv, 0x64041c + (head * 0x300));
	blanks = nv_rd32(priv, 0x640420 + (head * 0x300));

	args->vblanke = (blanke & 0xffff0000) >> 16;
	args->hblanke = (blanke & 0x0000ffff);
	args->vblanks = (blanks & 0xffff0000) >> 16;
	args->hblanks = (blanks & 0x0000ffff);
	args->vtotal  = ( total & 0xffff0000) >> 16;
	args->htotal  = ( total & 0x0000ffff);

	args->time[0] = ktime_to_ns(ktime_get());
	args->vline   = nv_rd32(priv, 0x616340 + (head * 0x800)) & 0xffff;
	args->time[1] = ktime_to_ns(ktime_get()); /* vline read locks hline */
	args->hline   = nv_rd32(priv, 0x616344 + (head * 0x800)) & 0xffff;
	return 0;
}

static void
nvd0_disp_base_vblank_enable(struct nouveau_event *event, int head)
{
	nv_mask(event->priv, 0x6100c0 + (head * 0x800), 0x00000001, 0x00000001);
}

static void
nvd0_disp_base_vblank_disable(struct nouveau_event *event, int head)
{
	nv_mask(event->priv, 0x6100c0 + (head * 0x800), 0x00000001, 0x00000000);
}

static int
nvd0_disp_base_ctor(struct nouveau_object *parent,
		    struct nouveau_object *engine,
		    struct nouveau_oclass *oclass, void *data, u32 size,
		    struct nouveau_object **pobject)
{
	struct nv50_disp_priv *priv = (void *)engine;
	struct nv50_disp_base *base;
	int ret;

	ret = nouveau_parent_create(parent, engine, oclass, 0,
				    priv->sclass, 0, &base);
	*pobject = nv_object(base);
	if (ret)
		return ret;

	priv->base.vblank->priv = priv;
	priv->base.vblank->enable = nvd0_disp_base_vblank_enable;
	priv->base.vblank->disable = nvd0_disp_base_vblank_disable;

	return nouveau_ramht_new(nv_object(base), nv_object(base), 0x1000, 0,
				&base->ramht);
}

static void
nvd0_disp_base_dtor(struct nouveau_object *object)
{
	struct nv50_disp_base *base = (void *)object;
	nouveau_ramht_ref(NULL, &base->ramht);
	nouveau_parent_destroy(&base->base);
}

static int
nvd0_disp_base_init(struct nouveau_object *object)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_base *base = (void *)object;
	int ret, i;
	u32 tmp;

	ret = nouveau_parent_init(&base->base);
	if (ret)
		return ret;

	/* The below segments of code copying values from one register to
	 * another appear to inform EVO of the display capabilities or
	 * something similar.
	 */

	/* ... CRTC caps */
	for (i = 0; i < priv->head.nr; i++) {
		tmp = nv_rd32(priv, 0x616104 + (i * 0x800));
		nv_wr32(priv, 0x6101b4 + (i * 0x800), tmp);
		tmp = nv_rd32(priv, 0x616108 + (i * 0x800));
		nv_wr32(priv, 0x6101b8 + (i * 0x800), tmp);
		tmp = nv_rd32(priv, 0x61610c + (i * 0x800));
		nv_wr32(priv, 0x6101bc + (i * 0x800), tmp);
	}

	/* ... DAC caps */
	for (i = 0; i < priv->dac.nr; i++) {
		tmp = nv_rd32(priv, 0x61a000 + (i * 0x800));
		nv_wr32(priv, 0x6101c0 + (i * 0x800), tmp);
	}

	/* ... SOR caps */
	for (i = 0; i < priv->sor.nr; i++) {
		tmp = nv_rd32(priv, 0x61c000 + (i * 0x800));
		nv_wr32(priv, 0x6301c4 + (i * 0x800), tmp);
	}

	/* steal display away from vbios, or something like that */
	if (nv_rd32(priv, 0x6100ac) & 0x00000100) {
		nv_wr32(priv, 0x6100ac, 0x00000100);
		nv_mask(priv, 0x6194e8, 0x00000001, 0x00000000);
		if (!nv_wait(priv, 0x6194e8, 0x00000002, 0x00000000)) {
			nv_error(priv, "timeout acquiring display\n");
			return -EBUSY;
		}
	}

	/* point at display engine memory area (hash table, objects) */
	nv_wr32(priv, 0x610010, (nv_gpuobj(object->parent)->addr >> 8) | 9);

	/* enable supervisor interrupts, disable everything else */
	nv_wr32(priv, 0x610090, 0x00000000);
	nv_wr32(priv, 0x6100a0, 0x00000000);
	nv_wr32(priv, 0x6100b0, 0x00000307);

	/* disable underflow reporting, preventing an intermittent issue
	 * on some nve4 boards where the production vbios left this
	 * setting enabled by default.
	 *
	 * ftp://download.nvidia.com/open-gpu-doc/gk104-disable-underflow-reporting/1/gk104-disable-underflow-reporting.txt
	 */
	for (i = 0; i < priv->head.nr; i++)
		nv_mask(priv, 0x616308 + (i * 0x800), 0x00000111, 0x00000010);

	return 0;
}

static int
nvd0_disp_base_fini(struct nouveau_object *object, bool suspend)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	struct nv50_disp_base *base = (void *)object;

	/* disable all interrupts */
	nv_wr32(priv, 0x6100b0, 0x00000000);

	return nouveau_parent_fini(&base->base, suspend);
}

struct nouveau_ofuncs
nvd0_disp_base_ofuncs = {
	.ctor = nvd0_disp_base_ctor,
	.dtor = nvd0_disp_base_dtor,
	.init = nvd0_disp_base_init,
	.fini = nvd0_disp_base_fini,
};

struct nouveau_omthds
nvd0_disp_base_omthds[] = {
	{ HEAD_MTHD(NV50_DISP_SCANOUTPOS)     , nvd0_disp_base_scanoutpos },
	{ SOR_MTHD(NV50_DISP_SOR_PWR)         , nv50_sor_mthd },
	{ SOR_MTHD(NVA3_DISP_SOR_HDA_ELD)     , nv50_sor_mthd },
	{ SOR_MTHD(NV84_DISP_SOR_HDMI_PWR)    , nv50_sor_mthd },
	{ SOR_MTHD(NV50_DISP_SOR_LVDS_SCRIPT) , nv50_sor_mthd },
	{ DAC_MTHD(NV50_DISP_DAC_PWR)         , nv50_dac_mthd },
	{ DAC_MTHD(NV50_DISP_DAC_LOAD)        , nv50_dac_mthd },
	{ PIOR_MTHD(NV50_DISP_PIOR_PWR)       , nv50_pior_mthd },
	{ PIOR_MTHD(NV50_DISP_PIOR_TMDS_PWR)  , nv50_pior_mthd },
	{ PIOR_MTHD(NV50_DISP_PIOR_DP_PWR)    , nv50_pior_mthd },
	{},
};

static struct nouveau_oclass
nvd0_disp_base_oclass[] = {
	{ NVD0_DISP_CLASS, &nvd0_disp_base_ofuncs, nvd0_disp_base_omthds },
	{}
};

static struct nouveau_oclass
nvd0_disp_sclass[] = {
	{ NVD0_DISP_MAST_CLASS, &nvd0_disp_mast_ofuncs },
	{ NVD0_DISP_SYNC_CLASS, &nvd0_disp_sync_ofuncs },
	{ NVD0_DISP_OVLY_CLASS, &nvd0_disp_ovly_ofuncs },
	{ NVD0_DISP_OIMM_CLASS, &nvd0_disp_oimm_ofuncs },
	{ NVD0_DISP_CURS_CLASS, &nvd0_disp_curs_ofuncs },
	{}
};

/*******************************************************************************
 * Display engine implementation
 ******************************************************************************/

static u16
exec_lookup(struct nv50_disp_priv *priv, int head, int outp, u32 ctrl,
	    struct dcb_output *dcb, u8 *ver, u8 *hdr, u8 *cnt, u8 *len,
	    struct nvbios_outp *info)
{
	struct nouveau_bios *bios = nouveau_bios(priv);
	u16 mask, type, data;

	if (outp < 4) {
		type = DCB_OUTPUT_ANALOG;
		mask = 0;
	} else {
		outp -= 4;
		switch (ctrl & 0x00000f00) {
		case 0x00000000: type = DCB_OUTPUT_LVDS; mask = 1; break;
		case 0x00000100: type = DCB_OUTPUT_TMDS; mask = 1; break;
		case 0x00000200: type = DCB_OUTPUT_TMDS; mask = 2; break;
		case 0x00000500: type = DCB_OUTPUT_TMDS; mask = 3; break;
		case 0x00000800: type = DCB_OUTPUT_DP; mask = 1; break;
		case 0x00000900: type = DCB_OUTPUT_DP; mask = 2; break;
		default:
			nv_error(priv, "unknown SOR mc 0x%08x\n", ctrl);
			return 0x0000;
		}
		dcb->sorconf.link = mask;
	}

	mask  = 0x00c0 & (mask << 6);
	mask |= 0x0001 << outp;
	mask |= 0x0100 << head;

	data = dcb_outp_match(bios, type, mask, ver, hdr, dcb);
	if (!data)
		return 0x0000;

	return nvbios_outp_match(bios, type, mask, ver, hdr, cnt, len, info);
}

static bool
exec_script(struct nv50_disp_priv *priv, int head, int id)
{
	struct nouveau_bios *bios = nouveau_bios(priv);
	struct nvbios_outp info;
	struct dcb_output dcb;
	u8  ver, hdr, cnt, len;
	u32 ctrl = 0x00000000;
	u16 data;
	int outp;

	for (outp = 0; !(ctrl & (1 << head)) && outp < 8; outp++) {
		ctrl = nv_rd32(priv, 0x640180 + (outp * 0x20));
		if (ctrl & (1 << head))
			break;
	}

	if (outp == 8)
		return false;

	data = exec_lookup(priv, head, outp, ctrl, &dcb, &ver, &hdr, &cnt, &len, &info);
	if (data) {
		struct nvbios_init init = {
			.subdev = nv_subdev(priv),
			.bios = bios,
			.offset = info.script[id],
			.outp = &dcb,
			.crtc = head,
			.execute = 1,
		};

		return nvbios_exec(&init) == 0;
	}

	return false;
}

static u32
exec_clkcmp(struct nv50_disp_priv *priv, int head, int id,
	    u32 pclk, struct dcb_output *dcb)
{
	struct nouveau_bios *bios = nouveau_bios(priv);
	struct nvbios_outp info1;
	struct nvbios_ocfg info2;
	u8  ver, hdr, cnt, len;
	u32 ctrl = 0x00000000;
	u32 data, conf = ~0;
	int outp;

	for (outp = 0; !(ctrl & (1 << head)) && outp < 8; outp++) {
		ctrl = nv_rd32(priv, 0x660180 + (outp * 0x20));
		if (ctrl & (1 << head))
			break;
	}

	if (outp == 8)
		return conf;

	data = exec_lookup(priv, head, outp, ctrl, dcb, &ver, &hdr, &cnt, &len, &info1);
	if (data == 0x0000)
		return conf;

	switch (dcb->type) {
	case DCB_OUTPUT_TMDS:
		conf = (ctrl & 0x00000f00) >> 8;
		if (pclk >= 165000)
			conf |= 0x0100;
		break;
	case DCB_OUTPUT_LVDS:
		conf = priv->sor.lvdsconf;
		break;
	case DCB_OUTPUT_DP:
		conf = (ctrl & 0x00000f00) >> 8;
		break;
	case DCB_OUTPUT_ANALOG:
	default:
		conf = 0x00ff;
		break;
	}

	data = nvbios_ocfg_match(bios, data, conf, &ver, &hdr, &cnt, &len, &info2);
	CTASSERT(__arraycount(info2.clkcmp) <= 0xff);
	if (data && id < __arraycount(info2.clkcmp)) {
		data = nvbios_oclk_match(bios, info2.clkcmp[id], pclk);
		if (data) {
			struct nvbios_init init = {
				.subdev = nv_subdev(priv),
				.bios = bios,
				.offset = data,
				.outp = dcb,
				.crtc = head,
				.execute = 1,
			};

			nvbios_exec(&init);
		}
	}

	return conf;
}

static void
nvd0_disp_intr_unk1_0(struct nv50_disp_priv *priv, int head)
{
	exec_script(priv, head, 1);
}

static void
nvd0_disp_intr_unk2_0(struct nv50_disp_priv *priv, int head)
{
	exec_script(priv, head, 2);
}

static void
nvd0_disp_intr_unk2_1(struct nv50_disp_priv *priv, int head)
{
	struct nouveau_devinit *devinit = nouveau_devinit(priv);
	u32 pclk = nv_rd32(priv, 0x660450 + (head * 0x300)) / 1000;
	if (pclk)
		devinit->pll_set(devinit, PLL_VPLL0 + head, pclk);
	nv_wr32(priv, 0x612200 + (head * 0x800), 0x00000000);
}

static void
nvd0_disp_intr_unk2_2_tu(struct nv50_disp_priv *priv, int head,
			 struct dcb_output *outp)
{
	const int or = ffs(outp->or) - 1;
	const u32 ctrl = nv_rd32(priv, 0x660200 + (or   * 0x020));
	const u32 conf = nv_rd32(priv, 0x660404 + (head * 0x300));
	const u32 pclk = nv_rd32(priv, 0x660450 + (head * 0x300)) / 1000;
	const u32 link = ((ctrl & 0xf00) == 0x800) ? 0 : 1;
	const u32 hoff = (head * 0x800);
	const u32 soff = (  or * 0x800);
	const u32 loff = (link * 0x080) + soff;
	const u32 symbol = 100000;
	const u32 TU = 64;
	u32 dpctrl = nv_rd32(priv, 0x61c10c + loff) & 0x000f0000;
	u32 clksor = nv_rd32(priv, 0x612300 + soff);
	u32 datarate, link_nr, link_bw, bits;
	u64 ratio, value;

	if      ((conf & 0x3c0) == 0x180) bits = 30;
	else if ((conf & 0x3c0) == 0x140) bits = 24;
	else                              bits = 18;
	datarate = (pclk * bits) / 8;

	if      (dpctrl > 0x00030000) link_nr = 4;
	else if (dpctrl > 0x00010000) link_nr = 2;
	else			      link_nr = 1;

	link_bw  = (clksor & 0x007c0000) >> 18;
	link_bw *= 27000;

	ratio  = datarate;
	ratio *= symbol;
	do_div(ratio, link_nr * link_bw);

	value  = (symbol - ratio) * TU;
	value *= ratio;
	do_div(value, symbol);
	do_div(value, symbol);

	value += 5;
	value |= 0x08000000;

	nv_wr32(priv, 0x616610 + hoff, value);
}

static void
nvd0_disp_intr_unk2_2(struct nv50_disp_priv *priv, int head)
{
	struct dcb_output outp;
	u32 pclk = nv_rd32(priv, 0x660450 + (head * 0x300)) / 1000;
	u32 conf = exec_clkcmp(priv, head, 0xff, pclk, &outp);
	if (conf != ~0) {
		u32 addr, data;

		if (outp.type == DCB_OUTPUT_DP) {
			u32 sync = nv_rd32(priv, 0x660404 + (head * 0x300));
			switch ((sync & 0x000003c0) >> 6) {
			case 6: pclk = pclk * 30 / 8; break;
			case 5: pclk = pclk * 24 / 8; break;
			case 2:
			default:
				pclk = pclk * 18 / 8;
				break;
			}

			nouveau_dp_train(&priv->base, priv->sor.dp,
					 &outp, head, pclk);
		}

		exec_clkcmp(priv, head, 0, pclk, &outp);

		if (outp.type == DCB_OUTPUT_ANALOG) {
			addr = 0x612280 + (ffs(outp.or) - 1) * 0x800;
			data = 0x00000000;
		} else {
			if (outp.type == DCB_OUTPUT_DP)
				nvd0_disp_intr_unk2_2_tu(priv, head, &outp);
			addr = 0x612300 + (ffs(outp.or) - 1) * 0x800;
			data = (conf & 0x0100) ? 0x00000101 : 0x00000000;
		}

		nv_mask(priv, addr, 0x00000707, data);
	}
}

static void
nvd0_disp_intr_unk4_0(struct nv50_disp_priv *priv, int head)
{
	struct dcb_output outp;
	u32 pclk = nv_rd32(priv, 0x660450 + (head * 0x300)) / 1000;
	exec_clkcmp(priv, head, 1, pclk, &outp);
}

void
nvd0_disp_intr_supervisor(struct work_struct *work)
{
	struct nv50_disp_priv *priv =
		container_of(work, struct nv50_disp_priv, supervisor);
	struct nv50_disp_impl *impl = (void *)nv_object(priv)->oclass;
	u32 mask[4];
	int head;

	nv_debug(priv, "supervisor %d\n", ffs(priv->super));
	for (head = 0; head < priv->head.nr; head++) {
		mask[head] = nv_rd32(priv, 0x6101d4 + (head * 0x800));
		nv_debug(priv, "head %d: 0x%08x\n", head, mask[head]);
	}

	if (priv->super & 0x00000001) {
		nv50_disp_mthd_chan(priv, NV_DBG_DEBUG, 0, impl->mthd.core);
		for (head = 0; head < priv->head.nr; head++) {
			if (!(mask[head] & 0x00001000))
				continue;
			nv_debug(priv, "supervisor 1.0 - head %d\n", head);
			nvd0_disp_intr_unk1_0(priv, head);
		}
	} else
	if (priv->super & 0x00000002) {
		for (head = 0; head < priv->head.nr; head++) {
			if (!(mask[head] & 0x00001000))
				continue;
			nv_debug(priv, "supervisor 2.0 - head %d\n", head);
			nvd0_disp_intr_unk2_0(priv, head);
		}
		for (head = 0; head < priv->head.nr; head++) {
			if (!(mask[head] & 0x00010000))
				continue;
			nv_debug(priv, "supervisor 2.1 - head %d\n", head);
			nvd0_disp_intr_unk2_1(priv, head);
		}
		for (head = 0; head < priv->head.nr; head++) {
			if (!(mask[head] & 0x00001000))
				continue;
			nv_debug(priv, "supervisor 2.2 - head %d\n", head);
			nvd0_disp_intr_unk2_2(priv, head);
		}
	} else
	if (priv->super & 0x00000004) {
		for (head = 0; head < priv->head.nr; head++) {
			if (!(mask[head] & 0x00001000))
				continue;
			nv_debug(priv, "supervisor 3.0 - head %d\n", head);
			nvd0_disp_intr_unk4_0(priv, head);
		}
	}

	for (head = 0; head < priv->head.nr; head++)
		nv_wr32(priv, 0x6101d4 + (head * 0x800), 0x00000000);
	nv_wr32(priv, 0x6101d0, 0x80000000);
}

static void
nvd0_disp_intr_error(struct nv50_disp_priv *priv, int chid)
{
	const struct nv50_disp_impl *impl = (void *)nv_object(priv)->oclass;
	u32 mthd = nv_rd32(priv, 0x6101f0 + (chid * 12));
	u32 data = nv_rd32(priv, 0x6101f4 + (chid * 12));
	u32 unkn = nv_rd32(priv, 0x6101f8 + (chid * 12));

	nv_error(priv, "chid %d mthd 0x%04x data 0x%08x "
		       "0x%08x 0x%08x\n",
		 chid, (mthd & 0x0000ffc), data, mthd, unkn);

	if (chid == 0) {
		switch (mthd) {
		case 0x0080:
			nv50_disp_mthd_chan(priv, NV_DBG_ERROR, chid - 0,
					    impl->mthd.core);
			break;
		default:
			break;
		}
	} else
	if (chid <= 4) {
		switch (mthd) {
		case 0x0080:
			nv50_disp_mthd_chan(priv, NV_DBG_ERROR, chid - 1,
					    impl->mthd.base);
			break;
		default:
			break;
		}
	} else
	if (chid <= 8) {
		switch (mthd) {
		case 0x0080:
			nv50_disp_mthd_chan(priv, NV_DBG_ERROR, chid - 5,
					    impl->mthd.ovly);
			break;
		default:
			break;
		}
	}

	nv_wr32(priv, 0x61009c, (1 << chid));
	nv_wr32(priv, 0x6101f0 + (chid * 12), 0x90000000);
}

void
nvd0_disp_intr(struct nouveau_subdev *subdev)
{
	struct nv50_disp_priv *priv = (void *)subdev;
	u32 intr = nv_rd32(priv, 0x610088);
	int i;

	if (intr & 0x00000001) {
		u32 stat = nv_rd32(priv, 0x61008c);
		nv_wr32(priv, 0x61008c, stat);
		intr &= ~0x00000001;
	}

	if (intr & 0x00000002) {
		u32 stat = nv_rd32(priv, 0x61009c);
		int chid = ffs(stat) - 1;
		if (chid >= 0)
			nvd0_disp_intr_error(priv, chid);
		intr &= ~0x00000002;
	}

	if (intr & 0x00100000) {
		u32 stat = nv_rd32(priv, 0x6100ac);
		if (stat & 0x00000007) {
			priv->super = (stat & 0x00000007);
			schedule_work(&priv->supervisor);
			nv_wr32(priv, 0x6100ac, priv->super);
			stat &= ~0x00000007;
		}

		if (stat) {
			nv_info(priv, "unknown intr24 0x%08x\n", stat);
			nv_wr32(priv, 0x6100ac, stat);
		}

		intr &= ~0x00100000;
	}

	for (i = 0; i < priv->head.nr; i++) {
		u32 mask = 0x01000000 << i;
		if (mask & intr) {
			u32 stat = nv_rd32(priv, 0x6100bc + (i * 0x800));
			if (stat & 0x00000001)
				nouveau_event_trigger(priv->base.vblank, i);
			nv_mask(priv, 0x6100bc + (i * 0x800), 0, 0);
			nv_rd32(priv, 0x6100c0 + (i * 0x800));
		}
	}
}

static int
nvd0_disp_ctor(struct nouveau_object *parent, struct nouveau_object *engine,
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

	nv_engine(priv)->sclass = nvd0_disp_base_oclass;
	nv_engine(priv)->cclass = &nv50_disp_cclass;
	nv_subdev(priv)->intr = nvd0_disp_intr;
	INIT_WORK(&priv->supervisor, nvd0_disp_intr_supervisor);
	priv->sclass = nvd0_disp_sclass;
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
nvd0_disp_oclass = &(struct nv50_disp_impl) {
	.base.base.handle = NV_ENGINE(DISP, 0x90),
	.base.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nvd0_disp_ctor,
		.dtor = _nouveau_disp_dtor,
		.init = _nouveau_disp_init,
		.fini = _nouveau_disp_fini,
	},
	.mthd.core = &nvd0_disp_mast_mthd_chan,
	.mthd.base = &nvd0_disp_sync_mthd_chan,
	.mthd.ovly = &nvd0_disp_ovly_mthd_chan,
	.mthd.prev = -0x020000,
}.base.base;
