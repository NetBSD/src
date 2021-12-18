/*	$NetBSD: priv.h,v 1.3 2021/12/18 23:45:34 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_CE_PRIV_H__
#define __NVKM_CE_PRIV_H__
#include <engine/ce.h>

void gt215_ce_intr(struct nvkm_falcon *, struct nvkm_fifo_chan *);
void gk104_ce_intr(struct nvkm_engine *);
void gp100_ce_intr(struct nvkm_engine *);
#endif
