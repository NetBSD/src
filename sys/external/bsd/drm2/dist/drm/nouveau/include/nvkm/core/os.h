/*	$NetBSD: os.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVKM_OS_H__
#define __NVKM_OS_H__
#include <nvif/os.h>

#ifndef __NetBSD__		/* XXX ioread */
#ifdef __BIG_ENDIAN
#define ioread16_native ioread16be
#define iowrite16_native iowrite16be
#define ioread32_native  ioread32be
#define iowrite32_native iowrite32be
#else
#define ioread16_native ioread16
#define iowrite16_native iowrite16
#define ioread32_native  ioread32
#define iowrite32_native iowrite32
#endif
#endif

#ifdef __NetBSD__
#include <sys/bus.h>
#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#endif
#endif

#define iowrite64_native(v,p) do {                                             \
	u32 __iomem *_p = (u32 __iomem *)(p);				       \
	u64 _v = (v);							       \
	iowrite32_native(lower_32_bits(_v), &_p[0]);			       \
	iowrite32_native(upper_32_bits(_v), &_p[1]);			       \
} while(0)

struct nvkm_blob {
	void *data;
	u32 size;
};

static inline void
nvkm_blob_dtor(struct nvkm_blob *blob)
{
	kfree(blob->data);
	blob->data = NULL;
	blob->size = 0;
}
#endif
