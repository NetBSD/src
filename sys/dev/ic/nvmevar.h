/*	$NetBSD: nvmevar.h,v 1.13.14.1 2018/03/22 01:44:48 pgoyette Exp $	*/
/*	$OpenBSD: nvmevar.h,v 1.8 2016/04/14 11:18:32 dlg Exp $ */

/*
 * Copyright (c) 2014 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>

struct nvme_dmamem {
	bus_dmamap_t		ndm_map;
	bus_dma_segment_t	ndm_seg;
	size_t			ndm_size;
	void			*ndm_kva;
};
#define NVME_DMA_MAP(_ndm)	((_ndm)->ndm_map)
#define NVME_DMA_LEN(_ndm)	((_ndm)->ndm_map->dm_segs[0].ds_len)
#define NVME_DMA_DVA(_ndm)	((uint64_t)(_ndm)->ndm_map->dm_segs[0].ds_addr)
#define NVME_DMA_KVA(_ndm)	((void *)(_ndm)->ndm_kva)

struct nvme_softc;
struct nvme_queue;

typedef void (*nvme_nnc_done)(void *, struct buf *, uint16_t, uint32_t);

struct nvme_ccb {
	SIMPLEQ_ENTRY(nvme_ccb)	ccb_entry;

	/* DMA handles */
	bus_dmamap_t		ccb_dmamap;

	bus_addr_t		ccb_prpl_off;
	uint64_t		ccb_prpl_dva;
	uint64_t		*ccb_prpl;

	/* command context */
	uint16_t		ccb_id;
	void			*ccb_cookie;
#define NVME_CCB_FREE	0xbeefdeed
	void			(*ccb_done)(struct nvme_queue *,
				    struct nvme_ccb *, struct nvme_cqe *);

	/* namespace context */
	void		*nnc_cookie;
	nvme_nnc_done	nnc_done;
	uint16_t	nnc_nsid;
	uint16_t	nnc_flags;
#define	NVME_NS_CTX_F_READ	__BIT(0)
#define	NVME_NS_CTX_F_POLL	__BIT(1)
#define	NVME_NS_CTX_F_FUA	__BIT(2)

	struct buf	*nnc_buf;
	daddr_t		nnc_blkno;
	size_t		nnc_datasize;
	int		nnc_secsize;
};

struct nvme_queue {
	struct nvme_softc	*q_sc;
	kmutex_t		q_sq_mtx;
	kmutex_t		q_cq_mtx;
	struct nvme_dmamem	*q_sq_dmamem;
	struct nvme_dmamem	*q_cq_dmamem;
	bus_size_t 		q_sqtdbl; /* submission queue tail doorbell */
	bus_size_t 		q_cqhdbl; /* completion queue head doorbell */
	uint16_t		q_id;
	uint32_t		q_entries;
	uint32_t		q_sq_tail;
	uint32_t		q_cq_head;
	uint16_t		q_cq_phase;

	kmutex_t		q_ccb_mtx;
	kcondvar_t		q_ccb_wait;	/* wait for ccb avail/finish */
	bool			q_ccb_waiting;	/* whether there are waiters */
	uint16_t		q_nccbs;	/* total number of ccbs */
	struct nvme_ccb		*q_ccbs;
	SIMPLEQ_HEAD(, nvme_ccb) q_ccb_list;
	struct nvme_dmamem	*q_ccb_prpls;
};

struct nvme_namespace {
	struct nvm_identify_namespace *ident;
	device_t dev;
	uint32_t flags;
#define	NVME_NS_F_OPEN	__BIT(0)
};

struct nvme_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	bus_dma_tag_t		sc_dmat;

	int			(*sc_intr_establish)(struct nvme_softc *,
				    uint16_t qid, struct nvme_queue *);
	int			(*sc_intr_disestablish)(struct nvme_softc *,
				    uint16_t qid);
	void			**sc_ih;	/* interrupt handlers */
	void			**sc_softih;	/* softintr handlers */

	u_int			sc_rdy_to;	/* RDY timeout */
	size_t			sc_mps;		/* memory page size */  
	size_t			sc_mdts;	/* max data trasfer size */
	u_int			sc_max_sgl;	/* max S/G segments */

	struct nvm_identify_controller
				sc_identify;

	u_int			sc_nn;		/* namespace count */
	struct nvme_namespace	*sc_namespaces;

	bool			sc_use_mq;
	u_int			sc_nq;		/* # of io queue (sc_q) */
	struct nvme_queue	*sc_admin_q;
	struct nvme_queue	**sc_q;

	uint32_t		sc_flags;
#define	NVME_F_ATTACHED	__BIT(0)
#define	NVME_F_OPEN	__BIT(1)
};

#define	lemtoh16(p)	le16toh(*((uint16_t *)(p)))
#define	lemtoh32(p)	le32toh(*((uint32_t *)(p)))
#define	lemtoh64(p)	le64toh(*((uint64_t *)(p)))
#define	htolem16(p, x)	(*((uint16_t *)(p)) = htole16(x))
#define	htolem32(p, x)	(*((uint32_t *)(p)) = htole32(x))
#define	htolem64(p, x)	(*((uint64_t *)(p)) = htole64(x))

struct nvme_attach_args {
	uint16_t	naa_nsid;
	uint32_t	naa_qentries;	/* total number of queue slots */
	uint32_t	naa_maxphys;	/* maximum device transfer size */
};

int	nvme_attach(struct nvme_softc *);
int	nvme_detach(struct nvme_softc *, int flags);
int	nvme_rescan(device_t, const char *, const int *);
void	nvme_childdet(device_t, device_t);
int	nvme_intr(void *);
void	nvme_softintr_intx(void *);
int	nvme_intr_msi(void *);
void	nvme_softintr_msi(void *);

static inline struct nvme_queue *
nvme_get_q(struct nvme_softc *sc)
{
	return sc->sc_q[cpu_index(curcpu()) % sc->sc_nq];
}

/*
 * namespace
 */
static inline struct nvme_namespace *
nvme_ns_get(struct nvme_softc *sc, uint16_t nsid)
{
	if (nsid == 0 || nsid - 1 >= sc->sc_nn)
		return NULL;
	return &sc->sc_namespaces[nsid - 1];
}

int	nvme_ns_identify(struct nvme_softc *, uint16_t);
void	nvme_ns_free(struct nvme_softc *, uint16_t);
int	nvme_ns_dobio(struct nvme_softc *, uint16_t, void *,
    struct buf *, void *, size_t, int, daddr_t, int, nvme_nnc_done);
int	nvme_ns_sync(struct nvme_softc *, uint16_t, int);
int	nvme_admin_getcache(struct nvme_softc *, int *);
