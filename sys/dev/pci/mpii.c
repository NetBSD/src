/* $NetBSD: mpii.c,v 1.8.10.4 2019/01/07 13:49:39 martin Exp $ */
/*	OpenBSD: mpii.c,v 1.115 2012/04/11 13:29:14 naddy Exp 	*/
/*
 * Copyright (c) 2010 Mike Belopuhov <mkb@crypt.org.ru>
 * Copyright (c) 2009 James Giannoules
 * Copyright (c) 2005 - 2010 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2005 - 2010 Marco Peereboom <marco@openbsd.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpii.c,v 1.8.10.4 2019/01/07 13:49:39 martin Exp $");

#include "bio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/dkio.h>
#include <sys/tree.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>

#if NBIO > 0
#include <dev/biovar.h>
#include <dev/sysmon/sysmonvar.h>     
#include <sys/envsys.h>
#endif

#include <dev/pci/mpiireg.h>

// #define MPII_DEBUG
#ifdef MPII_DEBUG
#define DPRINTF(x...)		do { if (mpii_debug) printf(x); } while(0)
#define DNPRINTF(n,x...)	do { if (mpii_debug & (n)) printf(x); } while(0)
#define	MPII_D_CMD		(0x0001)
#define	MPII_D_INTR		(0x0002)
#define	MPII_D_MISC		(0x0004)
#define	MPII_D_DMA		(0x0008)
#define	MPII_D_IOCTL		(0x0010)
#define	MPII_D_RW		(0x0020)
#define	MPII_D_MEM		(0x0040)
#define	MPII_D_CCB		(0x0080)
#define	MPII_D_PPR		(0x0100)
#define	MPII_D_RAID		(0x0200)
#define	MPII_D_EVT		(0x0400)
#define MPII_D_CFG		(0x0800)
#define MPII_D_MAP		(0x1000)

u_int32_t  mpii_debug = 0
//		| MPII_D_CMD
//		| MPII_D_INTR
//		| MPII_D_MISC
//		| MPII_D_DMA
//		| MPII_D_IOCTL
//		| MPII_D_RW
//		| MPII_D_MEM
//		| MPII_D_CCB
//		| MPII_D_PPR
//		| MPII_D_RAID
//		| MPII_D_EVT
//		| MPII_D_CFG
//		| MPII_D_MAP
	;
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define MPII_REQUEST_SIZE		(512)
#define MPII_REQUEST_CREDIT		(128)

struct mpii_dmamem {
	bus_dmamap_t		mdm_map;
	bus_dma_segment_t	mdm_seg;
	size_t			mdm_size;
	void 			*mdm_kva;
};
#define MPII_DMA_MAP(_mdm) ((_mdm)->mdm_map)
#define MPII_DMA_DVA(_mdm) ((uint64_t)(_mdm)->mdm_map->dm_segs[0].ds_addr)
#define MPII_DMA_KVA(_mdm) ((_mdm)->mdm_kva)

struct mpii_softc;

struct mpii_rcb {
	SIMPLEQ_ENTRY(mpii_rcb)	rcb_link;
	void			*rcb_reply;
	u_int32_t		rcb_reply_dva;
};

SIMPLEQ_HEAD(mpii_rcb_list, mpii_rcb);

struct mpii_device {
	int			flags;
#define MPII_DF_ATTACH		(0x0001)
#define MPII_DF_DETACH		(0x0002)
#define MPII_DF_HIDDEN		(0x0004)
#define MPII_DF_UNUSED		(0x0008)
#define MPII_DF_VOLUME		(0x0010)
#define MPII_DF_VOLUME_DISK	(0x0020)
#define MPII_DF_HOT_SPARE	(0x0040)
	short			slot;
	short			percent;
	u_int16_t		dev_handle;
	u_int16_t		enclosure;
	u_int16_t		expander;
	u_int8_t		phy_num;
	u_int8_t		physical_port;
};

struct mpii_ccb {
	struct mpii_softc	*ccb_sc;

	void *			ccb_cookie;
	kmutex_t		ccb_mtx;
	kcondvar_t		ccb_cv;

	bus_dmamap_t		ccb_dmamap;

	bus_addr_t		ccb_offset;
	void			*ccb_cmd;
	bus_addr_t		ccb_cmd_dva;
	u_int16_t		ccb_dev_handle;
	u_int16_t		ccb_smid;

	volatile enum {
		MPII_CCB_FREE,
		MPII_CCB_READY,
		MPII_CCB_QUEUED,
		MPII_CCB_TIMEOUT
	}			ccb_state;

	void			(*ccb_done)(struct mpii_ccb *);
	struct mpii_rcb		*ccb_rcb;

	SIMPLEQ_ENTRY(mpii_ccb)	ccb_link;
};

SIMPLEQ_HEAD(mpii_ccb_list, mpii_ccb);

struct mpii_softc {
	device_t		sc_dev;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;

	void			*sc_ih;

	struct scsipi_adapter	sc_adapt;
	struct scsipi_channel	sc_chan;
	device_t		sc_child; /* our scsibus */

	int			sc_flags;
#define MPII_F_RAID		(1<<1)
#define MPII_F_SAS3		(1<<2)

	struct mpii_device	**sc_devs;
	kmutex_t		sc_devs_mtx;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	bus_dma_tag_t		sc_dmat;

	kmutex_t		sc_req_mtx;
	kmutex_t		sc_rep_mtx;

	ushort			sc_reply_size;
	ushort			sc_request_size;

	ushort			sc_max_cmds;
	ushort			sc_num_reply_frames;
	u_int			sc_reply_free_qdepth;
	u_int			sc_reply_post_qdepth;

	ushort			sc_chain_sge;
	ushort			sc_max_sgl;

	u_int8_t		sc_ioc_event_replay;

	u_int8_t		sc_porttype;
	u_int8_t		sc_max_volumes;
	u_int16_t		sc_max_devices;
	u_int16_t		sc_vd_count;
	u_int16_t		sc_vd_id_low;
	u_int16_t		sc_pd_id_start;
	int			sc_ioc_number;
	u_int8_t		sc_vf_id;

	struct mpii_ccb		*sc_ccbs;
	struct mpii_ccb_list	sc_ccb_free;
	kmutex_t		sc_ccb_free_mtx;
	kcondvar_t		sc_ccb_free_cv;

	struct mpii_ccb_list	sc_ccb_tmos;
	kmutex_t		sc_ssb_tmomtx;
	struct workqueue	*sc_ssb_tmowk;
	struct work		sc_ssb_tmowork;

	struct mpii_dmamem	*sc_requests;

	struct mpii_dmamem	*sc_replies;
	struct mpii_rcb		*sc_rcbs;

	struct mpii_dmamem	*sc_reply_postq;
	struct mpii_reply_descr	*sc_reply_postq_kva;
	u_int			sc_reply_post_host_index;

	struct mpii_dmamem	*sc_reply_freeq;
	u_int			sc_reply_free_host_index;
	kmutex_t		sc_reply_free_mtx;

	struct mpii_rcb_list	sc_evt_sas_queue;
	kmutex_t		sc_evt_sas_mtx;
	struct workqueue	*sc_evt_sas_wq;
	struct work		sc_evt_sas_work;

	struct mpii_rcb_list	sc_evt_ack_queue;
	kmutex_t		sc_evt_ack_mtx;
	struct workqueue	*sc_evt_ack_wq;
	struct work		sc_evt_ack_work;

	struct sysmon_envsys	*sc_sme;
	envsys_data_t		*sc_sensors;
};

int	mpii_match(device_t, cfdata_t, void *);
void	mpii_attach(device_t, device_t, void *);
int	mpii_detach(device_t, int);
void	mpii_childdetached(device_t, device_t);
int	mpii_rescan(device_t, const char *, const int *);

int	mpii_intr(void *);

CFATTACH_DECL3_NEW(mpii, sizeof(struct mpii_softc),
    mpii_match, mpii_attach, mpii_detach, NULL, mpii_rescan,
    mpii_childdetached, DVF_DETACH_SHUTDOWN);

void		mpii_scsipi_request(struct scsipi_channel *,
			scsipi_adapter_req_t, void *);
void		mpii_scsi_cmd_done(struct mpii_ccb *);

struct mpii_dmamem *
		mpii_dmamem_alloc(struct mpii_softc *, size_t);
void		mpii_dmamem_free(struct mpii_softc *,
		    struct mpii_dmamem *);
int		mpii_alloc_ccbs(struct mpii_softc *);
struct mpii_ccb *mpii_get_ccb(struct mpii_softc *);
void		mpii_put_ccb(struct mpii_softc *, struct mpii_ccb *);
int		mpii_alloc_replies(struct mpii_softc *);
int		mpii_alloc_queues(struct mpii_softc *);
void		mpii_push_reply(struct mpii_softc *, struct mpii_rcb *);
void		mpii_push_replies(struct mpii_softc *);

void		mpii_scsi_cmd_tmo(void *);
void		mpii_scsi_cmd_tmo_handler(struct work *, void *);
void		mpii_scsi_cmd_tmo_done(struct mpii_ccb *);

int		mpii_insert_dev(struct mpii_softc *, struct mpii_device *);
int		mpii_remove_dev(struct mpii_softc *, struct mpii_device *);
struct mpii_device *
		mpii_find_dev(struct mpii_softc *, u_int16_t);

void		mpii_start(struct mpii_softc *, struct mpii_ccb *);
int		mpii_poll(struct mpii_softc *, struct mpii_ccb *);
void		mpii_poll_done(struct mpii_ccb *);
struct mpii_rcb *
		mpii_reply(struct mpii_softc *, struct mpii_reply_descr *);

void		mpii_wait(struct mpii_softc *, struct mpii_ccb *);
void		mpii_wait_done(struct mpii_ccb *);

void		mpii_init_queues(struct mpii_softc *);

int		mpii_load_xs(struct mpii_ccb *);
int		mpii_load_xs_sas3(struct mpii_ccb *);

u_int32_t	mpii_read(struct mpii_softc *, bus_size_t);
void		mpii_write(struct mpii_softc *, bus_size_t, u_int32_t);
int		mpii_wait_eq(struct mpii_softc *, bus_size_t, u_int32_t,
		    u_int32_t);
int		mpii_wait_ne(struct mpii_softc *, bus_size_t, u_int32_t,
		    u_int32_t);

int		mpii_init(struct mpii_softc *);
int		mpii_reset_soft(struct mpii_softc *);
int		mpii_reset_hard(struct mpii_softc *);

int		mpii_handshake_send(struct mpii_softc *, void *, size_t);
int		mpii_handshake_recv_dword(struct mpii_softc *,
		    u_int32_t *);
int		mpii_handshake_recv(struct mpii_softc *, void *, size_t);

void		mpii_empty_done(struct mpii_ccb *);

int		mpii_iocinit(struct mpii_softc *);
int		mpii_iocfacts(struct mpii_softc *);
int		mpii_portfacts(struct mpii_softc *);
int		mpii_portenable(struct mpii_softc *);
int		mpii_cfg_coalescing(struct mpii_softc *);
int		mpii_board_info(struct mpii_softc *);
int		mpii_target_map(struct mpii_softc *);

int		mpii_eventnotify(struct mpii_softc *);
void		mpii_eventnotify_done(struct mpii_ccb *);
void		mpii_eventack(struct work *, void *);
void		mpii_eventack_done(struct mpii_ccb *);
void		mpii_event_process(struct mpii_softc *, struct mpii_rcb *);
void		mpii_event_done(struct mpii_softc *, struct mpii_rcb *);
void		mpii_event_sas(struct mpii_softc *, struct mpii_rcb *);
void		mpii_event_sas_work(struct work *, void *);
void		mpii_event_raid(struct mpii_softc *,
		    struct mpii_msg_event_reply *);
void		mpii_event_discovery(struct mpii_softc *,
		    struct mpii_msg_event_reply *);

void		mpii_sas_remove_device(struct mpii_softc *, u_int16_t);

int		mpii_req_cfg_header(struct mpii_softc *, u_int8_t,
		    u_int8_t, u_int32_t, int, void *);
int		mpii_req_cfg_page(struct mpii_softc *, u_int32_t, int,
		    void *, int, void *, size_t);

#if 0
int		mpii_ioctl_cache(struct scsi_link *, u_long, struct dk_cache *);
#endif

#if NBIO > 0
int		mpii_ioctl(device_t, u_long, void *);
int		mpii_ioctl_inq(struct mpii_softc *, struct bioc_inq *);
int		mpii_ioctl_vol(struct mpii_softc *, struct bioc_vol *);
int		mpii_ioctl_disk(struct mpii_softc *, struct bioc_disk *);
int		mpii_bio_hs(struct mpii_softc *, struct bioc_disk *, int,
		    int, int *);
int		mpii_bio_disk(struct mpii_softc *, struct bioc_disk *,
		    u_int8_t);
struct mpii_device *
		mpii_find_vol(struct mpii_softc *, int);
#ifndef SMALL_KERNEL
 int		mpii_bio_volstate(struct mpii_softc *, struct bioc_vol *);
int		mpii_create_sensors(struct mpii_softc *);
void		mpii_refresh_sensors(struct sysmon_envsys *, envsys_data_t *);
int		mpii_destroy_sensors(struct mpii_softc *);
#endif /* SMALL_KERNEL */
#endif /* NBIO > 0 */

#define DEVNAME(s)		(device_xname((s)->sc_dev))

#define dwordsof(s)		(sizeof(s) / sizeof(u_int32_t))

#define mpii_read_db(s)		mpii_read((s), MPII_DOORBELL)
#define mpii_write_db(s, v)	mpii_write((s), MPII_DOORBELL, (v))
#define mpii_read_intr(s)	mpii_read((s), MPII_INTR_STATUS)
#define mpii_write_intr(s, v)	mpii_write((s), MPII_INTR_STATUS, (v))
#define mpii_reply_waiting(s)	((mpii_read_intr((s)) & MPII_INTR_STATUS_REPLY)\
				    == MPII_INTR_STATUS_REPLY)

#define mpii_write_reply_free(s, v) \
    bus_space_write_4((s)->sc_iot, (s)->sc_ioh, \
    MPII_REPLY_FREE_HOST_INDEX, (v))
#define mpii_write_reply_post(s, v) \
    bus_space_write_4((s)->sc_iot, (s)->sc_ioh, \
    MPII_REPLY_POST_HOST_INDEX, (v))

#define mpii_wait_db_int(s)	mpii_wait_ne((s), MPII_INTR_STATUS, \
				    MPII_INTR_STATUS_IOC2SYSDB, 0)
#define mpii_wait_db_ack(s)	mpii_wait_eq((s), MPII_INTR_STATUS, \
				    MPII_INTR_STATUS_SYS2IOCDB, 0)

static inline void
mpii_dvatosge(struct mpii_sge *sge, u_int64_t dva)
{
	sge->sg_addr_lo = htole32(dva);
	sge->sg_addr_hi = htole32(dva >> 32);
}

#define MPII_PG_EXTENDED	(1<<0)
#define MPII_PG_POLL		(1<<1)
#define MPII_PG_FMT		"\020" "\002POLL" "\001EXTENDED"

static const struct mpii_pci_product {
	pci_vendor_id_t         mpii_vendor;
	pci_product_id_t        mpii_product;
} mpii_devices[] = {
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2004 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2008 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2108_3 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2108_4 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2108_5 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2116_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2116_2 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_2 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_3 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_4 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_5 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2208_6 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2308_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2308_2 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS2308_3 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3004 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3008 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3108_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3108_2 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3108_3 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3108_4 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3408 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3416 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3508 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3508_1 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3516 },
	{ PCI_VENDOR_SYMBIOS,	PCI_PRODUCT_SYMBIOS_SAS3516_1 },
	{ 0, 0}
};

int
mpii_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	const struct mpii_pci_product *mpii;

	for (mpii = mpii_devices; mpii->mpii_vendor != 0; mpii++) {     
		if (PCI_VENDOR(pa->pa_id) == mpii->mpii_vendor &&       
		    PCI_PRODUCT(pa->pa_id) == mpii->mpii_product)       
			return (1);   
	}
	return (0);
}

void
mpii_attach(device_t parent, device_t self, void *aux)
{
	struct mpii_softc		*sc = device_private(self);
	struct pci_attach_args		*pa = aux;
	pcireg_t			memtype;
	int				r;
	pci_intr_handle_t		ih;
	struct mpii_ccb			*ccb;
	struct scsipi_adapter *adapt = &sc->sc_adapt;
	struct scsipi_channel *chan = &sc->sc_chan;
	char intrbuf[PCI_INTRSTR_LEN];
	const char *intrstr;

	pci_aprint_devinfo(pa, NULL);

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_dev = self;

	mutex_init(&sc->sc_req_mtx, MUTEX_DEFAULT, IPL_BIO);
	mutex_init(&sc->sc_rep_mtx, MUTEX_DEFAULT, IPL_BIO);

	/* find the appropriate memory base */
	for (r = PCI_MAPREG_START; r < PCI_MAPREG_END; r += sizeof(memtype)) {
		memtype = pci_mapreg_type(sc->sc_pc, sc->sc_tag, r);
		if ((memtype & PCI_MAPREG_TYPE_MASK) == PCI_MAPREG_TYPE_MEM)
			break;
	}
	if (r >= PCI_MAPREG_END) {
		aprint_error_dev(self,
		    "unable to locate system interface registers\n");
		return;
	}

	if (pci_mapreg_map(pa, r, memtype, 0, &sc->sc_iot, &sc->sc_ioh,
	    NULL, &sc->sc_ios) != 0) {
		aprint_error_dev(self,
		    "unable to map system interface registers\n");
		return;
	}

	/* disable the expansion rom */
	pci_conf_write(sc->sc_pc, sc->sc_tag, PCI_MAPREG_ROM,
	    pci_conf_read(sc->sc_pc, sc->sc_tag, PCI_MAPREG_ROM) &
	    ~PCI_MAPREG_ROM_ENABLE);

	/* disable interrupts */
	mpii_write(sc, MPII_INTR_MASK,
	    MPII_INTR_MASK_RESET | MPII_INTR_MASK_REPLY |
	    MPII_INTR_MASK_DOORBELL);

	/* hook up the interrupt */
	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error_dev(self, "unable to map interrupt\n");
		goto unmap;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih, intrbuf, sizeof(intrbuf));
	pci_intr_setattr(pa->pa_pc, &ih, PCI_INTR_MPSAFE, true);
	sc->sc_ih = pci_intr_establish_xname(pa->pa_pc, ih, IPL_BIO,
	    mpii_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	aprint_naive("\n");

	if (mpii_iocfacts(sc) != 0) {
		aprint_error_dev(self,  "unable to get iocfacts\n");
		goto unmap;
	}

	if (mpii_init(sc) != 0) {
		aprint_error_dev(self, "unable to initialize ioc\n");
		goto unmap;
	}

	if (mpii_alloc_ccbs(sc) != 0) {
		/* error already printed */
		goto unmap;
	}

	if (mpii_alloc_replies(sc) != 0) {
		aprint_error_dev(self, "unable to allocated reply space\n");
		goto free_ccbs;
	}

	if (mpii_alloc_queues(sc) != 0) {
		aprint_error_dev(self, "unable to allocate reply queues\n");
		goto free_replies;
	}

	if (mpii_iocinit(sc) != 0) {
		aprint_error_dev(self, "unable to send iocinit\n");
		goto free_queues;
	}

	if (mpii_wait_eq(sc, MPII_DOORBELL, MPII_DOORBELL_STATE,
	    MPII_DOORBELL_STATE_OPER) != 0) {
		aprint_error_dev(self, "state: 0x%08x\n",
			mpii_read_db(sc) & MPII_DOORBELL_STATE);
		aprint_error_dev(self, "operational state timeout\n");
		goto free_queues;
	}

	mpii_push_replies(sc);
	mpii_init_queues(sc);

	if (mpii_board_info(sc) != 0) {
		aprint_error_dev(self, "unable to get manufacturing page 0\n");
		goto free_queues;
	}

	if (mpii_portfacts(sc) != 0) {
		aprint_error_dev(self, "unable to get portfacts\n");
		goto free_queues;
	}

	if (mpii_target_map(sc) != 0) {
		aprint_error_dev(self, "unable to setup target mappings\n");
		goto free_queues;
	}

	if (mpii_cfg_coalescing(sc) != 0) {
		aprint_error_dev(self, "unable to configure coalescing\n");
		goto free_queues;
	}

	/* XXX bail on unsupported porttype? */
	if ((sc->sc_porttype == MPII_PORTFACTS_PORTTYPE_SAS_PHYSICAL) ||
	    (sc->sc_porttype == MPII_PORTFACTS_PORTTYPE_SAS_VIRTUAL) ||
	    (sc->sc_porttype == MPII_PORTFACTS_PORTTYPE_TRI_MODE)) {
		if (mpii_eventnotify(sc) != 0) {
			aprint_error_dev(self, "unable to enable events\n");
			goto free_queues;
		}
	}

	mutex_init(&sc->sc_devs_mtx, MUTEX_DEFAULT, IPL_BIO);
	sc->sc_devs = malloc(sc->sc_max_devices * sizeof(struct mpii_device *),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (sc->sc_devs == NULL) {
		aprint_error_dev(self,
		    "unable to allocate memory for mpii_device\n");
		goto free_queues;
	}

	if (mpii_portenable(sc) != 0) {
		aprint_error_dev(self, "unable to enable port\n");
		goto free_devs;
	}

	/* we should be good to go now, attach scsibus */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = sc->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = sc->sc_max_cmds - 4;
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_request = mpii_scsipi_request;
	adapt->adapt_minphys = minphys;
	adapt->adapt_flags = SCSIPI_ADAPT_MPSAFE;

	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_sas_bustype;
	chan->chan_channel = 0;
	chan->chan_flags = 0;
	chan->chan_nluns = 8;
	chan->chan_ntargets = sc->sc_max_devices;
	chan->chan_id = -1;

	mpii_rescan(self, "scsi", NULL);

	/* enable interrupts */
	mpii_write(sc, MPII_INTR_MASK, MPII_INTR_MASK_DOORBELL
	    | MPII_INTR_MASK_RESET);

#if NBIO > 0
	if (ISSET(sc->sc_flags, MPII_F_RAID)) {
		if (bio_register(sc->sc_dev, mpii_ioctl) != 0)
			panic("%s: controller registration failed",
			    DEVNAME(sc));
		if (mpii_create_sensors(sc) != 0)
			aprint_error_dev(self, "unable to create sensors\n");
	}
#endif

	return;

free_devs:
	free(sc->sc_devs, M_DEVBUF);
	sc->sc_devs = NULL;

free_queues:
	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_reply_freeq),
	    0, sc->sc_reply_free_qdepth * 4, BUS_DMASYNC_POSTREAD);
	mpii_dmamem_free(sc, sc->sc_reply_freeq);

	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_reply_postq),
	    0, sc->sc_reply_post_qdepth * 8, BUS_DMASYNC_POSTREAD);
	mpii_dmamem_free(sc, sc->sc_reply_postq);

free_replies:
	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_replies),
		0, PAGE_SIZE, BUS_DMASYNC_POSTREAD);
	mpii_dmamem_free(sc, sc->sc_replies);

free_ccbs:
	while ((ccb = mpii_get_ccb(sc)) != NULL)
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);
	mpii_dmamem_free(sc, sc->sc_requests);
	free(sc->sc_ccbs, M_DEVBUF);

unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

int
mpii_detach(device_t self, int flags)
{
	struct mpii_softc	*sc = device_private(self);
	int error;
	struct mpii_ccb *ccb;

	if ((error = config_detach_children(sc->sc_dev, flags)) != 0)
		return error;

#if NBIO > 0
	mpii_destroy_sensors(sc);
	bio_unregister(sc->sc_dev);
#endif /* NBIO > 0 */

	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc_ios != 0) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
		free(sc->sc_devs, M_DEVBUF);
		sc->sc_devs = NULL;

		bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_reply_freeq),
		    0, sc->sc_reply_free_qdepth * 4, BUS_DMASYNC_POSTREAD);
		mpii_dmamem_free(sc, sc->sc_reply_freeq);

		bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_reply_postq),
		    0, sc->sc_reply_post_qdepth * 8, BUS_DMASYNC_POSTREAD);
		mpii_dmamem_free(sc, sc->sc_reply_postq);

		bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_replies),
			0, PAGE_SIZE, BUS_DMASYNC_POSTREAD);
		mpii_dmamem_free(sc, sc->sc_replies);

		while ((ccb = mpii_get_ccb(sc)) != NULL)
			bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);
		mpii_dmamem_free(sc, sc->sc_requests);
		free(sc->sc_ccbs, M_DEVBUF);

		sc->sc_ios = 0;
	}

	return (0);
}

int
mpii_rescan(device_t self, const char *ifattr, const int *locators)     
{
	struct mpii_softc *sc = device_private(self);

	if (sc->sc_child != NULL)
		return 0;

	sc->sc_child = config_found_sm_loc(self, ifattr, locators, &sc->sc_chan,
	    scsiprint, NULL);

	return 0;
}

void
mpii_childdetached(device_t self, device_t child)
{
	struct mpii_softc *sc = device_private(self);

	KASSERT(self == sc->sc_dev);  
	KASSERT(child == sc->sc_child);

	if (child == sc->sc_child)    
		sc->sc_child = NULL;  
}


int
mpii_intr(void *arg)
{
	struct mpii_rcb_list		evts = SIMPLEQ_HEAD_INITIALIZER(evts);
	struct mpii_ccb_list		ccbs = SIMPLEQ_HEAD_INITIALIZER(ccbs);
	struct mpii_softc		*sc = arg;
	struct mpii_reply_descr		*postq = sc->sc_reply_postq_kva, *rdp;
	struct mpii_ccb			*ccb;
	struct mpii_rcb			*rcb;
	int				smid;
	u_int				idx;
	int				rv = 0;

	mutex_enter(&sc->sc_rep_mtx);
	bus_dmamap_sync(sc->sc_dmat,
	    MPII_DMA_MAP(sc->sc_reply_postq),
	    0, sc->sc_reply_post_qdepth * sizeof(*rdp),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	idx = sc->sc_reply_post_host_index;
	for (;;) {
		rdp = &postq[idx];
		if ((rdp->reply_flags & MPII_REPLY_DESCR_TYPE_MASK) ==
		    MPII_REPLY_DESCR_UNUSED)
			break;
		if (rdp->data == 0xffffffff) {
			/*
			 * ioc is still writing to the reply post queue
			 * race condition - bail!
			 */
			break;
		}

		smid = le16toh(rdp->smid);
		rcb = mpii_reply(sc, rdp);

		if (smid) {
			ccb = &sc->sc_ccbs[smid - 1];
			ccb->ccb_state = MPII_CCB_READY;
			ccb->ccb_rcb = rcb;
			SIMPLEQ_INSERT_TAIL(&ccbs, ccb, ccb_link);
		} else
			SIMPLEQ_INSERT_TAIL(&evts, rcb, rcb_link);

		if (++idx >= sc->sc_reply_post_qdepth)
			idx = 0;

		rv = 1;
	}

	bus_dmamap_sync(sc->sc_dmat,
	    MPII_DMA_MAP(sc->sc_reply_postq),
	    0, sc->sc_reply_post_qdepth * sizeof(*rdp),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (rv)
		mpii_write_reply_post(sc, sc->sc_reply_post_host_index = idx);

	mutex_exit(&sc->sc_rep_mtx);

	if (rv == 0)
		return (0);

	while ((ccb = SIMPLEQ_FIRST(&ccbs)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&ccbs, ccb_link);
		ccb->ccb_done(ccb);
	}
	while ((rcb = SIMPLEQ_FIRST(&evts)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&evts, rcb_link);
		mpii_event_process(sc, rcb);
	}

	return (1);
}

int
mpii_load_xs_sas3(struct mpii_ccb *ccb)
{
	struct mpii_softc	*sc = ccb->ccb_sc;
	struct scsipi_xfer	*xs = ccb->ccb_cookie;
	struct mpii_msg_scsi_io	*io = ccb->ccb_cmd;
	struct mpii_ieee_sge	*csge, *nsge, *sge;
	bus_dmamap_t		dmap = ccb->ccb_dmamap;
	int			i, error;

	/* Request frame structure is described in the mpii_iocfacts */
	nsge = (struct mpii_ieee_sge *)(io + 1);
	csge = nsge + sc->sc_chain_sge;

	/* zero length transfer still requires an SGE */
	if (xs->datalen == 0) {
		nsge->sg_flags = MPII_IEEE_SGE_END_OF_LIST;
		return (0);
	}

	error = bus_dmamap_load(sc->sc_dmat, dmap, xs->data, xs->datalen, NULL,
	    (xs->xs_control & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	if (error) {
		printf("%s: error %d loading dmamap\n", DEVNAME(sc), error);
		return (1);
	}

	sge = nsge;
	for (i = 0; i < dmap->dm_nsegs; i++, nsge++) {
		if (nsge == csge) {
			nsge++;
			/* offset to the chain sge from the beginning */
			io->chain_offset = ((uintptr_t)csge - (uintptr_t)io) / 4;
			csge->sg_flags = MPII_IEEE_SGE_CHAIN_ELEMENT |
			    MPII_IEEE_SGE_ADDR_SYSTEM;
			/* address of the next sge */
			csge->sg_addr = htole64(ccb->ccb_cmd_dva +
			    ((uintptr_t)nsge - (uintptr_t)io));
			csge->sg_len = htole32((dmap->dm_nsegs - i) *
			    sizeof(*sge));
		}

		sge = nsge;
		sge->sg_flags = MPII_IEEE_SGE_ADDR_SYSTEM;
		sge->sg_len = htole32(dmap->dm_segs[i].ds_len);
		sge->sg_addr = htole64(dmap->dm_segs[i].ds_addr);
	}

	/* terminate list */
	sge->sg_flags |= MPII_IEEE_SGE_END_OF_LIST;

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

	return (0);
}

int
mpii_load_xs(struct mpii_ccb *ccb)
{
	struct mpii_softc	*sc = ccb->ccb_sc;
	struct scsipi_xfer	*xs = ccb->ccb_cookie;
	struct mpii_msg_scsi_io	*io = ccb->ccb_cmd;
	struct mpii_sge		*csge, *nsge, *sge;
	bus_dmamap_t		dmap = ccb->ccb_dmamap;
	u_int32_t		flags;
	u_int16_t		len;
	int			i, error;

	/* Request frame structure is described in the mpii_iocfacts */
	nsge = (struct mpii_sge *)(io + 1);
	csge = nsge + sc->sc_chain_sge;

	/* zero length transfer still requires an SGE */
	if (xs->datalen == 0) {
		nsge->sg_hdr = htole32(MPII_SGE_FL_TYPE_SIMPLE |
		    MPII_SGE_FL_LAST | MPII_SGE_FL_EOB | MPII_SGE_FL_EOL);
		return (0);
	}

	error = bus_dmamap_load(sc->sc_dmat, dmap, xs->data, xs->datalen, NULL,
	    (xs->xs_control & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	if (error) {
		printf("%s: error %d loading dmamap\n", DEVNAME(sc), error);
		return (1);
	}

	/* safe default starting flags */
	flags = MPII_SGE_FL_TYPE_SIMPLE | MPII_SGE_FL_SIZE_64;
	if (xs->xs_control & XS_CTL_DATA_OUT)
		flags |= MPII_SGE_FL_DIR_OUT;

	sge = nsge;
	for (i = 0; i < dmap->dm_nsegs; i++, nsge++) {
		if (nsge == csge) {
			nsge++;
			/* offset to the chain sge from the beginning */
			io->chain_offset = ((uintptr_t)csge - (uintptr_t)io) / 4;
			/* length of the sgl segment we're pointing to */
			len = (dmap->dm_nsegs - i) * sizeof(*sge);
			csge->sg_hdr = htole32(MPII_SGE_FL_TYPE_CHAIN |
			    MPII_SGE_FL_SIZE_64 | len);
			/* address of the next sge */
			mpii_dvatosge(csge, ccb->ccb_cmd_dva +
			    ((uintptr_t)nsge - (uintptr_t)io));
		}

		sge = nsge;
		sge->sg_hdr = htole32(flags | dmap->dm_segs[i].ds_len);
		mpii_dvatosge(sge, dmap->dm_segs[i].ds_addr);
	}

	/* terminate list */
	sge->sg_hdr |= htole32(MPII_SGE_FL_LAST | MPII_SGE_FL_EOB |
	    MPII_SGE_FL_EOL);

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

	return (0);
}

u_int32_t
mpii_read(struct mpii_softc *sc, bus_size_t r)
{
	u_int32_t			rv;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	rv = bus_space_read_4(sc->sc_iot, sc->sc_ioh, r);

	DNPRINTF(MPII_D_RW, "%s: mpii_read %#lx %#x\n", DEVNAME(sc), r, rv);

	return (rv);
}

void
mpii_write(struct mpii_softc *sc, bus_size_t r, u_int32_t v)
{
	DNPRINTF(MPII_D_RW, "%s: mpii_write %#lx %#x\n", DEVNAME(sc), r, v);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}


int
mpii_wait_eq(struct mpii_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{
	int			i;

	DNPRINTF(MPII_D_RW, "%s: mpii_wait_eq %#lx %#x %#x\n", DEVNAME(sc), r,
	    mask, target);

	for (i = 0; i < 15000; i++) {
		if ((mpii_read(sc, r) & mask) == target)
			return (0);
		delay(1000);
	}

	return (1);
}

int
mpii_wait_ne(struct mpii_softc *sc, bus_size_t r, u_int32_t mask,
    u_int32_t target)
{
	int			i;

	DNPRINTF(MPII_D_RW, "%s: mpii_wait_ne %#lx %#x %#x\n", DEVNAME(sc), r,
	    mask, target);

	for (i = 0; i < 15000; i++) {
		if ((mpii_read(sc, r) & mask) != target)
			return (0);
		delay(1000);
	}

	return (1);
}

int
mpii_init(struct mpii_softc *sc)
{
	u_int32_t		db;
	int			i;

	/* spin until the ioc leaves the reset state */
	if (mpii_wait_ne(sc, MPII_DOORBELL, MPII_DOORBELL_STATE,
	    MPII_DOORBELL_STATE_RESET) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_init timeout waiting to leave "
		    "reset state\n", DEVNAME(sc));
		return (1);
	}

	/* check current ownership */
	db = mpii_read_db(sc);
	if ((db & MPII_DOORBELL_WHOINIT) == MPII_DOORBELL_WHOINIT_PCIPEER) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_init initialised by pci peer\n",
		    DEVNAME(sc));
		return (0);
	}

	for (i = 0; i < 5; i++) {
		switch (db & MPII_DOORBELL_STATE) {
		case MPII_DOORBELL_STATE_READY:
			DNPRINTF(MPII_D_MISC, "%s: mpii_init ioc is ready\n",
			    DEVNAME(sc));
			return (0);

		case MPII_DOORBELL_STATE_OPER:
			DNPRINTF(MPII_D_MISC, "%s: mpii_init ioc is oper\n",
			    DEVNAME(sc));
			if (sc->sc_ioc_event_replay)
				mpii_reset_soft(sc);
			else
				mpii_reset_hard(sc);
			break;

		case MPII_DOORBELL_STATE_FAULT:
			DNPRINTF(MPII_D_MISC, "%s: mpii_init ioc is being "
			    "reset hard\n" , DEVNAME(sc));
			mpii_reset_hard(sc);
			break;

		case MPII_DOORBELL_STATE_RESET:
			DNPRINTF(MPII_D_MISC, "%s: mpii_init waiting to come "
			    "out of reset\n", DEVNAME(sc));
			if (mpii_wait_ne(sc, MPII_DOORBELL, MPII_DOORBELL_STATE,
			    MPII_DOORBELL_STATE_RESET) != 0)
				return (1);
			break;
		}
		db = mpii_read_db(sc);
	}

	return (1);
}

int
mpii_reset_soft(struct mpii_softc *sc)
{
	DNPRINTF(MPII_D_MISC, "%s: mpii_reset_soft\n", DEVNAME(sc));

	if (mpii_read_db(sc) & MPII_DOORBELL_INUSE) {
		return (1);
	}

	mpii_write_db(sc,
	    MPII_DOORBELL_FUNCTION(MPII_FUNCTION_IOC_MESSAGE_UNIT_RESET));

	/* XXX LSI waits 15 sec */
	if (mpii_wait_db_ack(sc) != 0)
		return (1);

	/* XXX LSI waits 15 sec */
	if (mpii_wait_eq(sc, MPII_DOORBELL, MPII_DOORBELL_STATE,
	    MPII_DOORBELL_STATE_READY) != 0)
		return (1);

	/* XXX wait for Sys2IOCDB bit to clear in HIS?? */

	return (0);
}

int
mpii_reset_hard(struct mpii_softc *sc)
{
	u_int16_t		i;

	DNPRINTF(MPII_D_MISC, "%s: mpii_reset_hard\n", DEVNAME(sc));

	mpii_write_intr(sc, 0);

	/* enable diagnostic register */
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_FLUSH);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_1);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_2);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_3);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_4);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_5);
	mpii_write(sc, MPII_WRITESEQ, MPII_WRITESEQ_6);

	delay(100);

	if ((mpii_read(sc, MPII_HOSTDIAG) & MPII_HOSTDIAG_DWRE) == 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_reset_hard failure to enable "
		    "diagnostic read/write\n", DEVNAME(sc));
		return(1);
	}

	/* reset ioc */
	mpii_write(sc, MPII_HOSTDIAG, MPII_HOSTDIAG_RESET_ADAPTER);

	/* 240 milliseconds */
	delay(240000);


	/* XXX this whole function should be more robust */

	/* XXX  read the host diagnostic reg until reset adapter bit clears ? */
	for (i = 0; i < 30000; i++) {
		if ((mpii_read(sc, MPII_HOSTDIAG) &
		    MPII_HOSTDIAG_RESET_ADAPTER) == 0)
			break;
		delay(10000);
	}

	/* disable diagnostic register */
	mpii_write(sc, MPII_WRITESEQ, 0xff);

	/* XXX what else? */

	DNPRINTF(MPII_D_MISC, "%s: done with mpii_reset_hard\n", DEVNAME(sc));

	return(0);
}

int
mpii_handshake_send(struct mpii_softc *sc, void *buf, size_t dwords)
{
	u_int32_t		*query = buf;
	int			i;

	/* make sure the doorbell is not in use. */
	if (mpii_read_db(sc) & MPII_DOORBELL_INUSE)
		return (1);

	/* clear pending doorbell interrupts */
	if (mpii_read_intr(sc) & MPII_INTR_STATUS_IOC2SYSDB)
		mpii_write_intr(sc, 0);

	/*
	 * first write the doorbell with the handshake function and the
	 * dword count.
	 */
	mpii_write_db(sc, MPII_DOORBELL_FUNCTION(MPII_FUNCTION_HANDSHAKE) |
	    MPII_DOORBELL_DWORDS(dwords));

	/*
	 * the doorbell used bit will be set because a doorbell function has
	 * started. wait for the interrupt and then ack it.
	 */
	if (mpii_wait_db_int(sc) != 0)
		return (1);
	mpii_write_intr(sc, 0);

	/* poll for the acknowledgement. */
	if (mpii_wait_db_ack(sc) != 0)
		return (1);

	/* write the query through the doorbell. */
	for (i = 0; i < dwords; i++) {
		mpii_write_db(sc, htole32(query[i]));
		if (mpii_wait_db_ack(sc) != 0)
			return (1);
	}

	return (0);
}

int
mpii_handshake_recv_dword(struct mpii_softc *sc, u_int32_t *dword)
{
	u_int16_t		*words = (u_int16_t *)dword;
	int			i;

	for (i = 0; i < 2; i++) {
		if (mpii_wait_db_int(sc) != 0)
			return (1);
		words[i] = le16toh(mpii_read_db(sc) & MPII_DOORBELL_DATA_MASK);
		mpii_write_intr(sc, 0);
	}

	return (0);
}

int
mpii_handshake_recv(struct mpii_softc *sc, void *buf, size_t dwords)
{
	struct mpii_msg_reply	*reply = buf;
	u_int32_t		*dbuf = buf, dummy;
	int			i;

	/* get the first dword so we can read the length out of the header. */
	if (mpii_handshake_recv_dword(sc, &dbuf[0]) != 0)
		return (1);

	DNPRINTF(MPII_D_CMD, "%s: mpii_handshake_recv dwords: %lu reply: %d\n",
	    DEVNAME(sc), dwords, reply->msg_length);

	/*
	 * the total length, in dwords, is in the message length field of the
	 * reply header.
	 */
	for (i = 1; i < MIN(dwords, reply->msg_length); i++) {
		if (mpii_handshake_recv_dword(sc, &dbuf[i]) != 0)
			return (1);
	}

	/* if there's extra stuff to come off the ioc, discard it */
	while (i++ < reply->msg_length) {
		if (mpii_handshake_recv_dword(sc, &dummy) != 0)
			return (1);
		DNPRINTF(MPII_D_CMD, "%s: mpii_handshake_recv dummy read: "
		    "0x%08x\n", DEVNAME(sc), dummy);
	}

	/* wait for the doorbell used bit to be reset and clear the intr */
	if (mpii_wait_db_int(sc) != 0)
		return (1);

	if (mpii_wait_eq(sc, MPII_DOORBELL, MPII_DOORBELL_INUSE, 0) != 0)
		return (1);

	mpii_write_intr(sc, 0);

	return (0);
}

void
mpii_empty_done(struct mpii_ccb *ccb)
{
	/* nothing to do */
}

int
mpii_iocfacts(struct mpii_softc *sc)
{
	struct mpii_msg_iocfacts_request	ifq;
	struct mpii_msg_iocfacts_reply		ifp;
	int					irs;
	int					sge_size;
	u_int					qdepth;

	DNPRINTF(MPII_D_MISC, "%s: mpii_iocfacts\n", DEVNAME(sc));

	memset(&ifq, 0, sizeof(ifq));
	memset(&ifp, 0, sizeof(ifp));

	ifq.function = MPII_FUNCTION_IOC_FACTS;

	if (mpii_handshake_send(sc, &ifq, dwordsof(ifq)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_iocfacts send failed\n",
		    DEVNAME(sc));
		return (1);
	}

	if (mpii_handshake_recv(sc, &ifp, dwordsof(ifp)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_iocfacts recv failed\n",
		    DEVNAME(sc));
		return (1);
	}

	sc->sc_ioc_number = ifp.ioc_number;
	sc->sc_vf_id = ifp.vf_id;

	sc->sc_max_volumes = ifp.max_volumes;
	sc->sc_max_devices = ifp.max_volumes + le16toh(ifp.max_targets);

	if (ISSET(le32toh(ifp.ioc_capabilities),
	    MPII_IOCFACTS_CAPABILITY_INTEGRATED_RAID))
		SET(sc->sc_flags, MPII_F_RAID);
	if (ISSET(le32toh(ifp.ioc_capabilities),
	    MPII_IOCFACTS_CAPABILITY_EVENT_REPLAY))
		sc->sc_ioc_event_replay = 1;

	sc->sc_max_cmds = MIN(le16toh(ifp.request_credit),
	    MPII_REQUEST_CREDIT);

	/* SAS3 and 3.5 controllers have different sgl layouts */
	if (ifp.msg_version_maj == 2 && ((ifp.msg_version_min == 5)
	    || (ifp.msg_version_min == 6)))
		SET(sc->sc_flags, MPII_F_SAS3);

	/*
	 * The host driver must ensure that there is at least one
	 * unused entry in the Reply Free Queue. One way to ensure
	 * that this requirement is met is to never allocate a number
	 * of reply frames that is a multiple of 16.
	 */
	sc->sc_num_reply_frames = sc->sc_max_cmds + 32;
	if (!(sc->sc_num_reply_frames % 16))
		sc->sc_num_reply_frames--;

	/* must be multiple of 16 */
	sc->sc_reply_post_qdepth = sc->sc_max_cmds +
	    sc->sc_num_reply_frames;
	sc->sc_reply_post_qdepth += 16 - (sc->sc_reply_post_qdepth % 16);

	qdepth = le16toh(ifp.max_reply_descriptor_post_queue_depth);
	if (sc->sc_reply_post_qdepth > qdepth) {
		sc->sc_reply_post_qdepth = qdepth;
		if (sc->sc_reply_post_qdepth < 16) {
			printf("%s: RDPQ is too shallow\n", DEVNAME(sc));
			return (1);
		}
		sc->sc_max_cmds = sc->sc_reply_post_qdepth / 2 - 4;
		sc->sc_num_reply_frames = sc->sc_max_cmds + 4;
	}

	sc->sc_reply_free_qdepth = sc->sc_num_reply_frames +
	    16 - (sc->sc_num_reply_frames % 16);

	/*
	 * Our request frame for an I/O operation looks like this:
	 *
	 * +-------------------+ -.
	 * | mpii_msg_scsi_io  |  |
	 * +-------------------|  |
	 * | mpii_sge          |  |
	 * + - - - - - - - - - +  |
	 * | ...               |  > ioc_request_frame_size
	 * + - - - - - - - - - +  |
	 * | mpii_sge (tail)   |  |
	 * + - - - - - - - - - +  |
	 * | mpii_sge (csge)   |  | --.
	 * + - - - - - - - - - + -'   | chain sge points to the next sge
	 * | mpii_sge          |<-----'
	 * + - - - - - - - - - +
	 * | ...               |
	 * + - - - - - - - - - +
	 * | mpii_sge (tail)   |
	 * +-------------------+
	 * |                   |
	 * ~~~~~~~~~~~~~~~~~~~~~
	 * |                   |
	 * +-------------------+ <- sc_request_size - sizeof(scsi_sense_data)
	 * | scsi_sense_data   |
	 * +-------------------+
	 */

	/* both sizes are in 32-bit words */
	sc->sc_reply_size = ifp.reply_frame_size * 4;
	irs = le16toh(ifp.ioc_request_frame_size) * 4;
	sc->sc_request_size = MPII_REQUEST_SIZE;
	/* make sure we have enough space for scsi sense data */
	if (irs > sc->sc_request_size) {
		sc->sc_request_size = irs + sizeof(struct scsi_sense_data);
		sc->sc_request_size += 16 - (sc->sc_request_size % 16);
	}

	if (ISSET(sc->sc_flags, MPII_F_SAS3)) {
		sge_size = sizeof(struct mpii_ieee_sge);
	} else {
		sge_size = sizeof(struct mpii_sge);
	}

	/* offset to the chain sge */
	sc->sc_chain_sge = (irs - sizeof(struct mpii_msg_scsi_io)) /
	    sge_size - 1;

	/*
	 * A number of simple scatter-gather elements we can fit into the
	 * request buffer after the I/O command minus the chain element.
	 */
	sc->sc_max_sgl = (sc->sc_request_size -
 	    sizeof(struct mpii_msg_scsi_io) - sizeof(struct scsi_sense_data)) /
	    sge_size - 1;

	return (0);
}

int
mpii_iocinit(struct mpii_softc *sc)
{
	struct mpii_msg_iocinit_request		iiq;
	struct mpii_msg_iocinit_reply		iip;

	DNPRINTF(MPII_D_MISC, "%s: mpii_iocinit\n", DEVNAME(sc));

	memset(&iiq, 0, sizeof(iiq));
	memset(&iip, 0, sizeof(iip));

	iiq.function = MPII_FUNCTION_IOC_INIT;
	iiq.whoinit = MPII_WHOINIT_HOST_DRIVER;

	/* XXX JPG do something about vf_id */
	iiq.vf_id = 0;

	iiq.msg_version_maj = 0x02;
	iiq.msg_version_min = 0x00;

	/* XXX JPG ensure compliance with some level and hard-code? */
	iiq.hdr_version_unit = 0x00;
	iiq.hdr_version_dev = 0x00;

	iiq.system_request_frame_size = htole16(sc->sc_request_size / 4);

	iiq.reply_descriptor_post_queue_depth =
	    htole16(sc->sc_reply_post_qdepth);

	iiq.reply_free_queue_depth = htole16(sc->sc_reply_free_qdepth);

	iiq.sense_buffer_address_high =
	    htole32(MPII_DMA_DVA(sc->sc_requests) >> 32);

	iiq.system_reply_address_high =
	    htole32(MPII_DMA_DVA(sc->sc_replies) >> 32);

	iiq.system_request_frame_base_address_lo =
	    htole32(MPII_DMA_DVA(sc->sc_requests));
	iiq.system_request_frame_base_address_hi =
	    htole32(MPII_DMA_DVA(sc->sc_requests) >> 32);

	iiq.reply_descriptor_post_queue_address_lo =
	    htole32(MPII_DMA_DVA(sc->sc_reply_postq));
	iiq.reply_descriptor_post_queue_address_hi =
	    htole32(MPII_DMA_DVA(sc->sc_reply_postq) >> 32);

	iiq.reply_free_queue_address_lo =
	    htole32(MPII_DMA_DVA(sc->sc_reply_freeq));
	iiq.reply_free_queue_address_hi =
	    htole32(MPII_DMA_DVA(sc->sc_reply_freeq) >> 32);

	if (mpii_handshake_send(sc, &iiq, dwordsof(iiq)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_iocinit send failed\n",
		    DEVNAME(sc));
		return (1);
	}

	if (mpii_handshake_recv(sc, &iip, dwordsof(iip)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_iocinit recv failed\n",
		    DEVNAME(sc));
		return (1);
	}

	DNPRINTF(MPII_D_MISC, "%s:  function: 0x%02x msg_length: %d "
	    "whoinit: 0x%02x\n", DEVNAME(sc), iip.function,
	    iip.msg_length, iip.whoinit);
	DNPRINTF(MPII_D_MISC, "%s:  msg_flags: 0x%02x\n", DEVNAME(sc),
	    iip.msg_flags);
	DNPRINTF(MPII_D_MISC, "%s:  vf_id: 0x%02x vp_id: 0x%02x\n", DEVNAME(sc),
	    iip.vf_id, iip.vp_id);
	DNPRINTF(MPII_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    le16toh(iip.ioc_status));
	DNPRINTF(MPII_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    le32toh(iip.ioc_loginfo));

	if (le16toh(iip.ioc_status) != MPII_IOCSTATUS_SUCCESS ||
	    le32toh(iip.ioc_loginfo))
		return (1);

	return (0);
}

void
mpii_push_reply(struct mpii_softc *sc, struct mpii_rcb *rcb)
{
	u_int32_t		*rfp;
	u_int			idx;

	if (rcb == NULL)
		return;

	mutex_enter(&sc->sc_reply_free_mtx);
	idx = sc->sc_reply_free_host_index;

	rfp = MPII_DMA_KVA(sc->sc_reply_freeq);
	rfp[idx] = htole32(rcb->rcb_reply_dva);

	if (++idx >= sc->sc_reply_free_qdepth)
		idx = 0;

	mpii_write_reply_free(sc, sc->sc_reply_free_host_index = idx);
	mutex_exit(&sc->sc_reply_free_mtx);
}

int
mpii_portfacts(struct mpii_softc *sc)
{
	struct mpii_msg_portfacts_request	*pfq;
	struct mpii_msg_portfacts_reply		*pfp;
	struct mpii_ccb				*ccb;
	int					rv = 1;

	DNPRINTF(MPII_D_MISC, "%s: mpii_portfacts\n", DEVNAME(sc));

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_portfacts mpii_get_ccb fail\n",
		    DEVNAME(sc));
		return (rv);
	}

	ccb->ccb_done = mpii_empty_done;
	pfq = ccb->ccb_cmd;

	memset(pfq, 0, sizeof(*pfq));

	pfq->function = MPII_FUNCTION_PORT_FACTS;
	pfq->chain_offset = 0;
	pfq->msg_flags = 0;
	pfq->port_number = 0;
	pfq->vp_id = 0;
	pfq->vf_id = 0;

	if (mpii_poll(sc, ccb) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_portfacts poll\n",
		    DEVNAME(sc));
		goto err;
	}

	if (ccb->ccb_rcb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: empty portfacts reply\n",
		    DEVNAME(sc));
		goto err;
	}

	pfp = ccb->ccb_rcb->rcb_reply;
	sc->sc_porttype = pfp->port_type;

	mpii_push_reply(sc, ccb->ccb_rcb);
	rv = 0;
err:
	mpii_put_ccb(sc, ccb);

	return (rv);
}

void
mpii_eventack(struct work *wk, void * cookie)
{
	struct mpii_softc			*sc = cookie;
	struct mpii_ccb				*ccb;
	struct mpii_rcb				*rcb, *next;
	struct mpii_msg_event_reply		*enp;
	struct mpii_msg_eventack_request	*eaq;

	mutex_enter(&sc->sc_evt_ack_mtx);
	next = SIMPLEQ_FIRST(&sc->sc_evt_ack_queue);
	SIMPLEQ_INIT(&sc->sc_evt_ack_queue);
	mutex_exit(&sc->sc_evt_ack_mtx);

	while (next != NULL) {
		rcb = next;
		next = SIMPLEQ_NEXT(rcb, rcb_link);

		enp = (struct mpii_msg_event_reply *)rcb->rcb_reply;

		ccb = mpii_get_ccb(sc);
		ccb->ccb_done = mpii_eventack_done;
		eaq = ccb->ccb_cmd;

		eaq->function = MPII_FUNCTION_EVENT_ACK;

		eaq->event = enp->event;
		eaq->event_context = enp->event_context;

		mpii_push_reply(sc, rcb);

		mpii_start(sc, ccb);
	}
}

void
mpii_eventack_done(struct mpii_ccb *ccb)
{
	struct mpii_softc			*sc = ccb->ccb_sc;

	DNPRINTF(MPII_D_EVT, "%s: event ack done\n", DEVNAME(sc));

	mpii_push_reply(sc, ccb->ccb_rcb);
	mpii_put_ccb(sc, ccb);
}

int
mpii_portenable(struct mpii_softc *sc)
{
	struct mpii_msg_portenable_request	*peq;
	struct mpii_ccb				*ccb;

	DNPRINTF(MPII_D_MISC, "%s: mpii_portenable\n", DEVNAME(sc));

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_portenable ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	ccb->ccb_done = mpii_empty_done;
	peq = ccb->ccb_cmd;

	peq->function = MPII_FUNCTION_PORT_ENABLE;
	peq->vf_id = sc->sc_vf_id;

	if (mpii_poll(sc, ccb) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_portenable poll\n",
		    DEVNAME(sc));
		return (1);
	}

	if (ccb->ccb_rcb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: empty portenable reply\n",
		    DEVNAME(sc));
		return (1);
	}

	mpii_push_reply(sc, ccb->ccb_rcb);
	mpii_put_ccb(sc, ccb);

	return (0);
}

int
mpii_cfg_coalescing(struct mpii_softc *sc)
{
	struct mpii_cfg_hdr			hdr;
	struct mpii_cfg_ioc_pg1			ipg;

	hdr.page_version = 0;
	hdr.page_length = sizeof(ipg) / 4;
	hdr.page_number = 1;
	hdr.page_type = MPII_CONFIG_REQ_PAGE_TYPE_IOC;
	memset(&ipg, 0, sizeof(ipg));
	if (mpii_req_cfg_page(sc, 0, MPII_PG_POLL, &hdr, 1, &ipg,
	    sizeof(ipg)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: unable to fetch IOC page 1\n"
		    "page 1\n", DEVNAME(sc));
		return (1);
	}

	if (!ISSET(le32toh(ipg.flags), MPII_CFG_IOC_1_REPLY_COALESCING))
		return (0);

	/* Disable coalescing */
	CLR(ipg.flags, htole32(MPII_CFG_IOC_1_REPLY_COALESCING));
	if (mpii_req_cfg_page(sc, 0, MPII_PG_POLL, &hdr, 0, &ipg,
	    sizeof(ipg)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: unable to clear coalescing\n",
		    DEVNAME(sc));
		return (1);
	}

	return (0);
}

#define MPII_EVENT_MASKALL(enq)		do {			\
		enq->event_masks[0] = 0xffffffff;		\
		enq->event_masks[1] = 0xffffffff;		\
		enq->event_masks[2] = 0xffffffff;		\
		enq->event_masks[3] = 0xffffffff;		\
	} while (0)

#define MPII_EVENT_UNMASK(enq, evt)	do {			\
		enq->event_masks[evt / 32] &=			\
		    htole32(~(1 << (evt % 32)));		\
	} while (0)

int
mpii_eventnotify(struct mpii_softc *sc)
{
	struct mpii_msg_event_request		*enq;
	struct mpii_ccb				*ccb;
	char wkname[15];

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_eventnotify ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	SIMPLEQ_INIT(&sc->sc_evt_sas_queue);
	mutex_init(&sc->sc_evt_sas_mtx, MUTEX_DEFAULT, IPL_BIO);
	snprintf(wkname, sizeof(wkname), "%ssas", DEVNAME(sc));
	if (workqueue_create(&sc->sc_evt_sas_wq, wkname,
	    mpii_event_sas_work, sc, PRI_NONE, IPL_BIO, WQ_MPSAFE) != 0) {
		mpii_put_ccb(sc, ccb);
		aprint_error_dev(sc->sc_dev,
		    "can't create %s workqueue\n", wkname);
		return 1;
	}

	SIMPLEQ_INIT(&sc->sc_evt_ack_queue);
	mutex_init(&sc->sc_evt_ack_mtx, MUTEX_DEFAULT, IPL_BIO);
	snprintf(wkname, sizeof(wkname), "%sevt", DEVNAME(sc));
	if (workqueue_create(&sc->sc_evt_ack_wq, wkname,
	    mpii_eventack, sc, PRI_NONE, IPL_BIO, WQ_MPSAFE) != 0) {
		mpii_put_ccb(sc, ccb);
		aprint_error_dev(sc->sc_dev,
		    "can't create %s workqueue\n", wkname);
		return 1;
	}

	ccb->ccb_done = mpii_eventnotify_done;
	enq = ccb->ccb_cmd;

	enq->function = MPII_FUNCTION_EVENT_NOTIFICATION;

	/*
	 * Enable reporting of the following events:
	 *
	 * MPII_EVENT_SAS_DISCOVERY
	 * MPII_EVENT_SAS_TOPOLOGY_CHANGE_LIST
	 * MPII_EVENT_SAS_DEVICE_STATUS_CHANGE
	 * MPII_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE
	 * MPII_EVENT_IR_CONFIGURATION_CHANGE_LIST
	 * MPII_EVENT_IR_VOLUME
	 * MPII_EVENT_IR_PHYSICAL_DISK
	 * MPII_EVENT_IR_OPERATION_STATUS
	 */

	MPII_EVENT_MASKALL(enq);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_SAS_DISCOVERY);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_SAS_TOPOLOGY_CHANGE_LIST);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_SAS_DEVICE_STATUS_CHANGE);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_IR_CONFIGURATION_CHANGE_LIST);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_IR_VOLUME);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_IR_PHYSICAL_DISK);
	MPII_EVENT_UNMASK(enq, MPII_EVENT_IR_OPERATION_STATUS);

	mpii_start(sc, ccb);

	return (0);
}

void
mpii_eventnotify_done(struct mpii_ccb *ccb)
{
	struct mpii_softc			*sc = ccb->ccb_sc;
	struct mpii_rcb				*rcb = ccb->ccb_rcb;

	DNPRINTF(MPII_D_EVT, "%s: mpii_eventnotify_done\n", DEVNAME(sc));

	mpii_put_ccb(sc, ccb);
	mpii_event_process(sc, rcb);
}

void
mpii_event_raid(struct mpii_softc *sc, struct mpii_msg_event_reply *enp)
{
	struct mpii_evt_ir_cfg_change_list	*ccl;
	struct mpii_evt_ir_cfg_element		*ce;
	struct mpii_device			*dev;
	u_int16_t				type;
	int					i;

	ccl = (struct mpii_evt_ir_cfg_change_list *)(enp + 1);
	if (ccl->num_elements == 0)
		return;

	if (ISSET(le32toh(ccl->flags), MPII_EVT_IR_CFG_CHANGE_LIST_FOREIGN)) {
		/* bail on foreign configurations */
		return;
	}

	ce = (struct mpii_evt_ir_cfg_element *)(ccl + 1);

	for (i = 0; i < ccl->num_elements; i++, ce++) {
		type = (le16toh(ce->element_flags) &
		    MPII_EVT_IR_CFG_ELEMENT_TYPE_MASK);

		switch (type) {
		case MPII_EVT_IR_CFG_ELEMENT_TYPE_VOLUME:
			switch (ce->reason_code) {
			case MPII_EVT_IR_CFG_ELEMENT_RC_ADDED:
			case MPII_EVT_IR_CFG_ELEMENT_RC_VOLUME_CREATED:
				dev = malloc(sizeof(*dev), M_DEVBUF,
				    M_NOWAIT | M_ZERO);
				if (!dev) {
					printf("%s: failed to allocate a "
					    "device structure\n", DEVNAME(sc));
					break;
				}
				mutex_enter(&sc->sc_devs_mtx);
				if (mpii_find_dev(sc,
				    le16toh(ce->vol_dev_handle))) {
					mutex_exit(&sc->sc_devs_mtx);
					free(dev, M_DEVBUF);
					printf("%s: device %#x is already "
					    "configured\n", DEVNAME(sc),
					    le16toh(ce->vol_dev_handle));
					break;
				}
				SET(dev->flags, MPII_DF_VOLUME);
				dev->slot = sc->sc_vd_id_low;
				dev->dev_handle = le16toh(ce->vol_dev_handle);
				if (mpii_insert_dev(sc, dev)) {
					mutex_exit(&sc->sc_devs_mtx);
					free(dev, M_DEVBUF);
					break;
				}
				sc->sc_vd_count++;
				mutex_exit(&sc->sc_devs_mtx);
				break;
			case MPII_EVT_IR_CFG_ELEMENT_RC_REMOVED:
			case MPII_EVT_IR_CFG_ELEMENT_RC_VOLUME_DELETED:
				mutex_enter(&sc->sc_devs_mtx);
				if (!(dev = mpii_find_dev(sc,
				    le16toh(ce->vol_dev_handle)))) {
					mutex_exit(&sc->sc_devs_mtx);
					break;
				}
				mpii_remove_dev(sc, dev);
				sc->sc_vd_count--;
				mutex_exit(&sc->sc_devs_mtx);
				break;
			}
			break;
		case MPII_EVT_IR_CFG_ELEMENT_TYPE_VOLUME_DISK:
			if (ce->reason_code ==
			    MPII_EVT_IR_CFG_ELEMENT_RC_PD_CREATED ||
			    ce->reason_code ==
			    MPII_EVT_IR_CFG_ELEMENT_RC_HIDE) {
				/* there should be an underlying sas drive */
				mutex_enter(&sc->sc_devs_mtx);
				if (!(dev = mpii_find_dev(sc,
				    le16toh(ce->phys_disk_dev_handle)))) {
					mutex_exit(&sc->sc_devs_mtx);
					break;
				}
				/* promoted from a hot spare? */
				CLR(dev->flags, MPII_DF_HOT_SPARE);
				SET(dev->flags, MPII_DF_VOLUME_DISK |
				    MPII_DF_HIDDEN);
				mutex_exit(&sc->sc_devs_mtx);
			}
			break;
		case MPII_EVT_IR_CFG_ELEMENT_TYPE_HOT_SPARE:
			if (ce->reason_code ==
			    MPII_EVT_IR_CFG_ELEMENT_RC_HIDE) {
				/* there should be an underlying sas drive */
				mutex_enter(&sc->sc_devs_mtx);
				if (!(dev = mpii_find_dev(sc,
				    le16toh(ce->phys_disk_dev_handle)))) {
					mutex_exit(&sc->sc_devs_mtx);
					break;
				}
				SET(dev->flags, MPII_DF_HOT_SPARE |
				    MPII_DF_HIDDEN);
				mutex_exit(&sc->sc_devs_mtx);
			}
			break;
		}
	}
}

void
mpii_event_sas(struct mpii_softc *sc, struct mpii_rcb *rcb)
{
	struct mpii_msg_event_reply 	*enp;
	struct mpii_evt_sas_tcl		*tcl;
	struct mpii_evt_phy_entry	*pe;
	struct mpii_device		*dev;
	int				i;
	u_int16_t			handle;
	int				need_queue = 0;

	enp = (struct mpii_msg_event_reply *)rcb->rcb_reply;
	DNPRINTF(MPII_D_EVT, "%s: mpii_event_sas 0x%x\n",
		    DEVNAME(sc), le16toh(enp->event));
	KASSERT(le16toh(enp->event) == MPII_EVENT_SAS_TOPOLOGY_CHANGE_LIST);

	tcl = (struct mpii_evt_sas_tcl *)(enp + 1);
	pe = (struct mpii_evt_phy_entry *)(tcl + 1);

	for (i = 0; i < tcl->num_entries; i++, pe++) {
		DNPRINTF(MPII_D_EVT, "%s: sas change %d stat %d h %d slot %d phy %d enc %d expand %d\n",
		    DEVNAME(sc), i, pe->phy_status,
		    le16toh(pe->dev_handle),
		    sc->sc_pd_id_start + tcl->start_phy_num + i,
		    tcl->start_phy_num + i, le16toh(tcl->enclosure_handle), le16toh(tcl->expander_handle));
			
		switch (pe->phy_status & MPII_EVENT_SAS_TOPO_PS_RC_MASK) {
		case MPII_EVENT_SAS_TOPO_PS_RC_ADDED:
			handle = le16toh(pe->dev_handle);
			DNPRINTF(MPII_D_EVT, "%s: sas add handle %d\n",
			    DEVNAME(sc), handle);
			dev = malloc(sizeof(*dev), M_DEVBUF, M_WAITOK | M_ZERO);
			mutex_enter(&sc->sc_devs_mtx);
			if (mpii_find_dev(sc, handle)) {
				mutex_exit(&sc->sc_devs_mtx);
				free(dev, M_DEVBUF);
				printf("%s: device %#x is already "
				    "configured\n", DEVNAME(sc), handle);
				break;
			}

			dev->slot = sc->sc_pd_id_start + tcl->start_phy_num + i;
			dev->dev_handle = handle;
			dev->phy_num = tcl->start_phy_num + i;
			if (tcl->enclosure_handle)
				dev->physical_port = tcl->physical_port;
			dev->enclosure = le16toh(tcl->enclosure_handle);
			dev->expander = le16toh(tcl->expander_handle);

			if (mpii_insert_dev(sc, dev)) {
				mutex_exit(&sc->sc_devs_mtx);
				free(dev, M_DEVBUF);
				break;
			}
			printf("%s: physical disk inserted in slot %d\n",
			    DEVNAME(sc), dev->slot);
			mutex_exit(&sc->sc_devs_mtx);
			break;

		case MPII_EVENT_SAS_TOPO_PS_RC_MISSING:
			/* defer to workqueue thread */
			need_queue++;
			break;
		}
	}

	if (need_queue) {
		bool start_wk;
		mutex_enter(&sc->sc_evt_sas_mtx);
		start_wk = (SIMPLEQ_FIRST(&sc->sc_evt_sas_queue) == 0);
		SIMPLEQ_INSERT_TAIL(&sc->sc_evt_sas_queue, rcb, rcb_link);
		if (start_wk) {
			workqueue_enqueue(sc->sc_evt_sas_wq,
			    &sc->sc_evt_sas_work, NULL);
		}
		mutex_exit(&sc->sc_evt_sas_mtx);
	} else
		mpii_event_done(sc, rcb);
}

void
mpii_event_sas_work(struct work *wq, void *xsc)
{
	struct mpii_softc *sc = xsc;
	struct mpii_rcb *rcb, *next;
	struct mpii_msg_event_reply *enp;
	struct mpii_evt_sas_tcl		*tcl;
	struct mpii_evt_phy_entry	*pe;
	struct mpii_device		*dev;
	int				i;

	mutex_enter(&sc->sc_evt_sas_mtx);
	next = SIMPLEQ_FIRST(&sc->sc_evt_sas_queue);
	SIMPLEQ_INIT(&sc->sc_evt_sas_queue);
	mutex_exit(&sc->sc_evt_sas_mtx);

	while (next != NULL) {
		rcb = next;
		next = SIMPLEQ_NEXT(rcb, rcb_link);

		enp = (struct mpii_msg_event_reply *)rcb->rcb_reply;
		DNPRINTF(MPII_D_EVT, "%s: mpii_event_sas_work 0x%x\n",
			    DEVNAME(sc), le16toh(enp->event));
		KASSERT(le16toh(enp->event) == MPII_EVENT_SAS_TOPOLOGY_CHANGE_LIST);
		tcl = (struct mpii_evt_sas_tcl *)(enp + 1);
		pe = (struct mpii_evt_phy_entry *)(tcl + 1);

		for (i = 0; i < tcl->num_entries; i++, pe++) {
			DNPRINTF(MPII_D_EVT, "%s: sas change %d stat %d h %d slot %d phy %d enc %d expand %d\n",
			    DEVNAME(sc), i, pe->phy_status,
			    le16toh(pe->dev_handle),
			    sc->sc_pd_id_start + tcl->start_phy_num + i,
			    tcl->start_phy_num + i, le16toh(tcl->enclosure_handle), le16toh(tcl->expander_handle));
			
			switch (pe->phy_status & MPII_EVENT_SAS_TOPO_PS_RC_MASK) {
			case MPII_EVENT_SAS_TOPO_PS_RC_ADDED:
				/* already handled */
				break;

			case MPII_EVENT_SAS_TOPO_PS_RC_MISSING:
				mutex_enter(&sc->sc_devs_mtx);
				dev = mpii_find_dev(sc, le16toh(pe->dev_handle));
				if (dev == NULL) {
					mutex_exit(&sc->sc_devs_mtx);
					break;
				}

				printf(
				    "%s: physical disk removed from slot %d\n",
				    DEVNAME(sc), dev->slot);
				mpii_remove_dev(sc, dev);
				mutex_exit(&sc->sc_devs_mtx);
				mpii_sas_remove_device(sc, dev->dev_handle);
				if (!ISSET(dev->flags, MPII_DF_HIDDEN)) {
					scsipi_target_detach(&sc->sc_chan,
					    dev->slot, 0, DETACH_FORCE);
				}

				free(dev, M_DEVBUF);
				break;
			}
		}
		mpii_event_done(sc, rcb);
	}
}

void
mpii_event_discovery(struct mpii_softc *sc, struct mpii_msg_event_reply *enp)
{
	struct mpii_evt_sas_discovery *esd =
	    (struct mpii_evt_sas_discovery *)(enp + 1);

	if (esd->reason_code == MPII_EVENT_SAS_DISC_REASON_CODE_COMPLETED) {
		if (esd->discovery_status != 0) {
			printf("%s: sas discovery completed with status %#x\n",
			    DEVNAME(sc), esd->discovery_status);
		}

	}
}

void
mpii_event_process(struct mpii_softc *sc, struct mpii_rcb *rcb)
{
	struct mpii_msg_event_reply		*enp;

	enp = (struct mpii_msg_event_reply *)rcb->rcb_reply;

	DNPRINTF(MPII_D_EVT, "%s: mpii_event_process: %#x\n", DEVNAME(sc),
	    le16toh(enp->event));

	switch (le16toh(enp->event)) {
	case MPII_EVENT_EVENT_CHANGE:
		/* should be properly ignored */
		break;
	case MPII_EVENT_SAS_DISCOVERY:
		mpii_event_discovery(sc, enp);
		break;
	case MPII_EVENT_SAS_TOPOLOGY_CHANGE_LIST:
		mpii_event_sas(sc, rcb);
		return;
	case MPII_EVENT_SAS_DEVICE_STATUS_CHANGE:
		break;
	case MPII_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE:
		break;
	case MPII_EVENT_IR_VOLUME: {
		struct mpii_evt_ir_volume	*evd =
		    (struct mpii_evt_ir_volume *)(enp + 1);
		struct mpii_device		*dev;
#if NBIO > 0
		const char *vol_states[] = {
			BIOC_SVINVALID_S,
			BIOC_SVOFFLINE_S,
			BIOC_SVBUILDING_S,
			BIOC_SVONLINE_S,
			BIOC_SVDEGRADED_S,
			BIOC_SVONLINE_S,
		};
#endif

		if (cold)
			break;
		mutex_enter(&sc->sc_devs_mtx);
		dev = mpii_find_dev(sc, le16toh(evd->vol_dev_handle));
		if (dev == NULL) {
			mutex_exit(&sc->sc_devs_mtx);
			break;
		}
#if NBIO > 0
		if (evd->reason_code == MPII_EVENT_IR_VOL_RC_STATE_CHANGED)
			printf("%s: volume %d state changed from %s to %s\n",
			    DEVNAME(sc), dev->slot - sc->sc_vd_id_low,
			    vol_states[evd->prev_value],
			    vol_states[evd->new_value]);
#endif
		if (evd->reason_code == MPII_EVENT_IR_VOL_RC_STATUS_CHANGED &&
		    ISSET(evd->new_value, MPII_CFG_RAID_VOL_0_STATUS_RESYNC) &&
		    !ISSET(evd->prev_value, MPII_CFG_RAID_VOL_0_STATUS_RESYNC))
			printf("%s: started resync on a volume %d\n",
			    DEVNAME(sc), dev->slot - sc->sc_vd_id_low);
		}
		mutex_exit(&sc->sc_devs_mtx);
		break;
	case MPII_EVENT_IR_PHYSICAL_DISK:
		break;
	case MPII_EVENT_IR_CONFIGURATION_CHANGE_LIST:
		mpii_event_raid(sc, enp);
		break;
	case MPII_EVENT_IR_OPERATION_STATUS: {
		struct mpii_evt_ir_status	*evs =
		    (struct mpii_evt_ir_status *)(enp + 1);
		struct mpii_device		*dev;

		mutex_enter(&sc->sc_devs_mtx);
		dev = mpii_find_dev(sc, le16toh(evs->vol_dev_handle));
		if (dev != NULL &&
		    evs->operation == MPII_EVENT_IR_RAIDOP_RESYNC)
			dev->percent = evs->percent;
		mutex_exit(&sc->sc_devs_mtx);
		break;
		}
	default:
		DNPRINTF(MPII_D_EVT, "%s:  unhandled event 0x%02x\n",
		    DEVNAME(sc), le16toh(enp->event));
	}

	mpii_event_done(sc, rcb);
}

void
mpii_event_done(struct mpii_softc *sc, struct mpii_rcb *rcb)
{
	struct mpii_msg_event_reply *enp = rcb->rcb_reply;
	bool	need_start;

	if (enp->ack_required) {
		mutex_enter(&sc->sc_evt_ack_mtx);
		need_start = (SIMPLEQ_FIRST(&sc->sc_evt_ack_queue) == 0);
		SIMPLEQ_INSERT_TAIL(&sc->sc_evt_ack_queue, rcb, rcb_link);
		if (need_start)
			workqueue_enqueue(sc->sc_evt_ack_wq,
			    &sc->sc_evt_ack_work, NULL);
		mutex_exit(&sc->sc_evt_ack_mtx);
	} else
		mpii_push_reply(sc, rcb);
}

void
mpii_sas_remove_device(struct mpii_softc *sc, u_int16_t handle)
{
	struct mpii_msg_scsi_task_request	*stq;
	struct mpii_msg_sas_oper_request	*soq;
	struct mpii_ccb				*ccb;

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL)
		return;

	stq = ccb->ccb_cmd;
	stq->function = MPII_FUNCTION_SCSI_TASK_MGMT;
	stq->task_type = MPII_SCSI_TASK_TARGET_RESET;
	stq->dev_handle = htole16(handle);

	ccb->ccb_done = mpii_empty_done;
	mpii_wait(sc, ccb);

	if (ccb->ccb_rcb != NULL)
		mpii_push_reply(sc, ccb->ccb_rcb);

	/* reuse a ccb */
	ccb->ccb_state = MPII_CCB_READY;
	ccb->ccb_rcb = NULL;

	soq = ccb->ccb_cmd;
	memset(soq, 0, sizeof(*soq));
	soq->function = MPII_FUNCTION_SAS_IO_UNIT_CONTROL;
	soq->operation = MPII_SAS_OP_REMOVE_DEVICE;
	soq->dev_handle = htole16(handle);

	ccb->ccb_done = mpii_empty_done;
	mpii_wait(sc, ccb);
	if (ccb->ccb_rcb != NULL)
		mpii_push_reply(sc, ccb->ccb_rcb);

	mpii_put_ccb(sc, ccb);
}

int
mpii_board_info(struct mpii_softc *sc)
{
	struct mpii_msg_iocfacts_request	ifq;
	struct mpii_msg_iocfacts_reply		ifp;
	struct mpii_cfg_manufacturing_pg0	mpg;
	struct mpii_cfg_hdr			hdr;

	memset(&ifq, 0, sizeof(ifq));
	memset(&ifp, 0, sizeof(ifp));

	ifq.function = MPII_FUNCTION_IOC_FACTS;

	if (mpii_handshake_send(sc, &ifq, dwordsof(ifq)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: failed to request ioc facts\n",
		    DEVNAME(sc));
		return (1);
	}

	if (mpii_handshake_recv(sc, &ifp, dwordsof(ifp)) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: failed to receive ioc facts\n",
		    DEVNAME(sc));
		return (1);
	}

	hdr.page_version = 0;
	hdr.page_length = sizeof(mpg) / 4;
	hdr.page_number = 0;
	hdr.page_type = MPII_CONFIG_REQ_PAGE_TYPE_MANUFACTURING;
	memset(&mpg, 0, sizeof(mpg));
	if (mpii_req_cfg_page(sc, 0, MPII_PG_POLL, &hdr, 1, &mpg,
	    sizeof(mpg)) != 0) {
		printf("%s: unable to fetch manufacturing page 0\n",
		    DEVNAME(sc));
		return (EINVAL);
	}

	printf("%s: %s, firmware %u.%u.%u.%u%s, MPI %u.%u\n", DEVNAME(sc),
	    mpg.board_name, ifp.fw_version_maj, ifp.fw_version_min,
	    ifp.fw_version_unit, ifp.fw_version_dev,
	    ISSET(sc->sc_flags, MPII_F_RAID) ? " IR" : "",
	    ifp.msg_version_maj, ifp.msg_version_min);

	return (0);
}

int
mpii_target_map(struct mpii_softc *sc)
{
	struct mpii_cfg_hdr			hdr;
	struct mpii_cfg_ioc_pg8			ipg;
	int					flags, pad = 0;

	hdr.page_version = 0;
	hdr.page_length = sizeof(ipg) / 4;
	hdr.page_number = 8;
	hdr.page_type = MPII_CONFIG_REQ_PAGE_TYPE_IOC;
	memset(&ipg, 0, sizeof(ipg));
	if (mpii_req_cfg_page(sc, 0, MPII_PG_POLL, &hdr, 1, &ipg,
	    sizeof(ipg)) != 0) {
		printf("%s: unable to fetch ioc page 8\n",
		    DEVNAME(sc));
		return (EINVAL);
	}

	if (le16toh(ipg.flags) & MPII_IOC_PG8_FLAGS_RESERVED_TARGETID_0)
		pad = 1;

	flags = le16toh(ipg.ir_volume_mapping_flags) &
	    MPII_IOC_PG8_IRFLAGS_VOLUME_MAPPING_MODE_MASK;
	if (ISSET(sc->sc_flags, MPII_F_RAID)) {
		if (flags == MPII_IOC_PG8_IRFLAGS_LOW_VOLUME_MAPPING) {
			sc->sc_vd_id_low += pad;
			pad = sc->sc_max_volumes; /* for sc_pd_id_start */
		} else
			sc->sc_vd_id_low = sc->sc_max_devices -
			    sc->sc_max_volumes;
	}

	sc->sc_pd_id_start += pad;

	return (0);
}

int
mpii_req_cfg_header(struct mpii_softc *sc, u_int8_t type, u_int8_t number,
    u_int32_t address, int flags, void *p)
{
	struct mpii_msg_config_request		*cq;
	struct mpii_msg_config_reply		*cp;
	struct mpii_ccb				*ccb;
	struct mpii_cfg_hdr			*hdr = p;
	struct mpii_ecfg_hdr			*ehdr = p;
	int					etype = 0;
	int					rv = 0;

	DNPRINTF(MPII_D_MISC, "%s: mpii_req_cfg_header type: %#x number: %x "
	    "address: 0x%08x flags: 0x%x\n", DEVNAME(sc), type, number,
	    address, flags);

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_cfg_header ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	if (ISSET(flags, MPII_PG_EXTENDED)) {
		etype = type;
		type = MPII_CONFIG_REQ_PAGE_TYPE_EXTENDED;
	}

	cq = ccb->ccb_cmd;

	cq->function = MPII_FUNCTION_CONFIG;

	cq->action = MPII_CONFIG_REQ_ACTION_PAGE_HEADER;

	cq->config_header.page_number = number;
	cq->config_header.page_type = type;
	cq->ext_page_type = etype;
	cq->page_address = htole32(address);
	cq->page_buffer.sg_hdr = htole32(MPII_SGE_FL_TYPE_SIMPLE |
	    MPII_SGE_FL_LAST | MPII_SGE_FL_EOB | MPII_SGE_FL_EOL);

	ccb->ccb_done = mpii_empty_done;
	if (ISSET(flags, MPII_PG_POLL)) {
		if (mpii_poll(sc, ccb) != 0) {
			DNPRINTF(MPII_D_MISC, "%s: mpii_cfg_header poll\n",
			    DEVNAME(sc));
			return (1);
		}
	} else
		mpii_wait(sc, ccb);

	if (ccb->ccb_rcb == NULL) {
		mpii_put_ccb(sc, ccb);
		return (1);
	}
	cp = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPII_D_MISC, "%s:  action: 0x%02x sgl_flags: 0x%02x "
	    "msg_length: %d function: 0x%02x\n", DEVNAME(sc), cp->action,
	    cp->sgl_flags, cp->msg_length, cp->function);
	DNPRINTF(MPII_D_MISC, "%s:  ext_page_length: %d ext_page_type: 0x%02x "
	    "msg_flags: 0x%02x\n", DEVNAME(sc),
	    le16toh(cp->ext_page_length), cp->ext_page_type,
	    cp->msg_flags);
	DNPRINTF(MPII_D_MISC, "%s:  vp_id: 0x%02x vf_id: 0x%02x\n", DEVNAME(sc),
	    cp->vp_id, cp->vf_id);
	DNPRINTF(MPII_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    le16toh(cp->ioc_status));
	DNPRINTF(MPII_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    le32toh(cp->ioc_loginfo));
	DNPRINTF(MPII_D_MISC, "%s:  page_version: 0x%02x page_length: %d "
	    "page_number: 0x%02x page_type: 0x%02x\n", DEVNAME(sc),
	    cp->config_header.page_version,
	    cp->config_header.page_length,
	    cp->config_header.page_number,
	    cp->config_header.page_type);

	if (le16toh(cp->ioc_status) != MPII_IOCSTATUS_SUCCESS)
		rv = 1;
	else if (ISSET(flags, MPII_PG_EXTENDED)) {
		memset(ehdr, 0, sizeof(*ehdr));
		ehdr->page_version = cp->config_header.page_version;
		ehdr->page_number = cp->config_header.page_number;
		ehdr->page_type = cp->config_header.page_type;
		ehdr->ext_page_length = cp->ext_page_length;
		ehdr->ext_page_type = cp->ext_page_type;
	} else
		*hdr = cp->config_header;

	mpii_push_reply(sc, ccb->ccb_rcb);
	mpii_put_ccb(sc, ccb);

	return (rv);
}

int
mpii_req_cfg_page(struct mpii_softc *sc, u_int32_t address, int flags,
    void *p, int read, void *page, size_t len)
{
	struct mpii_msg_config_request		*cq;
	struct mpii_msg_config_reply		*cp;
	struct mpii_ccb				*ccb;
	struct mpii_cfg_hdr			*hdr = p;
	struct mpii_ecfg_hdr			*ehdr = p;
	uintptr_t				kva;
	int					page_length;
	int					rv = 0;

	DNPRINTF(MPII_D_MISC, "%s: mpii_cfg_page address: %d read: %d "
	    "type: %x\n", DEVNAME(sc), address, read, hdr->page_type);

	page_length = ISSET(flags, MPII_PG_EXTENDED) ?
	    le16toh(ehdr->ext_page_length) : hdr->page_length;

	if (len > sc->sc_request_size - sizeof(*cq) || len < page_length * 4)
		return (1);

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: mpii_cfg_page ccb_get\n",
		    DEVNAME(sc));
		return (1);
	}

	cq = ccb->ccb_cmd;

	cq->function = MPII_FUNCTION_CONFIG;

	cq->action = (read ? MPII_CONFIG_REQ_ACTION_PAGE_READ_CURRENT :
	    MPII_CONFIG_REQ_ACTION_PAGE_WRITE_CURRENT);

	if (ISSET(flags, MPII_PG_EXTENDED)) {
		cq->config_header.page_version = ehdr->page_version;
		cq->config_header.page_number = ehdr->page_number;
		cq->config_header.page_type = ehdr->page_type;
		cq->ext_page_len = ehdr->ext_page_length;
		cq->ext_page_type = ehdr->ext_page_type;
	} else
		cq->config_header = *hdr;
	cq->config_header.page_type &= MPII_CONFIG_REQ_PAGE_TYPE_MASK;
	cq->page_address = htole32(address);
	cq->page_buffer.sg_hdr = htole32(MPII_SGE_FL_TYPE_SIMPLE |
	    MPII_SGE_FL_LAST | MPII_SGE_FL_EOB | MPII_SGE_FL_EOL |
	    MPII_SGE_FL_SIZE_64 | (page_length * 4) |
	    (read ? MPII_SGE_FL_DIR_IN : MPII_SGE_FL_DIR_OUT));

	/* bounce the page via the request space to avoid more bus_dma games */
	mpii_dvatosge(&cq->page_buffer, ccb->ccb_cmd_dva +
	    sizeof(struct mpii_msg_config_request));

	kva = (uintptr_t)ccb->ccb_cmd;
	kva += sizeof(struct mpii_msg_config_request);

	if (!read)
		memcpy((void *)kva, page, len);

	ccb->ccb_done = mpii_empty_done;
	if (ISSET(flags, MPII_PG_POLL)) {
		if (mpii_poll(sc, ccb) != 0) {
			DNPRINTF(MPII_D_MISC, "%s: mpii_cfg_header poll\n",
			    DEVNAME(sc));
			return (1);
		}
	} else
		mpii_wait(sc, ccb);

	if (ccb->ccb_rcb == NULL) {
		mpii_put_ccb(sc, ccb);
		return (1);
	}
	cp = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPII_D_MISC, "%s:  action: 0x%02x msg_length: %d "
	    "function: 0x%02x\n", DEVNAME(sc), cp->action, cp->msg_length,
	    cp->function);
	DNPRINTF(MPII_D_MISC, "%s:  ext_page_length: %d ext_page_type: 0x%02x "
	    "msg_flags: 0x%02x\n", DEVNAME(sc),
	    le16toh(cp->ext_page_length), cp->ext_page_type,
	    cp->msg_flags);
	DNPRINTF(MPII_D_MISC, "%s:  vp_id: 0x%02x vf_id: 0x%02x\n", DEVNAME(sc),
	    cp->vp_id, cp->vf_id);
	DNPRINTF(MPII_D_MISC, "%s:  ioc_status: 0x%04x\n", DEVNAME(sc),
	    le16toh(cp->ioc_status));
	DNPRINTF(MPII_D_MISC, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    le32toh(cp->ioc_loginfo));
	DNPRINTF(MPII_D_MISC, "%s:  page_version: 0x%02x page_length: %d "
	    "page_number: 0x%02x page_type: 0x%02x\n", DEVNAME(sc),
	    cp->config_header.page_version,
	    cp->config_header.page_length,
	    cp->config_header.page_number,
	    cp->config_header.page_type);

	if (le16toh(cp->ioc_status) != MPII_IOCSTATUS_SUCCESS)
		rv = 1;
	else if (read)
		memcpy(page, (void *)kva, len);

	mpii_push_reply(sc, ccb->ccb_rcb);
	mpii_put_ccb(sc, ccb);

	return (rv);
}

struct mpii_rcb *
mpii_reply(struct mpii_softc *sc, struct mpii_reply_descr *rdp)
{
	struct mpii_rcb		*rcb = NULL;
	u_int32_t		rfid;

	KASSERT(mutex_owned(&sc->sc_rep_mtx));
	DNPRINTF(MPII_D_INTR, "%s: mpii_reply\n", DEVNAME(sc));

	if ((rdp->reply_flags & MPII_REPLY_DESCR_TYPE_MASK) ==
	    MPII_REPLY_DESCR_ADDRESS_REPLY) {
		rfid = (le32toh(rdp->frame_addr) -
		    (u_int32_t)MPII_DMA_DVA(sc->sc_replies)) /
		    sc->sc_reply_size;

		bus_dmamap_sync(sc->sc_dmat,
		    MPII_DMA_MAP(sc->sc_replies), sc->sc_reply_size * rfid,
		    sc->sc_reply_size, BUS_DMASYNC_POSTREAD);

		rcb = &sc->sc_rcbs[rfid];
	}

	memset(rdp, 0xff, sizeof(*rdp));

	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_reply_postq),
	    8 * sc->sc_reply_post_host_index, 8,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	return (rcb);
}

struct mpii_dmamem *
mpii_dmamem_alloc(struct mpii_softc *sc, size_t size)
{
	struct mpii_dmamem	*mdm;
	int			nsegs;

	mdm = malloc(sizeof(*mdm), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (mdm == NULL)
		return (NULL);

	mdm->mdm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &mdm->mdm_map) != 0)
		goto mdmfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &mdm->mdm_seg,
	    1, &nsegs, BUS_DMA_NOWAIT) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &mdm->mdm_seg, nsegs, size,
	    &mdm->mdm_kva, BUS_DMA_NOWAIT) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, mdm->mdm_map, mdm->mdm_kva, size,
	    NULL, BUS_DMA_NOWAIT) != 0)
		goto unmap;

	memset(mdm->mdm_kva, 0, size);

	return (mdm);

unmap:
	bus_dmamem_unmap(sc->sc_dmat, mdm->mdm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &mdm->mdm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, mdm->mdm_map);
mdmfree:
	free(mdm, M_DEVBUF);

	return (NULL);
}

void
mpii_dmamem_free(struct mpii_softc *sc, struct mpii_dmamem *mdm)
{
	DNPRINTF(MPII_D_MEM, "%s: mpii_dmamem_free %p\n", DEVNAME(sc), mdm);

	bus_dmamap_unload(sc->sc_dmat, mdm->mdm_map);
	bus_dmamem_unmap(sc->sc_dmat, mdm->mdm_kva, mdm->mdm_size);
	bus_dmamem_free(sc->sc_dmat, &mdm->mdm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, mdm->mdm_map);
	free(mdm, M_DEVBUF);
}

int
mpii_insert_dev(struct mpii_softc *sc, struct mpii_device *dev)
{
	int		slot;   /* initial hint */

	KASSERT(mutex_owned(&sc->sc_devs_mtx));
	DNPRINTF(MPII_D_EVT, "%s: mpii_insert_dev wants slot %d\n",
	    DEVNAME(sc), dev->slot);
	if (dev == NULL || dev->slot < 0)
		return (1);
	slot = dev->slot;

	while (slot < sc->sc_max_devices && sc->sc_devs[slot] != NULL)
		slot++;

	if (slot >= sc->sc_max_devices)
		return (1);

	DNPRINTF(MPII_D_EVT, "%s: mpii_insert_dev alloc slot %d\n",
	    DEVNAME(sc), slot);

	dev->slot = slot;
	sc->sc_devs[slot] = dev;

	return (0);
}

int
mpii_remove_dev(struct mpii_softc *sc, struct mpii_device *dev)
{
	int			i;

	KASSERT(mutex_owned(&sc->sc_devs_mtx));
	if (dev == NULL)
		return (1);

	for (i = 0; i < sc->sc_max_devices; i++) {
		if (sc->sc_devs[i] == NULL)
			continue;

		if (sc->sc_devs[i]->dev_handle == dev->dev_handle) {
			sc->sc_devs[i] = NULL;
			return (0);
		}
	}

	return (1);
}

struct mpii_device *
mpii_find_dev(struct mpii_softc *sc, u_int16_t handle)
{
	int			i;
	KASSERT(mutex_owned(&sc->sc_devs_mtx));

	for (i = 0; i < sc->sc_max_devices; i++) {
		if (sc->sc_devs[i] == NULL)
			continue;

		if (sc->sc_devs[i]->dev_handle == handle)
			return (sc->sc_devs[i]);
	}

	return (NULL);
}

int
mpii_alloc_ccbs(struct mpii_softc *sc)
{
	struct mpii_ccb		*ccb;
	u_int8_t		*cmd;
	int			i;
	char wqname[16];

	SIMPLEQ_INIT(&sc->sc_ccb_free);
	SIMPLEQ_INIT(&sc->sc_ccb_tmos);
	mutex_init(&sc->sc_ccb_free_mtx, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_ccb_free_cv, "mpii_ccbs");
	mutex_init(&sc->sc_ssb_tmomtx, MUTEX_DEFAULT, IPL_BIO);
	snprintf(wqname, sizeof(wqname) - 1, "%sabrt", DEVNAME(sc));
	workqueue_create(&sc->sc_ssb_tmowk, wqname, mpii_scsi_cmd_tmo_handler,
	    sc, PRI_BIO, IPL_BIO, WQ_MPSAFE);
	if (sc->sc_ssb_tmowk == NULL)
		return 1;

	sc->sc_ccbs = malloc((sc->sc_max_cmds-1) * sizeof(*ccb),
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (sc->sc_ccbs == NULL) {
		printf("%s: unable to allocate ccbs\n", DEVNAME(sc));
		return (1);
	}

	sc->sc_requests = mpii_dmamem_alloc(sc,
	    sc->sc_request_size * sc->sc_max_cmds);
	if (sc->sc_requests == NULL) {
		printf("%s: unable to allocate ccb dmamem\n", DEVNAME(sc));
		goto free_ccbs;
	}
	cmd = MPII_DMA_KVA(sc->sc_requests);

	/*
	 * we have sc->sc_max_cmds system request message
	 * frames, but smid zero cannot be used. so we then
	 * have (sc->sc_max_cmds - 1) number of ccbs
	 */
	for (i = 1; i < sc->sc_max_cmds; i++) {
		ccb = &sc->sc_ccbs[i - 1];

		if (bus_dmamap_create(sc->sc_dmat, MAXPHYS, sc->sc_max_sgl,
		    MAXPHYS, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
		    &ccb->ccb_dmamap) != 0) {
			printf("%s: unable to create dma map\n", DEVNAME(sc));
			goto free_maps;
		}

		ccb->ccb_sc = sc;
		mutex_init(&ccb->ccb_mtx, MUTEX_DEFAULT, IPL_BIO);
		cv_init(&ccb->ccb_cv, "mpiiexec");

		ccb->ccb_smid = htole16(i);
		ccb->ccb_offset = sc->sc_request_size * i;

		ccb->ccb_cmd = &cmd[ccb->ccb_offset];
		ccb->ccb_cmd_dva = (u_int32_t)MPII_DMA_DVA(sc->sc_requests) +
		    ccb->ccb_offset;

		DNPRINTF(MPII_D_CCB, "%s: mpii_alloc_ccbs(%d) ccb: %p map: %p "
		    "sc: %p smid: %#x offs: %#lx cmd: %p dva: %#lx\n",
		    DEVNAME(sc), i, ccb, ccb->ccb_dmamap, ccb->ccb_sc,
		    ccb->ccb_smid, ccb->ccb_offset, ccb->ccb_cmd,
		    ccb->ccb_cmd_dva);

		mpii_put_ccb(sc, ccb);
	}

	return (0);

free_maps:
	while ((ccb = mpii_get_ccb(sc)) != NULL)
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);

	mpii_dmamem_free(sc, sc->sc_requests);
free_ccbs:
	free(sc->sc_ccbs, M_DEVBUF);

	return (1);
}

void
mpii_put_ccb(struct mpii_softc *sc, struct mpii_ccb *ccb)
{
	DNPRINTF(MPII_D_CCB, "%s: mpii_put_ccb %p\n", DEVNAME(sc), ccb);

	ccb->ccb_state = MPII_CCB_FREE;
	ccb->ccb_cookie = NULL;
	ccb->ccb_done = NULL;
	ccb->ccb_rcb = NULL;
	memset(ccb->ccb_cmd, 0, sc->sc_request_size);

	mutex_enter(&sc->sc_ccb_free_mtx);
	SIMPLEQ_INSERT_HEAD(&sc->sc_ccb_free, ccb, ccb_link);
	mutex_exit(&sc->sc_ccb_free_mtx);
}

struct mpii_ccb *
mpii_get_ccb(struct mpii_softc *sc)
{
	struct mpii_ccb		*ccb;

	mutex_enter(&sc->sc_ccb_free_mtx);
	ccb = SIMPLEQ_FIRST(&sc->sc_ccb_free);
	if (ccb != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_free, ccb_link);
		ccb->ccb_state = MPII_CCB_READY;
		KASSERT(ccb->ccb_sc == sc);
	}
	mutex_exit(&sc->sc_ccb_free_mtx);

	DNPRINTF(MPII_D_CCB, "%s: mpii_get_ccb %p\n", DEVNAME(sc), ccb);

	return (ccb);
}

int
mpii_alloc_replies(struct mpii_softc *sc)
{
	DNPRINTF(MPII_D_MISC, "%s: mpii_alloc_replies\n", DEVNAME(sc));

	sc->sc_rcbs = malloc(sc->sc_num_reply_frames * sizeof(struct mpii_rcb),
	    M_DEVBUF, M_NOWAIT);
	if (sc->sc_rcbs == NULL)
		return (1);

	sc->sc_replies = mpii_dmamem_alloc(sc, sc->sc_reply_size *
	    sc->sc_num_reply_frames);
	if (sc->sc_replies == NULL) {
		free(sc->sc_rcbs, M_DEVBUF);
		return (1);
	}

	return (0);
}

void
mpii_push_replies(struct mpii_softc *sc)
{
	struct mpii_rcb		*rcb;
	uintptr_t		kva = (uintptr_t)MPII_DMA_KVA(sc->sc_replies);
	int			i;

	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_replies),
	    0, sc->sc_reply_size * sc->sc_num_reply_frames,
	    BUS_DMASYNC_PREREAD);

	for (i = 0; i < sc->sc_num_reply_frames; i++) {
		rcb = &sc->sc_rcbs[i];

		rcb->rcb_reply = (void *)(kva + sc->sc_reply_size * i);
		rcb->rcb_reply_dva = (u_int32_t)MPII_DMA_DVA(sc->sc_replies) +
		    sc->sc_reply_size * i;
		mpii_push_reply(sc, rcb);
	}
}

void
mpii_start(struct mpii_softc *sc, struct mpii_ccb *ccb)
{
	struct mpii_request_header	*rhp;
	struct mpii_request_descr	descr;
#if defined(__LP64__) && 0
	u_long				 *rdp = (u_long *)&descr;
#else
	u_int32_t			 *rdp = (u_int32_t *)&descr;
#endif

	DNPRINTF(MPII_D_RW, "%s: mpii_start %#lx\n", DEVNAME(sc),
	    ccb->ccb_cmd_dva);

	bus_dmamap_sync(sc->sc_dmat, MPII_DMA_MAP(sc->sc_requests),
	    ccb->ccb_offset, sc->sc_request_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	ccb->ccb_state = MPII_CCB_QUEUED;

	rhp = ccb->ccb_cmd;

	memset(&descr, 0, sizeof(descr));

	switch (rhp->function) {
	case MPII_FUNCTION_SCSI_IO_REQUEST:
		descr.request_flags = MPII_REQ_DESCR_SCSI_IO;
		descr.dev_handle = htole16(ccb->ccb_dev_handle);
		break;
	case MPII_FUNCTION_SCSI_TASK_MGMT:
		descr.request_flags = MPII_REQ_DESCR_HIGH_PRIORITY;
		break;
	default:
		descr.request_flags = MPII_REQ_DESCR_DEFAULT;
	}

	descr.vf_id = sc->sc_vf_id;
	descr.smid = ccb->ccb_smid;

#if defined(__LP64__) && 0
	DNPRINTF(MPII_D_RW, "%s:   MPII_REQ_DESCR_POST_LOW (0x%08x) write "
	    "0x%08lx\n", DEVNAME(sc), MPII_REQ_DESCR_POST_LOW, *rdp);
	bus_space_write_raw_8(sc->sc_iot, sc->sc_ioh,
	    MPII_REQ_DESCR_POST_LOW, *rdp);
#else
	DNPRINTF(MPII_D_RW, "%s:   MPII_REQ_DESCR_POST_LOW (0x%08x) write "
	    "0x%04x\n", DEVNAME(sc), MPII_REQ_DESCR_POST_LOW, *rdp);

	DNPRINTF(MPII_D_RW, "%s:   MPII_REQ_DESCR_POST_HIGH (0x%08x) write "
	    "0x%04x\n", DEVNAME(sc), MPII_REQ_DESCR_POST_HIGH, *(rdp+1));

	mutex_enter(&sc->sc_req_mtx);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    MPII_REQ_DESCR_POST_LOW, rdp[0]);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh,
	    MPII_REQ_DESCR_POST_LOW, 8, BUS_SPACE_BARRIER_WRITE);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    MPII_REQ_DESCR_POST_HIGH, rdp[1]);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh,
	    MPII_REQ_DESCR_POST_LOW, 8, BUS_SPACE_BARRIER_WRITE);
	mutex_exit(&sc->sc_req_mtx);
#endif
}

int
mpii_poll(struct mpii_softc *sc, struct mpii_ccb *ccb)
{
	void				(*done)(struct mpii_ccb *);
	void				*cookie;
	int				rv = 1;

	DNPRINTF(MPII_D_INTR, "%s: mpii_poll\n", DEVNAME(sc));

	done = ccb->ccb_done;
	cookie = ccb->ccb_cookie;

	ccb->ccb_done = mpii_poll_done;
	ccb->ccb_cookie = &rv;

	mpii_start(sc, ccb);

	while (rv == 1) {
		/* avoid excessive polling */
		if (mpii_reply_waiting(sc))
			mpii_intr(sc);
		else
			delay(10);
	}

	ccb->ccb_cookie = cookie;
	done(ccb);

	return (0);
}

void
mpii_poll_done(struct mpii_ccb *ccb)
{
	int				*rv = ccb->ccb_cookie;

	*rv = 0;
}

int
mpii_alloc_queues(struct mpii_softc *sc)
{
	u_int32_t		*rfp;
	int			i;

	DNPRINTF(MPII_D_MISC, "%s: mpii_alloc_queues\n", DEVNAME(sc));

	mutex_init(&sc->sc_reply_free_mtx, MUTEX_DEFAULT, IPL_BIO);
	sc->sc_reply_freeq = mpii_dmamem_alloc(sc,
	    sc->sc_reply_free_qdepth * sizeof(*rfp));
	if (sc->sc_reply_freeq == NULL)
		return (1);
	rfp = MPII_DMA_KVA(sc->sc_reply_freeq);
	for (i = 0; i < sc->sc_num_reply_frames; i++) {
		rfp[i] = (u_int32_t)MPII_DMA_DVA(sc->sc_replies) +
		    sc->sc_reply_size * i;
	}

	sc->sc_reply_postq = mpii_dmamem_alloc(sc,
	    sc->sc_reply_post_qdepth * sizeof(struct mpii_reply_descr));
	if (sc->sc_reply_postq == NULL)
		goto free_reply_freeq;
	sc->sc_reply_postq_kva = MPII_DMA_KVA(sc->sc_reply_postq);
	memset(sc->sc_reply_postq_kva, 0xff, sc->sc_reply_post_qdepth *
	    sizeof(struct mpii_reply_descr));

	return (0);

free_reply_freeq:
	mpii_dmamem_free(sc, sc->sc_reply_freeq);
	return (1);
}

void
mpii_init_queues(struct mpii_softc *sc)
{
	DNPRINTF(MPII_D_MISC, "%s:  mpii_init_queues\n", DEVNAME(sc));

	sc->sc_reply_free_host_index = sc->sc_reply_free_qdepth - 1;
	sc->sc_reply_post_host_index = 0;
	mpii_write_reply_free(sc, sc->sc_reply_free_host_index);
	mpii_write_reply_post(sc, sc->sc_reply_post_host_index);
}

void
mpii_wait(struct mpii_softc *sc, struct mpii_ccb *ccb)
{
	void			(*done)(struct mpii_ccb *);
	void			*cookie;

	done = ccb->ccb_done;
	cookie = ccb->ccb_cookie;

	ccb->ccb_done = mpii_wait_done;
	ccb->ccb_cookie = ccb;

	/* XXX this will wait forever for the ccb to complete */

	mpii_start(sc, ccb);

	mutex_enter(&ccb->ccb_mtx);
	while (ccb->ccb_cookie != NULL)
		cv_wait(&ccb->ccb_cv, &ccb->ccb_mtx);
	mutex_exit(&ccb->ccb_mtx);

	ccb->ccb_cookie = cookie;
	done(ccb);
}

void
mpii_wait_done(struct mpii_ccb *ccb)
{
	mutex_enter(&ccb->ccb_mtx);
	ccb->ccb_cookie = NULL;
	cv_signal(&ccb->ccb_cv);
	mutex_exit(&ccb->ccb_mtx);
}

void
mpii_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
    void *arg)
{
	struct scsipi_periph	*periph;
	struct scsipi_xfer	*xs;
	struct scsipi_adapter	*adapt = chan->chan_adapter;
	struct mpii_softc	*sc = device_private(adapt->adapt_dev);
	struct mpii_ccb		*ccb;
	struct mpii_msg_scsi_io	*io;
	struct mpii_device	*dev;
	int			target, timeout, ret;
	u_int16_t		dev_handle;

	DNPRINTF(MPII_D_CMD, "%s: mpii_scsipi_request\n", DEVNAME(sc));

	switch (req) {
	case ADAPTER_REQ_GROW_RESOURCES:
		/* Not supported. */ 
		return;
	case ADAPTER_REQ_SET_XFER_MODE:
	{
		struct scsipi_xfer_mode *xm = arg;
		xm->xm_mode = PERIPH_CAP_TQING;
		xm->xm_period = 0;   
		xm->xm_offset = 0;   
		scsipi_async_event(&sc->sc_chan, ASYNC_EVENT_XFER_MODE, xm);
		return;
	}
	case ADAPTER_REQ_RUN_XFER:    
		break;
	}

	xs = arg;
	periph = xs->xs_periph;
	target = periph->periph_target;

	if (xs->cmdlen > MPII_CDB_LEN) {
		DNPRINTF(MPII_D_CMD, "%s: CDB too big %d\n",
		    DEVNAME(sc), xs->cmdlen);
		memset(&xs->sense, 0, sizeof(xs->sense));
		xs->sense.scsi_sense.response_code =
		    SSD_RCODE_VALID | SSD_RCODE_CURRENT;
		xs->sense.scsi_sense.flags = SKEY_ILLEGAL_REQUEST;
		xs->sense.scsi_sense.asc = 0x20;
		xs->error = XS_SENSE;
		scsipi_done(xs);
		return;
	}

	mutex_enter(&sc->sc_devs_mtx);
	if ((dev = sc->sc_devs[target]) == NULL) {
		mutex_exit(&sc->sc_devs_mtx);
		/* device no longer exists */
		xs->error = XS_SELTIMEOUT;
		scsipi_done(xs);
		return;
	}
	dev_handle = dev->dev_handle;
	mutex_exit(&sc->sc_devs_mtx);

	ccb = mpii_get_ccb(sc);
	if (ccb == NULL) {
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_done(xs);
		return;
	}
	DNPRINTF(MPII_D_CMD, "%s: ccb_smid: %d xs->xs_control: 0x%x\n",
	    DEVNAME(sc), ccb->ccb_smid, xs->xs_control);

	ccb->ccb_cookie = xs;
	ccb->ccb_done = mpii_scsi_cmd_done;
	ccb->ccb_dev_handle = dev_handle;

	io = ccb->ccb_cmd;
	memset(io, 0, sizeof(*io));
	io->function = MPII_FUNCTION_SCSI_IO_REQUEST;
	io->sense_buffer_length = sizeof(xs->sense);
	io->sgl_offset0 = sizeof(struct mpii_msg_scsi_io) / 4;
	io->io_flags = htole16(xs->cmdlen);
	io->dev_handle = htole16(ccb->ccb_dev_handle);
	io->lun[0] = htobe16(periph->periph_lun);

	switch (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
	case XS_CTL_DATA_IN:
		io->direction = MPII_SCSIIO_DIR_READ;
		break;
	case XS_CTL_DATA_OUT:
		io->direction = MPII_SCSIIO_DIR_WRITE;
		break;
	default:
		io->direction = MPII_SCSIIO_DIR_NONE;
		break;
	}

	io->tagging = MPII_SCSIIO_ATTR_SIMPLE_Q;

	memcpy(io->cdb, xs->cmd, xs->cmdlen);

	io->data_length = htole32(xs->datalen);

	/* sense data is at the end of a request */
	io->sense_buffer_low_address = htole32(ccb->ccb_cmd_dva +
	    sc->sc_request_size - sizeof(struct scsi_sense_data));

	if (ISSET(sc->sc_flags, MPII_F_SAS3))
		ret = mpii_load_xs_sas3(ccb);
	else
		ret = mpii_load_xs(ccb);

	if (ret != 0) {
		xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}

	if (xs->xs_control & XS_CTL_POLL) {
		if (mpii_poll(sc, ccb) != 0) {
			xs->error = XS_DRIVER_STUFFUP;
			goto done;
		}
		return;
	}
        timeout = mstohz(xs->timeout);
	if (timeout == 0)
		timeout = 1;
	callout_reset(&xs->xs_callout, timeout, mpii_scsi_cmd_tmo, ccb);
	mpii_start(sc, ccb);
	return;
done:
	mpii_put_ccb(sc, ccb);
	scsipi_done(xs);
}

void
mpii_scsi_cmd_tmo(void *xccb)
{
	struct mpii_ccb		*ccb = xccb;
	struct mpii_softc	*sc = ccb->ccb_sc;
	bool	start_work;

	printf("%s: mpii_scsi_cmd_tmo\n", DEVNAME(sc));

	if (ccb->ccb_state == MPII_CCB_QUEUED) {
		mutex_enter(&sc->sc_ssb_tmomtx);
		start_work = (SIMPLEQ_FIRST(&sc->sc_ccb_tmos) == 0);
		ccb->ccb_state = MPII_CCB_TIMEOUT;
		SIMPLEQ_INSERT_HEAD(&sc->sc_ccb_tmos, ccb, ccb_link);
		if (start_work) {
			workqueue_enqueue(sc->sc_ssb_tmowk,
			    &sc->sc_ssb_tmowork, NULL);
		}
		mutex_exit(&sc->sc_ssb_tmomtx);
	}
}

void
mpii_scsi_cmd_tmo_handler(struct work *wk, void *cookie)
{
	struct mpii_softc			*sc = cookie;
	struct mpii_ccb				*next;
	struct mpii_ccb				*ccb;
	struct mpii_ccb				*tccb;
	struct mpii_msg_scsi_task_request	*stq;

	mutex_enter(&sc->sc_ssb_tmomtx);
	next = SIMPLEQ_FIRST(&sc->sc_ccb_tmos);
	SIMPLEQ_INIT(&sc->sc_ccb_tmos);
	mutex_exit(&sc->sc_ssb_tmomtx);

	while (next != NULL) {
		ccb = next;
		next = SIMPLEQ_NEXT(ccb, ccb_link);
		if (ccb->ccb_state != MPII_CCB_TIMEOUT)
			continue;
		tccb = mpii_get_ccb(sc);
		stq = tccb->ccb_cmd;
		stq->function = MPII_FUNCTION_SCSI_TASK_MGMT;
		stq->task_type = MPII_SCSI_TASK_TARGET_RESET;
		stq->dev_handle = htole16(ccb->ccb_dev_handle);

		tccb->ccb_done = mpii_scsi_cmd_tmo_done;
		mpii_wait(sc, tccb);
	}
}

void
mpii_scsi_cmd_tmo_done(struct mpii_ccb *tccb)
{
	mpii_put_ccb(tccb->ccb_sc, tccb);
}

static u_int8_t
map_scsi_status(u_int8_t mpii_scsi_status)
{
	u_int8_t scsi_status;
	
	switch (mpii_scsi_status) 
	{
	case MPII_SCSIIO_STATUS_GOOD:
		scsi_status = SCSI_OK;
		break;
		
	case MPII_SCSIIO_STATUS_CHECK_COND:
		scsi_status = SCSI_CHECK;
		break;
		
	case MPII_SCSIIO_STATUS_BUSY:
		scsi_status = SCSI_BUSY;
		break;
		
	case MPII_SCSIIO_STATUS_INTERMEDIATE:
		scsi_status = SCSI_INTERM;
		break;
		
	case MPII_SCSIIO_STATUS_INTERMEDIATE_CONDMET:
		scsi_status = SCSI_INTERM;
		break;
		
	case MPII_SCSIIO_STATUS_RESERVATION_CONFLICT:
		scsi_status = SCSI_RESV_CONFLICT;
		break;
		
	case MPII_SCSIIO_STATUS_CMD_TERM:
	case MPII_SCSIIO_STATUS_TASK_ABORTED:
		scsi_status = SCSI_TERMINATED;
		break;

	case MPII_SCSIIO_STATUS_TASK_SET_FULL:
		scsi_status = SCSI_QUEUE_FULL;
		break;

	case MPII_SCSIIO_STATUS_ACA_ACTIVE:
		scsi_status = SCSI_ACA_ACTIVE;
		break;

	default:
		/* XXX: for the lack of anything better and other than OK */
		scsi_status = 0xFF;
		break;
	}

	return scsi_status;
}

void
mpii_scsi_cmd_done(struct mpii_ccb *ccb)
{
	struct mpii_msg_scsi_io_error	*sie;
	struct mpii_softc	*sc = ccb->ccb_sc;
	struct scsipi_xfer	*xs = ccb->ccb_cookie;
	struct scsi_sense_data	*sense;
	bus_dmamap_t		dmap = ccb->ccb_dmamap;
	bool timeout = 1;

	callout_stop(&xs->xs_callout);
	if (ccb->ccb_state == MPII_CCB_TIMEOUT)
		timeout = 1;
	ccb->ccb_state = MPII_CCB_READY;

	if (xs->datalen != 0) {
		bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_POSTREAD :
		    BUS_DMASYNC_POSTWRITE);

		bus_dmamap_unload(sc->sc_dmat, dmap);
	}

	xs->error = XS_NOERROR;
	xs->resid = 0;

	if (ccb->ccb_rcb == NULL) {
		/* no scsi error, we're ok so drop out early */
		xs->status = SCSI_OK;
		goto done;
	}

	sie = ccb->ccb_rcb->rcb_reply;

	DNPRINTF(MPII_D_CMD, "%s: mpii_scsi_cmd_done xs cmd: 0x%02x len: %d "
	    "flags 0x%x\n", DEVNAME(sc), xs->cmd->opcode, xs->datalen,
	    xs->xs_control);
	DNPRINTF(MPII_D_CMD, "%s:  dev_handle: %d msg_length: %d "
	    "function: 0x%02x\n", DEVNAME(sc), le16toh(sie->dev_handle),
	    sie->msg_length, sie->function);
	DNPRINTF(MPII_D_CMD, "%s:  vp_id: 0x%02x vf_id: 0x%02x\n", DEVNAME(sc),
	    sie->vp_id, sie->vf_id);
	DNPRINTF(MPII_D_CMD, "%s:  scsi_status: 0x%02x scsi_state: 0x%02x "
	    "ioc_status: 0x%04x\n", DEVNAME(sc), sie->scsi_status,
	    sie->scsi_state, le16toh(sie->ioc_status));
	DNPRINTF(MPII_D_CMD, "%s:  ioc_loginfo: 0x%08x\n", DEVNAME(sc),
	    le32toh(sie->ioc_loginfo));
	DNPRINTF(MPII_D_CMD, "%s:  transfer_count: %d\n", DEVNAME(sc),
	    le32toh(sie->transfer_count));
	DNPRINTF(MPII_D_CMD, "%s:  sense_count: %d\n", DEVNAME(sc),
	    le32toh(sie->sense_count));
	DNPRINTF(MPII_D_CMD, "%s:  response_info: 0x%08x\n", DEVNAME(sc),
	    le32toh(sie->response_info));
	DNPRINTF(MPII_D_CMD, "%s:  task_tag: 0x%04x\n", DEVNAME(sc),
	    le16toh(sie->task_tag));
	DNPRINTF(MPII_D_CMD, "%s:  bidirectional_transfer_count: 0x%08x\n",
	    DEVNAME(sc), le32toh(sie->bidirectional_transfer_count));

	xs->status = map_scsi_status(sie->scsi_status);

	switch (le16toh(sie->ioc_status) & MPII_IOCSTATUS_MASK) {
	case MPII_IOCSTATUS_SCSI_DATA_UNDERRUN:
		switch(sie->scsi_status) {
		case MPII_SCSIIO_STATUS_CHECK_COND:
			xs->error = XS_SENSE;
		case MPII_SCSIIO_STATUS_GOOD:
			xs->resid = xs->datalen - le32toh(sie->transfer_count);
			break;
		default:
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		break;

	case MPII_IOCSTATUS_SUCCESS:
	case MPII_IOCSTATUS_SCSI_RECOVERED_ERROR:
		switch (sie->scsi_status) {
		case MPII_SCSIIO_STATUS_GOOD:
			break;

		case MPII_SCSIIO_STATUS_CHECK_COND:
			xs->error = XS_SENSE;
			break;

		case MPII_SCSIIO_STATUS_BUSY:
		case MPII_SCSIIO_STATUS_TASK_SET_FULL:
			xs->error = XS_BUSY;
			break;

		default:
			xs->error = XS_DRIVER_STUFFUP;
		}
		break;

	case MPII_IOCSTATUS_BUSY:
	case MPII_IOCSTATUS_INSUFFICIENT_RESOURCES:
		xs->error = XS_BUSY;
		break;

	case MPII_IOCSTATUS_SCSI_IOC_TERMINATED:
	case MPII_IOCSTATUS_SCSI_TASK_TERMINATED:
		xs->error = timeout ? XS_TIMEOUT : XS_RESET;
		break;

	case MPII_IOCSTATUS_SCSI_INVALID_DEVHANDLE:
	case MPII_IOCSTATUS_SCSI_DEVICE_NOT_THERE:
		xs->error = XS_SELTIMEOUT;
		break;

	default:
		xs->error = XS_DRIVER_STUFFUP;
		break;
	}

	sense = (struct scsi_sense_data *)((uintptr_t)ccb->ccb_cmd +
	    sc->sc_request_size - sizeof(*sense));
	if (sie->scsi_state & MPII_SCSIIO_STATE_AUTOSENSE_VALID)
		memcpy(&xs->sense, sense, sizeof(xs->sense));

	DNPRINTF(MPII_D_CMD, "%s:  xs err: %d status: %#x\n", DEVNAME(sc),
	    xs->error, xs->status);

	mpii_push_reply(sc, ccb->ccb_rcb);
done:
	mpii_put_ccb(sc, ccb);
	scsipi_done(xs);
}

#if 0
int
mpii_scsi_ioctl(struct scsi_link *link, u_long cmd, void *addr, int flag)
{
	struct mpii_softc	*sc = (struct mpii_softc *)link->adapter_softc;
	struct mpii_device	*dev = sc->sc_devs[link->target];

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_scsi_ioctl\n", DEVNAME(sc));

	switch (cmd) {
	case DIOCGCACHE:
	case DIOCSCACHE:
		if (dev != NULL && ISSET(dev->flags, MPII_DF_VOLUME)) {
			return (mpii_ioctl_cache(link, cmd,
			    (struct dk_cache *)addr));
		}
		break;

	default:
		if (sc->sc_ioctl)
			return (sc->sc_ioctl(link->adapter_softc, cmd, addr));

		break;
	}

	return (ENOTTY);
}

int
mpii_ioctl_cache(struct scsi_link *link, u_long cmd, struct dk_cache *dc)
{
	struct mpii_softc *sc = (struct mpii_softc *)link->adapter_softc;
	struct mpii_device *dev = sc->sc_devs[link->target];
	struct mpii_cfg_raid_vol_pg0 *vpg;
	struct mpii_msg_raid_action_request *req;
	struct mpii_msg_raid_action_reply *rep;
	struct mpii_cfg_hdr hdr;
	struct mpii_ccb	*ccb;
	u_int32_t addr = MPII_CFG_RAID_VOL_ADDR_HANDLE | dev->dev_handle;
	size_t pagelen;
	int rv = 0;
	int enabled;

	if (mpii_req_cfg_header(sc, MPII_CONFIG_REQ_PAGE_TYPE_RAID_VOL, 0,
	    addr, MPII_PG_POLL, &hdr) != 0)
		return (EINVAL);

	pagelen = hdr.page_length * 4;
	vpg = malloc(pagelen, M_TEMP, M_WAITOK | M_ZERO);
	if (vpg == NULL)
		return (ENOMEM);

	if (mpii_req_cfg_page(sc, addr, MPII_PG_POLL, &hdr, 1,
	    vpg, pagelen) != 0) {
		rv = EINVAL;
		goto done;
	}

	enabled = ((le16toh(vpg->volume_settings) &
	    MPII_CFG_RAID_VOL_0_SETTINGS_CACHE_MASK) ==
	    MPII_CFG_RAID_VOL_0_SETTINGS_CACHE_ENABLED) ? 1 : 0;

	if (cmd == DIOCGCACHE) {
		dc->wrcache = enabled;
		dc->rdcache = 0;
		goto done;
	} /* else DIOCSCACHE */

	if (dc->rdcache) {
		rv = EOPNOTSUPP;
		goto done;
	}

	if (((dc->wrcache) ? 1 : 0) == enabled)
		goto done;

	ccb = scsi_io_get(&sc->sc_iopool, SCSI_POLL);
	if (ccb == NULL) {
		rv = ENOMEM;
		goto done;
	}

	ccb->ccb_done = mpii_empty_done;

	req = ccb->ccb_cmd;
	memset(req, 0, sizeof(*req));
	req->function = MPII_FUNCTION_RAID_ACTION;
	req->action = MPII_RAID_ACTION_CHANGE_VOL_WRITE_CACHE;
	req->vol_dev_handle = htole16(dev->dev_handle);
	req->action_data = htole32(dc->wrcache ?
	    MPII_RAID_VOL_WRITE_CACHE_ENABLE :
	    MPII_RAID_VOL_WRITE_CACHE_DISABLE);

	if (mpii_poll(sc, ccb) != 0) {
		rv = EIO;
		goto done;
	}

	if (ccb->ccb_rcb != NULL) {
		rep = ccb->ccb_rcb->rcb_reply;
		if ((rep->ioc_status != MPII_IOCSTATUS_SUCCESS) ||
		    ((rep->action_data[0] &
		     MPII_RAID_VOL_WRITE_CACHE_MASK) !=
		    (dc->wrcache ? MPII_RAID_VOL_WRITE_CACHE_ENABLE :
		     MPII_RAID_VOL_WRITE_CACHE_DISABLE)))
			rv = EINVAL;
		mpii_push_reply(sc, ccb->ccb_rcb);
	}

	scsi_io_put(&sc->sc_iopool, ccb);

done:
	free(vpg, M_TEMP);
	return (rv);
}
#endif /* 0 */

#if NBIO > 0
int
mpii_ioctl(device_t dev, u_long cmd, void *addr)
{
	struct mpii_softc	*sc = device_private(dev);
	int			error = 0;

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_ioctl ", DEVNAME(sc));

	switch (cmd) {
	case BIOCINQ:
		DNPRINTF(MPII_D_IOCTL, "inq\n");
		error = mpii_ioctl_inq(sc, (struct bioc_inq *)addr);
		break;
	case BIOCVOL:
		DNPRINTF(MPII_D_IOCTL, "vol\n");
		error = mpii_ioctl_vol(sc, (struct bioc_vol *)addr);
		break;
	case BIOCDISK:
		DNPRINTF(MPII_D_IOCTL, "disk\n");
		error = mpii_ioctl_disk(sc, (struct bioc_disk *)addr);
		break;
	default:
		DNPRINTF(MPII_D_IOCTL, " invalid ioctl\n");
		error = ENOTTY;
	}

	return (error);
}

int
mpii_ioctl_inq(struct mpii_softc *sc, struct bioc_inq *bi)
{
	int			i;

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_ioctl_inq\n", DEVNAME(sc));

	strlcpy(bi->bi_dev, DEVNAME(sc), sizeof(bi->bi_dev));
	mutex_enter(&sc->sc_devs_mtx);
	for (i = 0; i < sc->sc_max_devices; i++)
		if (sc->sc_devs[i] &&
		    ISSET(sc->sc_devs[i]->flags, MPII_DF_VOLUME))
			bi->bi_novol++;
	mutex_exit(&sc->sc_devs_mtx);
	return (0);
}

int
mpii_ioctl_vol(struct mpii_softc *sc, struct bioc_vol *bv)
{
	struct mpii_cfg_raid_vol_pg0	*vpg;
	struct mpii_cfg_hdr		hdr;
	struct mpii_device		*dev;
	size_t				pagelen;
	u_int16_t			volh;
	int				rv, hcnt = 0;
	int				percent;

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_ioctl_vol %d\n",
	    DEVNAME(sc), bv->bv_volid);

	mutex_enter(&sc->sc_devs_mtx);
	if ((dev = mpii_find_vol(sc, bv->bv_volid)) == NULL) {
		mutex_exit(&sc->sc_devs_mtx);
		return (ENODEV);
	}
	volh = dev->dev_handle;
	percent = dev->percent;
	mutex_exit(&sc->sc_devs_mtx);

	if (mpii_req_cfg_header(sc, MPII_CONFIG_REQ_PAGE_TYPE_RAID_VOL, 0,
	    MPII_CFG_RAID_VOL_ADDR_HANDLE | volh, 0, &hdr) != 0) {
		printf("%s: unable to fetch header for raid volume page 0\n",
		    DEVNAME(sc));
		return (EINVAL);
	}

	pagelen = hdr.page_length * 4;
	vpg = malloc(pagelen, M_TEMP, M_WAITOK | M_ZERO);
	if (vpg == NULL) {
		printf("%s: unable to allocate space for raid "
		    "volume page 0\n", DEVNAME(sc));
		return (ENOMEM);
	}

	if (mpii_req_cfg_page(sc, MPII_CFG_RAID_VOL_ADDR_HANDLE | volh, 0,
	    &hdr, 1, vpg, pagelen) != 0) {
		printf("%s: unable to fetch raid volume page 0\n",
		    DEVNAME(sc));
		free(vpg, M_TEMP);
		return (EINVAL);
	}

	switch (vpg->volume_state) {
	case MPII_CFG_RAID_VOL_0_STATE_ONLINE:
	case MPII_CFG_RAID_VOL_0_STATE_OPTIMAL:
		bv->bv_status = BIOC_SVONLINE;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_DEGRADED:
		if (ISSET(le32toh(vpg->volume_status),
		    MPII_CFG_RAID_VOL_0_STATUS_RESYNC)) {
			bv->bv_status = BIOC_SVREBUILD;
			bv->bv_percent = percent;
		} else
			bv->bv_status = BIOC_SVDEGRADED;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_FAILED:
		bv->bv_status = BIOC_SVOFFLINE;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_INITIALIZING:
		bv->bv_status = BIOC_SVBUILDING;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_MISSING:
	default:
		bv->bv_status = BIOC_SVINVALID;
		break;
	}

	switch (vpg->volume_type) {
	case MPII_CFG_RAID_VOL_0_TYPE_RAID0:
		bv->bv_level = 0;
		break;
	case MPII_CFG_RAID_VOL_0_TYPE_RAID1:
		bv->bv_level = 1;
		break;
	case MPII_CFG_RAID_VOL_0_TYPE_RAID1E:
	case MPII_CFG_RAID_VOL_0_TYPE_RAID10:
		bv->bv_level = 10;
		break;
	default:
		bv->bv_level = -1;
	}

	if ((rv = mpii_bio_hs(sc, NULL, 0, vpg->hot_spare_pool, &hcnt)) != 0) {
		free(vpg, M_TEMP);
		return (rv);
	}

	bv->bv_nodisk = vpg->num_phys_disks + hcnt;

	bv->bv_size = le64toh(vpg->max_lba) * le16toh(vpg->block_size);

	free(vpg, M_TEMP);
	return (0);
}

int
mpii_ioctl_disk(struct mpii_softc *sc, struct bioc_disk *bd)
{
	struct mpii_cfg_raid_vol_pg0		*vpg;
	struct mpii_cfg_raid_vol_pg0_physdisk	*pd;
	struct mpii_cfg_hdr			hdr;
	struct mpii_device			*dev;
	size_t					pagelen;
	u_int16_t				volh;
	u_int8_t				dn;

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_ioctl_disk %d/%d\n",
	    DEVNAME(sc), bd->bd_volid, bd->bd_diskid);

	mutex_enter(&sc->sc_devs_mtx);
	if ((dev = mpii_find_vol(sc, bd->bd_volid)) == NULL) {
		mutex_exit(&sc->sc_devs_mtx);
		return (ENODEV);
	}
	volh = dev->dev_handle;
	mutex_exit(&sc->sc_devs_mtx);

	if (mpii_req_cfg_header(sc, MPII_CONFIG_REQ_PAGE_TYPE_RAID_VOL, 0,
	    MPII_CFG_RAID_VOL_ADDR_HANDLE | volh, 0, &hdr) != 0) {
		printf("%s: unable to fetch header for raid volume page 0\n",
		    DEVNAME(sc));
		return (EINVAL);
	}

	pagelen = hdr.page_length * 4;
	vpg = malloc(pagelen, M_TEMP, M_WAITOK | M_CANFAIL | M_ZERO);
	if (vpg == NULL) {
		printf("%s: unable to allocate space for raid "
		    "volume page 0\n", DEVNAME(sc));
		return (ENOMEM);
	}

	if (mpii_req_cfg_page(sc, MPII_CFG_RAID_VOL_ADDR_HANDLE | volh, 0,
	    &hdr, 1, vpg, pagelen) != 0) {
		printf("%s: unable to fetch raid volume page 0\n",
		    DEVNAME(sc));
		free(vpg, M_TEMP);
		return (EINVAL);
	}

	if (bd->bd_diskid >= vpg->num_phys_disks) {
		int		nvdsk = vpg->num_phys_disks;
		int		hsmap = vpg->hot_spare_pool;

		free(vpg, M_TEMP);
		return (mpii_bio_hs(sc, bd, nvdsk, hsmap, NULL));
	}

	pd = (struct mpii_cfg_raid_vol_pg0_physdisk *)(vpg + 1) +
	    bd->bd_diskid;
	dn = pd->phys_disk_num;

	free(vpg, M_TEMP);
	return (mpii_bio_disk(sc, bd, dn));
}

int
mpii_bio_hs(struct mpii_softc *sc, struct bioc_disk *bd, int nvdsk,
     int hsmap, int *hscnt)
{
	struct mpii_cfg_raid_config_pg0	*cpg;
	struct mpii_raid_config_element	*el;
	struct mpii_ecfg_hdr		ehdr;
	size_t				pagelen;
	int				i, nhs = 0;

	if (bd) {
		DNPRINTF(MPII_D_IOCTL, "%s: mpii_bio_hs %d\n", DEVNAME(sc),
		    bd->bd_diskid - nvdsk);
	} else {
		DNPRINTF(MPII_D_IOCTL, "%s: mpii_bio_hs\n", DEVNAME(sc));
	}

	if (mpii_req_cfg_header(sc, MPII_CONFIG_REQ_PAGE_TYPE_RAID_CONFIG,
	    0, MPII_CFG_RAID_CONFIG_ACTIVE_CONFIG, MPII_PG_EXTENDED,
	    &ehdr) != 0) {
		printf("%s: unable to fetch header for raid config page 0\n",
		    DEVNAME(sc));
		return (EINVAL);
	}

	pagelen = le16toh(ehdr.ext_page_length) * 4;
	cpg = malloc(pagelen, M_TEMP, M_WAITOK | M_CANFAIL | M_ZERO);
	if (cpg == NULL) {
		printf("%s: unable to allocate space for raid config page 0\n",
		    DEVNAME(sc));
		return (ENOMEM);
	}

	if (mpii_req_cfg_page(sc, MPII_CFG_RAID_CONFIG_ACTIVE_CONFIG,
	    MPII_PG_EXTENDED, &ehdr, 1, cpg, pagelen) != 0) {
		printf("%s: unable to fetch raid config page 0\n",
		    DEVNAME(sc));
		free(cpg, M_TEMP);
		return (EINVAL);
	}

	el = (struct mpii_raid_config_element *)(cpg + 1);
	for (i = 0; i < cpg->num_elements; i++, el++) {
		if (ISSET(le16toh(el->element_flags),
		    MPII_RAID_CONFIG_ELEMENT_FLAG_HSP_PHYS_DISK) &&
		    el->hot_spare_pool == hsmap) {
			/*
			 * diskid comparison is based on the idea that all
			 * disks are counted by the bio(4) in sequence, thus
			 * substracting the number of disks in the volume
			 * from the diskid yields us a "relative" hotspare
			 * number, which is good enough for us.
			 */
			if (bd != NULL && bd->bd_diskid == nhs + nvdsk) {
				u_int8_t dn = el->phys_disk_num;

				free(cpg, M_TEMP);
				return (mpii_bio_disk(sc, bd, dn));
			}
			nhs++;
		}
	}

	if (hscnt)
		*hscnt = nhs;

	free(cpg, M_TEMP);
	return (0);
}

int
mpii_bio_disk(struct mpii_softc *sc, struct bioc_disk *bd, u_int8_t dn)
{
	struct mpii_cfg_raid_physdisk_pg0	*ppg;
	struct mpii_cfg_hdr			hdr;
	struct mpii_device			*dev;
	int					len;

	DNPRINTF(MPII_D_IOCTL, "%s: mpii_bio_disk %d\n", DEVNAME(sc),
	    bd->bd_diskid);

	ppg = malloc(sizeof(*ppg), M_TEMP, M_WAITOK | M_CANFAIL | M_ZERO);
	if (ppg == NULL) {
		printf("%s: unable to allocate space for raid physical disk "
		    "page 0\n", DEVNAME(sc));
		return (ENOMEM);
	}

	hdr.page_version = 0;
	hdr.page_length = sizeof(*ppg) / 4;
	hdr.page_number = 0;
	hdr.page_type = MPII_CONFIG_REQ_PAGE_TYPE_RAID_PD;

	if (mpii_req_cfg_page(sc, MPII_CFG_RAID_PHYS_DISK_ADDR_NUMBER | dn, 0,
	    &hdr, 1, ppg, sizeof(*ppg)) != 0) {
		printf("%s: unable to fetch raid drive page 0\n",
		    DEVNAME(sc));
		free(ppg, M_TEMP);
		return (EINVAL);
	}

	bd->bd_target = ppg->phys_disk_num;

	mutex_enter(&sc->sc_devs_mtx);
	if ((dev = mpii_find_dev(sc, le16toh(ppg->dev_handle))) == NULL) {
		mutex_exit(&sc->sc_devs_mtx);
		bd->bd_status = BIOC_SDINVALID;
		free(ppg, M_TEMP);
		return (0);
	}
	mutex_exit(&sc->sc_devs_mtx);

	switch (ppg->phys_disk_state) {
	case MPII_CFG_RAID_PHYDISK_0_STATE_ONLINE:
	case MPII_CFG_RAID_PHYDISK_0_STATE_OPTIMAL:
		bd->bd_status = BIOC_SDONLINE;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_OFFLINE:
		if (ppg->offline_reason ==
		    MPII_CFG_RAID_PHYDISK_0_OFFLINE_FAILED ||
		    ppg->offline_reason ==
		    MPII_CFG_RAID_PHYDISK_0_OFFLINE_FAILEDREQ)
			bd->bd_status = BIOC_SDFAILED;
		else
			bd->bd_status = BIOC_SDOFFLINE;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_DEGRADED:
		bd->bd_status = BIOC_SDFAILED;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_REBUILDING:
		bd->bd_status = BIOC_SDREBUILD;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_HOTSPARE:
		bd->bd_status = BIOC_SDHOTSPARE;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_NOTCONFIGURED:
		bd->bd_status = BIOC_SDUNUSED;
		break;
	case MPII_CFG_RAID_PHYDISK_0_STATE_NOTCOMPATIBLE:
	default:
		bd->bd_status = BIOC_SDINVALID;
		break;
	}

	bd->bd_size = le64toh(ppg->dev_max_lba) * le16toh(ppg->block_size);

	strnvisx(bd->bd_vendor, sizeof(bd->bd_vendor),
	    ppg->vendor_id, sizeof(ppg->vendor_id),
	    VIS_TRIM|VIS_SAFE|VIS_OCTAL);
	len = strlen(bd->bd_vendor);
	bd->bd_vendor[len] = ' ';
	strnvisx(&bd->bd_vendor[len + 1], sizeof(ppg->vendor_id) - len - 1,
	    ppg->product_id, sizeof(ppg->product_id),
	    VIS_TRIM|VIS_SAFE|VIS_OCTAL);
	strnvisx(bd->bd_serial, sizeof(bd->bd_serial),
	    ppg->serial, sizeof(ppg->serial), VIS_TRIM|VIS_SAFE|VIS_OCTAL);

	free(ppg, M_TEMP);
	return (0);
}

struct mpii_device *
mpii_find_vol(struct mpii_softc *sc, int volid)
{
	struct mpii_device	*dev = NULL;

	KASSERT(mutex_owned(&sc->sc_devs_mtx));
	if (sc->sc_vd_id_low + volid >= sc->sc_max_devices)
		return (NULL);
	dev = sc->sc_devs[sc->sc_vd_id_low + volid];
	if (dev && ISSET(dev->flags, MPII_DF_VOLUME))
		return (dev);
	return (NULL);
}

/*
 * Non-sleeping lightweight version of the mpii_ioctl_vol
 */
int
mpii_bio_volstate(struct mpii_softc *sc, struct bioc_vol *bv)
{
	struct mpii_cfg_raid_vol_pg0	*vpg;
	struct mpii_cfg_hdr		hdr;
	struct mpii_device		*dev = NULL;
	size_t				pagelen;
	u_int16_t			volh;

	mutex_enter(&sc->sc_devs_mtx);
	if ((dev = mpii_find_vol(sc, bv->bv_volid)) == NULL) {
		mutex_exit(&sc->sc_devs_mtx);
		return (ENODEV);
	}
	volh = dev->dev_handle;
	mutex_exit(&sc->sc_devs_mtx);

	if (mpii_req_cfg_header(sc, MPII_CONFIG_REQ_PAGE_TYPE_RAID_VOL, 0,
	    MPII_CFG_RAID_VOL_ADDR_HANDLE | volh, MPII_PG_POLL, &hdr) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: unable to fetch header for raid "
		    "volume page 0\n", DEVNAME(sc));
		return (EINVAL);
	}

	pagelen = hdr.page_length * 4;
	vpg = malloc(pagelen, M_TEMP, M_NOWAIT | M_ZERO);
	if (vpg == NULL) {
		DNPRINTF(MPII_D_MISC, "%s: unable to allocate space for raid "
		    "volume page 0\n", DEVNAME(sc));
		return (ENOMEM);
	}

	if (mpii_req_cfg_page(sc, MPII_CFG_RAID_VOL_ADDR_HANDLE | volh,
	    MPII_PG_POLL, &hdr, 1, vpg, pagelen) != 0) {
		DNPRINTF(MPII_D_MISC, "%s: unable to fetch raid volume "
		    "page 0\n", DEVNAME(sc));
		free(vpg, M_TEMP);
		return (EINVAL);
	}

	switch (vpg->volume_state) {
	case MPII_CFG_RAID_VOL_0_STATE_ONLINE:
	case MPII_CFG_RAID_VOL_0_STATE_OPTIMAL:
		bv->bv_status = BIOC_SVONLINE;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_DEGRADED:
		if (ISSET(le32toh(vpg->volume_status),
		    MPII_CFG_RAID_VOL_0_STATUS_RESYNC))
			bv->bv_status = BIOC_SVREBUILD;
		else
			bv->bv_status = BIOC_SVDEGRADED;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_FAILED:
		bv->bv_status = BIOC_SVOFFLINE;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_INITIALIZING:
		bv->bv_status = BIOC_SVBUILDING;
		break;
	case MPII_CFG_RAID_VOL_0_STATE_MISSING:
	default:
		bv->bv_status = BIOC_SVINVALID;
		break;
	}

	free(vpg, M_TEMP);
	return (0);
}

int
mpii_create_sensors(struct mpii_softc *sc)
{
	int			i, rv;

	DNPRINTF(MPII_D_MISC, "%s: mpii_create_sensors(%d)\n",
	    DEVNAME(sc), sc->sc_max_volumes);
	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensors = malloc(sizeof(envsys_data_t) * sc->sc_max_volumes,
	    M_DEVBUF, M_NOWAIT | M_ZERO);
	if (sc->sc_sensors == NULL) {
		aprint_error_dev(sc->sc_dev, "can't allocate envsys_data_t\n");
		return (1);
	}

	for (i = 0; i < sc->sc_max_volumes; i++) {
		sc->sc_sensors[i].units = ENVSYS_DRIVE;
		sc->sc_sensors[i].state = ENVSYS_SINVALID;
		sc->sc_sensors[i].value_cur = ENVSYS_DRIVE_EMPTY;
		sc->sc_sensors[i].flags |= ENVSYS_FMONSTCHANGED;

		/* logical drives */  
		snprintf(sc->sc_sensors[i].desc,
		    sizeof(sc->sc_sensors[i].desc), "%s:%d",
		    DEVNAME(sc), i); 
												if ((rv = sysmon_envsys_sensor_attach(sc->sc_sme,      
		    &sc->sc_sensors[i])) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to attach sensor (rv = %d)\n", rv);
			goto out;    
												}
	}
	sc->sc_sme->sme_name =  DEVNAME(sc);
	sc->sc_sme->sme_cookie = sc;  
	sc->sc_sme->sme_refresh = mpii_refresh_sensors;

	rv = sysmon_envsys_register(sc->sc_sme);
	if (rv != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon (rv = %d)\n", rv); 
		goto out;
	}
	return 0;

out:
	free(sc->sc_sensors, M_DEVBUF);
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
	return 1;
}

int
mpii_destroy_sensors(struct mpii_softc *sc)
{
	if (sc->sc_sme == NULL)       
		return 0;
	sysmon_envsys_unregister(sc->sc_sme);
	sc->sc_sme = NULL;
	free(sc->sc_sensors, M_DEVBUF);
	return 0;

}

void
mpii_refresh_sensors(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct mpii_softc	*sc = sme->sme_cookie;
	struct bioc_vol		bv;

	memset(&bv, 0, sizeof(bv));
	bv.bv_volid = edata->sensor;
	if (mpii_bio_volstate(sc, &bv))
		bv.bv_status = BIOC_SVINVALID;
	bio_vol_to_envsys(edata, &bv);
}
#endif /* NBIO > 0 */
