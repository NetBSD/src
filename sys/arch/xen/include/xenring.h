/* $NetBSD: xenring.h,v 1.3 2019/02/02 15:09:32 cherry Exp $ */

/*
 * Glue goop for xbd ring request/response protocol structures.
 *
 * These are used only from __XEN_INTERFACE_VERSION__ >= 0x00030201
 * prior to which they were part of the public XEN api.
 */

#ifndef _XEN_RING_H
#define _XEN_RING_H

#if (__XEN_INTERFACE_VERSION__ >= 0x00030201) && \
	(__XEN_INTERFACE_VERSION < 0x00030208)

#include <xen/include/public/io/ring.h>

/*
 * Undo namespace damage from xen/include/public/io/ring.h
 * The proper fix is to get upstream to stop assuming that all OSs use
 * mb(), rmb(), wmb().
 */
#undef xen_mb
#undef xen_rmb
#undef xen_wmb

#define xen_mb()  membar_sync()
#define xen_rmb() membar_producer()
#define xen_wmb() membar_consumer()

/*
 * Define ring types. These were previously part of the public API.
 * Not anymore.
 */
DEFINE_RING_TYPES(blkif_x86_32, struct blkif_request, struct blkif_response);
DEFINE_RING_TYPES(blkif_x86_64, struct blkif_request, struct blkif_response);

typedef struct blkif_request blkif_x86_64_request_t;
typedef struct blkif_response blkif_x86_64_response_t;
typedef struct blkif_request blkif_x86_32_request_t;
typedef struct blkif_response blkif_x86_32_response_t;


union blkif_back_ring_proto {
	blkif_back_ring_t ring_n; /* native/common members */
	blkif_x86_32_back_ring_t ring_32;
	blkif_x86_64_back_ring_t ring_64;
};
typedef union blkif_back_ring_proto blkif_back_ring_proto_t;

#endif /* __XEN_INTERFACE_VERSION__ */

#endif /* _XEN_RING_H_ */
