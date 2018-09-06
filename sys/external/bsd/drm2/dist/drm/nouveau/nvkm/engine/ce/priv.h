/*	$NetBSD: priv.h,v 1.2.2.2 2018/09/06 06:56:25 pgoyette Exp $	*/

#ifndef __NVKM_CE_PRIV_H__
#define __NVKM_CE_PRIV_H__
#include <engine/ce.h>

void gt215_ce_intr(struct nvkm_falcon *, struct nvkm_fifo_chan *);
void gk104_ce_intr(struct nvkm_engine *);
#endif
