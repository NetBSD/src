/*	$NetBSD: priv.h,v 1.2.6.2 2019/06/10 22:08:16 christos Exp $	*/

#ifndef __NVKM_CE_PRIV_H__
#define __NVKM_CE_PRIV_H__
#include <engine/ce.h>

void gt215_ce_intr(struct nvkm_falcon *, struct nvkm_fifo_chan *);
void gk104_ce_intr(struct nvkm_engine *);
#endif
