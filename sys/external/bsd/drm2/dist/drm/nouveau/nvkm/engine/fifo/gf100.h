/*	$NetBSD: gf100.h,v 1.2 2018/08/27 04:58:31 riastradh Exp $	*/

#ifndef __GF100_FIFO_H__
#define __GF100_FIFO_H__
#define gf100_fifo(p) container_of((p), struct gf100_fifo, base)
#include "priv.h"

#include <subdev/mmu.h>

struct gf100_fifo {
	struct nvkm_fifo base;

	struct list_head chan;

	struct work_struct fault;
	u64 mask;

	struct {
		struct nvkm_memory *mem[2];
		int active;
#ifdef __NetBSD__
		spinlock_t lock;
		drm_waitqueue_t wait;
#else
		wait_queue_head_t wait;
#endif
	} runlist;

	struct {
		struct nvkm_memory *mem;
		struct nvkm_vma bar;
	} user;
	int spoon_nr;
};

void gf100_fifo_intr_engine(struct gf100_fifo *);
void gf100_fifo_runlist_update(struct gf100_fifo *);
#endif
