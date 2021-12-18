/*	$NetBSD: cgrp.h,v 1.2 2021/12/18 23:45:35 riastradh Exp $	*/

#ifndef __NVKM_FIFO_CGRP_H__
#define __NVKM_FIFO_CGRP_H__
#include "priv.h"

struct nvkm_fifo_cgrp {
	int id;
	struct list_head head;
	struct list_head chan;
	int chan_nr;
};
#endif
