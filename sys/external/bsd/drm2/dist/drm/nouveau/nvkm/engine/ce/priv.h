/*	$NetBSD: priv.h,v 1.2 2018/08/27 04:58:31 riastradh Exp $	*/

#ifndef __NVKM_CE_PRIV_H__
#define __NVKM_CE_PRIV_H__
#include <engine/ce.h>

void gt215_ce_intr(struct nvkm_falcon *, struct nvkm_fifo_chan *);
void gk104_ce_intr(struct nvkm_engine *);
#endif
