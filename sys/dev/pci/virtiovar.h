/*	$NetBSD: virtiovar.h,v 1.24 2022/03/24 08:08:05 andvar Exp $	*/

/*
 * Copyright (c) 2010 Minoura Makoto.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Part of the file derived from `Virtio PCI Card Specification v0.8.6 DRAFT'
 * Appendix A.
 */
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef _DEV_PCI_VIRTIOVAR_H_
#define	_DEV_PCI_VIRTIOVAR_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/bus.h>
#include <dev/pci/virtioreg.h>


struct vq_entry {
	SIMPLEQ_ENTRY(vq_entry)	qe_list; /* free list */
	uint16_t		qe_index; /* index in vq_desc array */
	/* followings are used only when it is the `head' entry */
	int16_t			qe_next;     /* next enq slot */
	bool			qe_indirect; /* 1 if using indirect */
	struct vring_desc	*qe_desc_base;
};

struct virtqueue {
	struct virtio_softc	*vq_owner;
        unsigned int		vq_num; /* queue size (# of entries) */
	int			vq_index; /* queue number (0, 1, ...) */

	/* vring pointers (KVA) */
        struct vring_desc	*vq_desc;
        struct vring_avail	*vq_avail;
        struct vring_used	*vq_used;
	void			*vq_indirect;
	uint16_t		*vq_used_event;		/* trails avail */
	uint16_t		*vq_avail_event;	/* trails used  */

	/* virtqueue allocation info */
	void			*vq_vaddr;
	int			vq_availoffset;
	int			vq_usedoffset;
	int			vq_indirectoffset;
	bus_dma_segment_t	vq_segs[1];
	unsigned int		vq_bytesize;
	bus_dmamap_t		vq_dmamap;

	int			vq_maxsegsize;
	int			vq_maxnsegs;

	/* free entry management */
	struct vq_entry		*vq_entries;
	SIMPLEQ_HEAD(, vq_entry) vq_freelist;
	kmutex_t		vq_freelist_lock;

	/* enqueue/dequeue status */
	uint16_t		vq_avail_idx;
	uint16_t		vq_used_idx;
	int			vq_queued;
	kmutex_t		vq_aring_lock;
	kmutex_t		vq_uring_lock;

	/* interrupt handler */
	int			(*vq_done)(struct virtqueue*);
	int			(*vq_intrhand)(void *);
	void			*vq_intrhand_arg;

	/* for 1.0 */
	uint32_t		vq_notify_off;
};

struct virtio_attach_args {
	int			sc_childdevid;
};

typedef int (*virtio_callback)(struct virtio_softc*);

#ifdef VIRTIO_PRIVATE
struct virtio_ops {
	void		(*kick)(struct virtio_softc *, uint16_t);
	uint16_t	(*read_queue_size)(struct virtio_softc *, uint16_t);
	void		(*setup_queue)(struct virtio_softc *, uint16_t, uint64_t);
	void		(*set_status)(struct virtio_softc *, int);
	void		(*neg_features)(struct virtio_softc *, uint64_t);
	int		(*alloc_interrupts)(struct virtio_softc *);
	void		(*free_interrupts)(struct virtio_softc *);
	int		(*setup_interrupts)(struct virtio_softc *, int);
};

struct virtio_softc {
	device_t		sc_dev;
	const struct virtio_ops *sc_ops;
	bus_dma_tag_t		sc_dmat;

	int			sc_bus_endian;
	int			sc_struct_endian;

	bus_space_tag_t		sc_devcfg_iot;
	bus_space_handle_t	sc_devcfg_ioh;
	bus_size_t		sc_devcfg_iosize;

	int			sc_ipl; /* set by child */
	void			*sc_soft_ih;

	int			sc_flags; /* set by child */

	uint64_t		sc_active_features;
	bool			sc_indirect;
	bool			sc_version_1;
	bool			sc_finished_called;

	int			sc_nvqs; /* set by child */
	struct virtqueue	*sc_vqs; /* set by child */

	int			sc_childdevid;
	device_t		sc_child; 		/* set by child */
	bool			sc_child_mq;
	virtio_callback		sc_config_change; 	/* set by child */
	virtio_callback		sc_intrhand;		/* set by child */
};
#else
struct virtio_softc;
#endif


/* interrupt types, stored in virtio_softc->sc_flags */
#define VIRTIO_F_INTR_MPSAFE	(1 << 0)
#define VIRTIO_F_INTR_SOFTINT	(1 << 1)
#define VIRTIO_F_INTR_MSIX	(1 << 2)


#define	VIRTIO_CHILD_FAILED		((void *)1)

/* public interface */
void virtio_negotiate_features(struct virtio_softc*, uint64_t);

uint8_t virtio_read_device_config_1(struct virtio_softc *, int);
uint16_t virtio_read_device_config_2(struct virtio_softc *, int);
uint32_t virtio_read_device_config_4(struct virtio_softc *, int);
uint64_t virtio_read_device_config_8(struct virtio_softc *, int);
uint16_t virtio_read_device_config_le_2(struct virtio_softc *, int);
uint32_t virtio_read_device_config_le_4(struct virtio_softc *, int);
void virtio_write_device_config_1(struct virtio_softc *, int, uint8_t);
void virtio_write_device_config_2(struct virtio_softc *, int, uint16_t);
void virtio_write_device_config_4(struct virtio_softc *, int, uint32_t);
void virtio_write_device_config_8(struct virtio_softc *, int, uint64_t);
void virtio_write_device_config_le_2(struct virtio_softc *, int, uint16_t);
void virtio_write_device_config_le_4(struct virtio_softc *, int, uint32_t);

int virtio_alloc_vq(struct virtio_softc*, struct virtqueue*, int, int, int,
		    const char*);
int virtio_free_vq(struct virtio_softc*, struct virtqueue*);
void virtio_reset(struct virtio_softc *);
int virtio_reinit_start(struct virtio_softc *);
void virtio_reinit_end(struct virtio_softc *);
void virtio_child_attach_start(struct virtio_softc *, device_t, int,
                    struct virtqueue *,
                    virtio_callback, virtio_callback, int,
		    int, const char *);
void virtio_child_attach_set_vqs(struct virtio_softc *,
                    struct virtqueue *, int);
int virtio_child_attach_finish(struct virtio_softc *);
void virtio_child_attach_failed(struct virtio_softc *);
void virtio_child_detach(struct virtio_softc *);

int virtio_enqueue_prep(struct virtio_softc*, struct virtqueue*, int*);
int virtio_enqueue_reserve(struct virtio_softc*, struct virtqueue*, int, int);
int virtio_enqueue(struct virtio_softc*, struct virtqueue*, int,
		   bus_dmamap_t, bool);
int virtio_enqueue_p(struct virtio_softc*, struct virtqueue*, int,
		     bus_dmamap_t, bus_addr_t, bus_size_t, bool);
int virtio_enqueue_commit(struct virtio_softc*, struct virtqueue*, int, bool);
int virtio_enqueue_abort(struct virtio_softc*, struct virtqueue*, int);

int virtio_dequeue(struct virtio_softc*, struct virtqueue*, int *, int *);
int virtio_dequeue_commit(struct virtio_softc*, struct virtqueue*, int);

bool virtio_vq_is_enqueued(struct virtio_softc *, struct virtqueue *);
int virtio_vq_intr(struct virtio_softc *);
int virtio_vq_intrhand(struct virtio_softc *);
int virtio_postpone_intr(struct virtio_softc *sc, struct virtqueue *vq,
		uint16_t nslots);
int virtio_postpone_intr_smart(struct virtio_softc *sc, struct virtqueue *vq);
int virtio_postpone_intr_far(struct virtio_softc *sc, struct virtqueue *vq);
void virtio_stop_vq_intr(struct virtio_softc *, struct virtqueue *);
int virtio_start_vq_intr(struct virtio_softc *, struct virtqueue *);

/* encapsulation */
bus_dma_tag_t	virtio_dmat(struct virtio_softc *);
device_t	virtio_child(struct virtio_softc *);
int		virtio_intrhand(struct virtio_softc *);
uint64_t	virtio_features(struct virtio_softc *);

/* autoconf(9) common */
void virtio_set_status(struct virtio_softc *, int);
void virtio_print_device_type(device_t, int, int);
int virtio_attach_failed(struct virtio_softc *);

#define virtio_device_reset(sc)	virtio_set_status((sc), 0)

/* endian conversion */

/*
 * Virtio structures are read/written in host endian for 0.9 but little endian
 * for 1.0. One notable exception is AArch BE that always uses little endian.
 */
uint16_t virtio_rw16(struct virtio_softc *sc, uint16_t val);
uint32_t virtio_rw32(struct virtio_softc *sc, uint32_t val);
uint64_t virtio_rw64(struct virtio_softc *sc, uint64_t val);

#endif /* _DEV_PCI_VIRTIOVAR_H_ */
