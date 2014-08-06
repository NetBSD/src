/*	$NetBSD: nouveau_engine_disp_sornv50.c,v 1.1.1.1 2014/08/06 12:36:24 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_engine_disp_sornv50.c,v 1.1.1.1 2014/08/06 12:36:24 riastradh Exp $");

#include <core/os.h>
#include <core/class.h>

#include <subdev/bios.h>
#include <subdev/bios/dcb.h>
#include <subdev/timer.h>

#include "nv50.h"

int
nv50_sor_power(struct nv50_disp_priv *priv, int or, u32 data)
{
	const u32 stat = data & NV50_DISP_SOR_PWR_STATE;
	const u32 soff = (or * 0x800);
	nv_wait(priv, 0x61c004 + soff, 0x80000000, 0x00000000);
	nv_mask(priv, 0x61c004 + soff, 0x80000001, 0x80000000 | stat);
	nv_wait(priv, 0x61c004 + soff, 0x80000000, 0x00000000);
	nv_wait(priv, 0x61c030 + soff, 0x10000000, 0x00000000);
	return 0;
}

int
nv50_sor_mthd(struct nouveau_object *object, u32 mthd, void *args, u32 size)
{
	struct nv50_disp_priv *priv = (void *)object->engine;
	const u8  head = (mthd & NV50_DISP_SOR_MTHD_HEAD) >> 3;
	const u8    or = (mthd & NV50_DISP_SOR_MTHD_OR);
	u32 data;
	int ret = -EINVAL;

	if (size < sizeof(u32))
		return -EINVAL;
	data = *(u32 *)args;


	switch (mthd & ~0x3f) {
	case NV50_DISP_SOR_PWR:
		ret = priv->sor.power(priv, or, data);
		break;
	case NVA3_DISP_SOR_HDA_ELD:
		ret = priv->sor.hda_eld(priv, or, args, size);
		break;
	case NV84_DISP_SOR_HDMI_PWR:
		ret = priv->sor.hdmi(priv, head, or, data);
		break;
	case NV50_DISP_SOR_LVDS_SCRIPT:
		priv->sor.lvdsconf = data & NV50_DISP_SOR_LVDS_SCRIPT_ID;
		ret = 0;
		break;
	default:
		BUG_ON(1);
	}

	return ret;
}
