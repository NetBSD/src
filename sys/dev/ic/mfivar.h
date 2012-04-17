/* $NetBSD: mfivar.h,v 1.14.10.1 2012/04/17 00:07:35 yamt Exp $ */
/* $OpenBSD: mfivar.h,v 1.28 2006/08/31 18:18:46 marco Exp $ */
/*
 * Copyright (c) 2006 Marco Peereboom <marco@peereboom.us>
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

#include <dev/sysmon/sysmonvar.h>
#include <sys/envsys.h>

#define DEVNAME(_s)     (device_xname((_s)->sc_dev))

/* #define MFI_DEBUG */
#ifdef MFI_DEBUG
extern uint32_t			mfi_debug;
#define DPRINTF(x...)		do { if (mfi_debug) printf(x); } while(0)
#define DNPRINTF(n,x...)	do { if (mfi_debug & n) printf(x); } while(0)
#define	MFI_D_CMD		0x0001
#define	MFI_D_INTR		0x0002
#define	MFI_D_MISC		0x0004
#define	MFI_D_DMA		0x0008
#define	MFI_D_IOCTL		0x0010
#define	MFI_D_RW		0x0020
#define	MFI_D_MEM		0x0040
#define	MFI_D_CCB		0x0080
#else
#define DPRINTF(x, ...)
#define DNPRINTF(n, x, ...)
#endif

struct mfi_mem {
	bus_dmamap_t		am_map;
	bus_dma_segment_t	am_seg;
	size_t			am_size;
	void *			am_kva;
};

#define MFIMEM_MAP(_am)		((_am)->am_map)
#define MFIMEM_DVA(_am)		((_am)->am_map->dm_segs[0].ds_addr)
#define MFIMEM_KVA(_am)		((void *)(_am)->am_kva)

struct mfi_prod_cons {
	uint32_t		mpc_producer;
	uint32_t		mpc_consumer;
	uint32_t		mpc_reply_q[1]; /* compensate for 1 extra reply per spec */
};

struct mfi_ccb {
	struct mfi_softc	*ccb_sc;

	union mfi_frame		*ccb_frame;
	paddr_t			ccb_pframe;
	uint32_t		ccb_frame_size;
	uint32_t		ccb_extra_frames;

	struct mfi_sense	*ccb_sense;
	paddr_t			ccb_psense;

	bus_dmamap_t		ccb_dmamap;

	union mfi_sgl		*ccb_sgl;

	/* data for sgl */
	void			*ccb_data;
	uint32_t		ccb_len;

	uint32_t		ccb_direction;
#define MFI_DATA_NONE	0
#define MFI_DATA_IN	1
#define MFI_DATA_OUT	2

	struct scsipi_xfer	*ccb_xs;

	void			(*ccb_done)(struct mfi_ccb *);

	volatile enum {
		MFI_CCB_FREE,
		MFI_CCB_READY,
		MFI_CCB_DONE
	}			ccb_state;
	uint32_t		ccb_flags;
#define MFI_CCB_F_ERR			(1<<0)
	TAILQ_ENTRY(mfi_ccb)	ccb_link;
};

TAILQ_HEAD(mfi_ccb_list, mfi_ccb);

enum mfi_iop {
	MFI_IOP_XSCALE,
	MFI_IOP_PPC,
	MFI_IOP_GEN2,
	MFI_IOP_SKINNY
};

struct mfi_iop_ops {
	uint32_t 		(*mio_fw_state)(struct mfi_softc *);
	void 			(*mio_intr_dis)(struct mfi_softc *);
	void 			(*mio_intr_ena)(struct mfi_softc *);
	int 			(*mio_intr)(struct mfi_softc *);
	void 			(*mio_post)(struct mfi_softc *, struct mfi_ccb *);
};

struct mfi_softc {
	device_t		sc_dev;
	struct scsipi_channel	sc_chan;
	struct scsipi_adapter	sc_adapt;

	const struct mfi_iop_ops *sc_iop;

	void			*sc_ih;

	uint32_t		sc_flags;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_size_t		sc_size;

	/* save some useful information for logical drives that is missing
	 * in sc_ld_list
	 */
	struct {
		uint32_t	ld_present;
		char		ld_dev[16];	/* device name sd? */
	}			sc_ld[MFI_MAX_LD];

	/* firmware determined max, totals and other information*/
	uint32_t		sc_max_cmds;
	uint32_t		sc_max_sgl;
	uint32_t		sc_max_ld;
	uint32_t		sc_ld_cnt;
	/* XXX these struct should be local to mgmt function */
	struct mfi_ctrl_info	sc_info;
	struct mfi_ld_list	sc_ld_list;
	struct mfi_ld_details	sc_ld_details;

	/* all commands */
	struct mfi_ccb		*sc_ccb;

	/* producer/consumer pointers and reply queue */
	struct mfi_mem		*sc_pcq;

	/* frame memory */
	struct mfi_mem		*sc_frames;
	uint32_t		sc_frames_size;

	/* sense memory */
	struct mfi_mem		*sc_sense;

	struct mfi_ccb_list	sc_ccb_freeq;

	struct sysmon_envsys    *sc_sme;
	envsys_data_t		*sc_sensor;

	device_t		sc_child;
};

int	mfi_rescan(device_t, const char *, const int *);
void	mfi_childdetached(device_t, device_t);
int	mfi_attach(struct mfi_softc *, enum mfi_iop);
int	mfi_detach(struct mfi_softc *, int);
int	mfi_intr(void *);
