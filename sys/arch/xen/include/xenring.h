/* $NetBSD: xenring.h,v 1.6.20.2 2024/07/20 16:11:26 martin Exp $ */

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

void xen_mb(void);
#define xen_rmb() membar_acquire()
#define xen_wmb() membar_release()

/*
 * Define ring types. These were previously part of the public API.
 * Not anymore.
 */
/* i386 requests/responses */
struct blkif_x86_32_request {
    uint8_t        operation;    /* BLKIF_OP_???                         */
    uint8_t        nr_segments;  /* number of segments                   */
    blkif_vdev_t   handle;       /* only for read/write requests         */
    uint64_t       id;           /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} __packed;
typedef struct blkif_x86_32_request blkif_x86_32_request_t;

struct blkif_x86_32_response {
    uint64_t        id;              /* copied from request */
    uint8_t         operation;       /* copied from request */
    uint8_t         _pad;
    int16_t         status;          /* BLKIF_RSP_???       */
} __packed;
typedef struct blkif_x86_32_response blkif_x86_32_response_t;

struct blkif_x86_32_request_indirect {
    uint8_t        operation;    /* BLKIF_OP_INDIRECT                    */
    uint8_t        indirect_op;  /* BLKIF_OP_{READ/WRITE}                */
    uint16_t       nr_segments;  /* number of segments                   */
    uint64_t       id;		 /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    blkif_vdev_t   handle;       /* only for read/write requests         */
    uint16_t	   _pad2;
    grant_ref_t    indirect_grefs[BLKIF_MAX_INDIRECT_PAGES_PER_REQUEST];
    uint64_t       _pad3;	 /* make it 64 byte aligned */
} __packed;
typedef struct blkif_x86_32_request_indirect blkif_x86_32_request_indirect_t;

/* amd64-type requests/responses (always used in frontends ) */

struct blkif_x86_64_request {
    uint8_t        operation;    /* BLKIF_OP_???                         */
    uint8_t        nr_segments;  /* number of segments                   */
    blkif_vdev_t   handle;       /* only for read/write requests         */
    uint64_t __attribute__((__aligned__(8))) id;/* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};
typedef struct blkif_x86_64_request blkif_x86_64_request_t;

struct blkif_x86_64_response {
    uint64_t __attribute__((__aligned__(8))) id; /* copied from request */
    uint8_t         operation;       /* copied from request */
    int16_t         status;          /* BLKIF_RSP_???       */
};
typedef struct blkif_x86_64_response blkif_x86_64_response_t;

struct blkif_x86_64_request_indirect {
    uint8_t        operation;    /* BLKIF_OP_INDIRECT                    */
    uint8_t        indirect_op;  /* BLKIF_OP_{READ/WRITE}                */
    uint16_t       nr_segments;  /* number of segments                   */
    uint32_t       _pad1;
    uint64_t       id;           /* private guest value, echoed in resp  */
    blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
    blkif_vdev_t   handle;       /* only for read/write requests         */
    uint16_t	   _pad2;
    grant_ref_t    indirect_grefs[BLKIF_MAX_INDIRECT_PAGES_PER_REQUEST];
    uint32_t       _pad3;	 /* make it 64 byte aligned */
} __packed;
typedef struct blkif_x86_64_request_indirect blkif_x86_64_request_indirect_t;

CTASSERT(sizeof(struct blkif_x86_32_request_indirect)
	== sizeof(struct blkif_x86_64_request_indirect));
CTASSERT(sizeof(struct blkif_request_indirect)
	== sizeof(struct blkif_x86_64_request_indirect));

DEFINE_RING_TYPES(blkif_x86_32, struct blkif_x86_32_request, struct blkif_x86_32_response);
DEFINE_RING_TYPES(blkif_x86_64, struct blkif_x86_64_request, struct blkif_x86_64_response);

union blkif_back_ring_proto {
	blkif_back_ring_t ring_n; /* native/common members */
	blkif_x86_32_back_ring_t ring_32;
	blkif_x86_64_back_ring_t ring_64;
};
typedef union blkif_back_ring_proto blkif_back_ring_proto_t;

#endif /* __XEN_INTERFACE_VERSION__ */

#endif /* _XEN_RING_H_ */
