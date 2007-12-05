/*	$NetBSD: arcmsr.c,v 1.4 2007/12/05 18:25:53 xtraeme Exp $ */
/*	$OpenBSD: arc.c,v 1.68 2007/10/27 03:28:27 dlg Exp $ */

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
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

#include "bio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arcmsr.c,v 1.4 2007/12/05 18:25:53 xtraeme Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#if NBIO > 0
#include <sys/ioctl.h>
#include <dev/biovar.h>
#endif

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/sysmon/sysmonvar.h>

#include <sys/bus.h>

#include <uvm/uvm_extern.h>	/* for PAGE_SIZE */

#include <dev/pci/arcmsrvar.h>

/* #define ARC_DEBUG */
#ifdef ARC_DEBUG
#define ARC_D_INIT	(1<<0)
#define ARC_D_RW	(1<<1)
#define ARC_D_DB	(1<<2)

int arcdebug = 0;

#define DPRINTF(p...)		do { if (arcdebug) printf(p); } while (0)
#define DNPRINTF(n, p...)	do { if ((n) & arcdebug) printf(p); } while (0)

#else
#define DPRINTF(p...)		/* p */
#define DNPRINTF(n, p...)	/* n, p */
#endif

/* 
 * the fw header must always equal this.
 */
static struct arc_fw_hdr arc_fw_hdr = { 0x5e, 0x01, 0x61 };

/*
 * autoconf(9) glue.
 */
static int 	arc_match(device_t, struct cfdata *, void *);
static void 	arc_attach(device_t, device_t, void *);
static int 	arc_detach(device_t, int);
static void 	arc_shutdown(void *);
static int 	arc_intr(void *);
static void	arc_minphys(struct buf *);

CFATTACH_DECL(arcmsr, sizeof(struct arc_softc),
	arc_match, arc_attach, arc_detach, NULL);

/*
 * bio(4) and sysmon_envsys(9) glue.
 */
#if NBIO > 0
static int 	arc_bioctl(struct device *, u_long, void *);
static int 	arc_bio_inq(struct arc_softc *, struct bioc_inq *);
static int 	arc_bio_vol(struct arc_softc *, struct bioc_vol *);
static int 	arc_bio_disk(struct arc_softc *, struct bioc_disk *);
static int 	arc_bio_alarm(struct arc_softc *, struct bioc_alarm *);
static int 	arc_bio_alarm_state(struct arc_softc *, struct bioc_alarm *);
static int 	arc_bio_getvol(struct arc_softc *, int,
			       struct arc_fw_volinfo *);
static void 	arc_create_sensors(void *);
static void 	arc_refresh_sensors(struct sysmon_envsys *, envsys_data_t *);
#endif

static int
arc_match(device_t parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_ARECA) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_ARECA_ARC1110:
		case PCI_PRODUCT_ARECA_ARC1120:
		case PCI_PRODUCT_ARECA_ARC1130:
		case PCI_PRODUCT_ARECA_ARC1160:
		case PCI_PRODUCT_ARECA_ARC1170:
		case PCI_PRODUCT_ARECA_ARC1200:
		case PCI_PRODUCT_ARECA_ARC1202:
		case PCI_PRODUCT_ARECA_ARC1210:
		case PCI_PRODUCT_ARECA_ARC1220:
		case PCI_PRODUCT_ARECA_ARC1230:
		case PCI_PRODUCT_ARECA_ARC1260:
		case PCI_PRODUCT_ARECA_ARC1270:
		case PCI_PRODUCT_ARECA_ARC1280:
		case PCI_PRODUCT_ARECA_ARC1380:
		case PCI_PRODUCT_ARECA_ARC1381:
		case PCI_PRODUCT_ARECA_ARC1680:
		case PCI_PRODUCT_ARECA_ARC1681:
			return 1;
		default:
			break;
		}
	}

	return 0;
}

static void
arc_attach(device_t parent, device_t self, void *aux)
{
	struct arc_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	struct scsipi_adapter	*adapt = &sc->sc_adapter;
	struct scsipi_channel	*chan = &sc->sc_chan;

	sc->sc_talking = 0;
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_condvar, "arcdb");

	if (arc_map_pci_resources(sc, pa) != 0) {
		/* error message printed by arc_map_pci_resources */
		return;
	}

	if (arc_query_firmware(sc) != 0) {
		/* error message printed by arc_query_firmware */
		goto unmap_pci;
	}

	if (arc_alloc_ccbs(sc) != 0) {
		/* error message printed by arc_alloc_ccbs */
		goto unmap_pci;
	}

	sc->sc_shutdownhook = shutdownhook_establish(arc_shutdown, sc);
	if (sc->sc_shutdownhook == NULL)
		panic("unable to establish arc powerhook");

	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = self;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = sc->sc_req_count / ARC_MAX_TARGET;
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_minphys = arc_minphys;		
	adapt->adapt_request = arc_scsi_cmd;

	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_nluns = ARC_MAX_LUN;
	chan->chan_ntargets = ARC_MAX_TARGET;
	chan->chan_id = ARC_MAX_TARGET;
	chan->chan_channel = 0;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE;

	(void)config_found(self, &sc->sc_chan, scsiprint);

	/* enable interrupts */
	arc_write(sc, ARC_REG_INTRMASK,
	    ~(ARC_REG_INTRMASK_POSTQUEUE|ARC_REG_INTRSTAT_DOORBELL));

#if NBIO > 0
	if (bio_register(self, arc_bioctl) != 0)
		panic("%s: bioctl registration failed\n", device_xname(self));
	/*
	 * you need to talk to the firmware to get volume info. our firmware
	 * interface relies on being able to sleep, so we need to use a thread
	 * to do the work.
	 */
	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    arc_create_sensors, sc, &sc->sc_lwp, "arcmsr_sensors") != 0)
		panic("%s: unable to create a kernel thread for sensors\n",
		    device_xname(self));
#endif

        return;

unmap_pci:
	arc_unmap_pci_resources(sc);
}

static int
arc_detach(device_t self, int flags)
{
	struct arc_softc		*sc = device_private(self);

	shutdownhook_disestablish(sc->sc_shutdownhook);

	mutex_enter(&sc->sc_mutex);
	if (arc_msg0(sc, ARC_REG_INB_MSG0_STOP_BGRB) != 0)
		aprint_error("%s: timeout waiting to stop bg rebuild\n",
		    device_xname(&sc->sc_dev));

	if (arc_msg0(sc, ARC_REG_INB_MSG0_FLUSH_CACHE) != 0)
		aprint_error("%s: timeout waiting to flush cache\n",
		    device_xname(&sc->sc_dev));
	mutex_exit(&sc->sc_mutex);

	return 0;
}

static void
arc_shutdown(void *xsc)
{
	struct arc_softc		*sc = xsc;

	mutex_enter(&sc->sc_mutex);
	if (arc_msg0(sc, ARC_REG_INB_MSG0_STOP_BGRB) != 0)
		aprint_error("%s: timeout waiting to stop bg rebuild\n",
		    device_xname(&sc->sc_dev));

	if (arc_msg0(sc, ARC_REG_INB_MSG0_FLUSH_CACHE) != 0)
		aprint_error("%s: timeout waiting to flush cache\n",
		    device_xname(&sc->sc_dev));
	mutex_exit(&sc->sc_mutex);
}

static void
arc_minphys(struct buf *bp)
{
	if (bp->b_bcount > MAXPHYS)
		bp->b_bcount = MAXPHYS;
	minphys(bp);
}

static int
arc_intr(void *arg)
{
	struct arc_softc		*sc = arg;
	struct arc_ccb			*ccb = NULL;
	char				*kva = ARC_DMA_KVA(sc->sc_requests);
	struct arc_io_cmd		*cmd;
	uint32_t			reg, intrstat;

	mutex_enter(&sc->sc_mutex);
	intrstat = arc_read(sc, ARC_REG_INTRSTAT);
	if (intrstat == 0x0) {
		mutex_exit(&sc->sc_mutex);
		return 0;
	}

	intrstat &= ARC_REG_INTRSTAT_POSTQUEUE | ARC_REG_INTRSTAT_DOORBELL;
	arc_write(sc, ARC_REG_INTRSTAT, intrstat);

	if (intrstat & ARC_REG_INTRSTAT_DOORBELL) {
		if (sc->sc_talking) {
			/* if an ioctl is talking, wake it up */
			arc_write(sc, ARC_REG_INTRMASK,
			    ~ARC_REG_INTRMASK_POSTQUEUE);
			cv_broadcast(&sc->sc_condvar);
		} else {
			/* otherwise drop it */
			reg = arc_read(sc, ARC_REG_OUTB_DOORBELL);
			arc_write(sc, ARC_REG_OUTB_DOORBELL, reg);
			if (reg & ARC_REG_OUTB_DOORBELL_WRITE_OK)
				arc_write(sc, ARC_REG_INB_DOORBELL,
				    ARC_REG_INB_DOORBELL_READ_OK);
		}
	}
	mutex_exit(&sc->sc_mutex);

	while ((reg = arc_pop(sc)) != 0xffffffff) {
		cmd = (struct arc_io_cmd *)(kva +
		    ((reg << ARC_REG_REPLY_QUEUE_ADDR_SHIFT) -
		    (uint32_t)ARC_DMA_DVA(sc->sc_requests)));
		ccb = &sc->sc_ccbs[htole32(cmd->cmd.context)];

		bus_dmamap_sync(sc->sc_dmat, ARC_DMA_MAP(sc->sc_requests),
		    ccb->ccb_offset, ARC_MAX_IOCMDLEN,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		arc_scsi_cmd_done(sc, ccb, reg);
	}

	return 1;
}

void
arc_scsi_cmd(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct scsipi_periph		*periph;
	struct scsipi_xfer		*xs;
	struct scsipi_adapter		*adapt = chan->chan_adapter;
	struct arc_softc		*sc = device_private(adapt->adapt_dev);
	struct arc_ccb			*ccb;
	struct arc_msg_scsicmd		*cmd;
	uint32_t			reg;
	uint8_t				target;

	switch (req) {
	case ADAPTER_REQ_GROW_RESOURCES:
		/* Not supported. */
		return;
	case ADAPTER_REQ_SET_XFER_MODE:
		/* Not supported. */
		return;
	case ADAPTER_REQ_RUN_XFER:
		break;
	}

	xs = arg;
	periph = xs->xs_periph;
	target = periph->periph_target;

	if (xs->cmdlen > ARC_MSG_CDBLEN) {
		memset(&xs->sense, 0, sizeof(xs->sense));
		xs->sense.scsi_sense.response_code = SSD_RCODE_VALID | 0x70;
		xs->sense.scsi_sense.flags = SKEY_ILLEGAL_REQUEST;
		xs->sense.scsi_sense.asc = 0x20;
		xs->error = XS_SENSE;
		xs->status = SCSI_CHECK;
		scsipi_done(xs);
		return;
	}

	ccb = arc_get_ccb(sc);
	if (ccb == NULL) {
		xs->error = XS_RESOURCE_SHORTAGE;
		scsipi_done(xs);
		return;
	}

	ccb->ccb_xs = xs;

	if (arc_load_xs(ccb) != 0) {
		xs->error = XS_DRIVER_STUFFUP;
		arc_put_ccb(sc, ccb);
		scsipi_done(xs);
		return;
	}

	cmd = &ccb->ccb_cmd->cmd;
	reg = ccb->ccb_cmd_post;

	/* bus is always 0 */
	cmd->target = target;
	cmd->lun = periph->periph_lun;
	cmd->function = 1; /* XXX magic number */

	cmd->cdb_len = xs->cmdlen;
	cmd->sgl_len = ccb->ccb_dmamap->dm_nsegs;
	if (xs->xs_control & XS_CTL_DATA_OUT)
		cmd->flags = ARC_MSG_SCSICMD_FLAG_WRITE;
	if (ccb->ccb_dmamap->dm_nsegs > ARC_SGL_256LEN) {
		cmd->flags |= ARC_MSG_SCSICMD_FLAG_SGL_BSIZE_512;
		reg |= ARC_REG_POST_QUEUE_BIGFRAME;
	}

	cmd->context = htole32(ccb->ccb_id);
	cmd->data_len = htole32(xs->datalen);

	memcpy(cmd->cdb, xs->cmd, xs->cmdlen);

	/* we've built the command, let's put it on the hw */
	bus_dmamap_sync(sc->sc_dmat, ARC_DMA_MAP(sc->sc_requests),
	    ccb->ccb_offset, ARC_MAX_IOCMDLEN,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	arc_push(sc, reg);
	if (xs->xs_control & XS_CTL_POLL) {
		if (arc_complete(sc, ccb, xs->timeout) != 0) {
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
		}
	}
}

int
arc_load_xs(struct arc_ccb *ccb)
{
	struct arc_softc		*sc = ccb->ccb_sc;
	struct scsipi_xfer		*xs = ccb->ccb_xs;
	bus_dmamap_t			dmap = ccb->ccb_dmamap;
	struct arc_sge			*sgl = ccb->ccb_cmd->sgl, *sge;
	uint64_t			addr;
	int				i, error;

	if (xs->datalen == 0)
		return 0;

	error = bus_dmamap_load(sc->sc_dmat, dmap,
	    xs->data, xs->datalen, NULL,
	    (xs->xs_control & XS_CTL_NOSLEEP) ?
	    BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
	if (error != 0) {
		aprint_error("%s: error %d loading dmamap\n",
		    device_xname(&sc->sc_dev), error);
		return 1;
	}

	for (i = 0; i < dmap->dm_nsegs; i++) {
		sge = &sgl[i];

		sge->sg_hdr = htole32(ARC_SGE_64BIT | dmap->dm_segs[i].ds_len);
		addr = dmap->dm_segs[i].ds_addr;
		sge->sg_hi_addr = htole32((uint32_t)(addr >> 32));
		sge->sg_lo_addr = htole32((uint32_t)addr);
	}

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    (xs->xs_control & XS_CTL_DATA_IN) ? BUS_DMASYNC_PREREAD :
	    BUS_DMASYNC_PREWRITE);

	return 0;
}

void
arc_scsi_cmd_done(struct arc_softc *sc, struct arc_ccb *ccb, uint32_t reg)
{
	struct scsipi_xfer		*xs = ccb->ccb_xs;
	struct arc_msg_scsicmd		*cmd;

	if (xs->datalen != 0) {
		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmamap, 0,
		    ccb->ccb_dmamap->dm_mapsize,
		    (xs->xs_control & XS_CTL_DATA_IN) ?
		    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ccb->ccb_dmamap);
	}

	/* timeout_del */
	xs->status |= XS_STS_DONE;

	if (reg & ARC_REG_REPLY_QUEUE_ERR) {
		cmd = &ccb->ccb_cmd->cmd;

		switch (cmd->status) {
		case ARC_MSG_STATUS_SELTIMEOUT:
		case ARC_MSG_STATUS_ABORTED:
		case ARC_MSG_STATUS_INIT_FAIL:
			xs->status = SCSI_OK;
			xs->error = XS_SELTIMEOUT;
			break;

		case SCSI_CHECK:
			memset(&xs->sense, 0, sizeof(xs->sense));
			memcpy(&xs->sense, cmd->sense_data,
			    min(ARC_MSG_SENSELEN, sizeof(xs->sense)));
			xs->sense.scsi_sense.response_code =
			    SSD_RCODE_VALID | 0x70;
			xs->status = SCSI_CHECK;
			xs->error = XS_SENSE;
			xs->resid = 0;
			break;

		default:
			/* unknown device status */
			xs->error = XS_BUSY; /* try again later? */
			xs->status = SCSI_BUSY;
			break;
		}
	} else {
		xs->status = SCSI_OK;
		xs->error = XS_NOERROR;
		xs->resid = 0;
	}

	arc_put_ccb(sc, ccb);
	scsipi_done(xs);
}

int
arc_complete(struct arc_softc *sc, struct arc_ccb *nccb, int timeout)
{
	struct arc_ccb			*ccb = NULL;
	char				*kva = ARC_DMA_KVA(sc->sc_requests);
	struct arc_io_cmd		*cmd;
	uint32_t			reg;

	do {
		reg = arc_pop(sc);
		if (reg == 0xffffffff) {
			if (timeout-- == 0)
				return 1;

			delay(1000);
			continue;
		}

		cmd = (struct arc_io_cmd *)(kva +
		    ((reg << ARC_REG_REPLY_QUEUE_ADDR_SHIFT) -
		    ARC_DMA_DVA(sc->sc_requests)));
		ccb = &sc->sc_ccbs[htole32(cmd->cmd.context)];

		bus_dmamap_sync(sc->sc_dmat, ARC_DMA_MAP(sc->sc_requests),
		    ccb->ccb_offset, ARC_MAX_IOCMDLEN,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		arc_scsi_cmd_done(sc, ccb, reg);
	} while (nccb != ccb);

	return 0;
}

int
arc_map_pci_resources(struct arc_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t			memtype;
	pci_intr_handle_t		ih;

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;

	memtype = pci_mapreg_type(sc->sc_pc, sc->sc_tag, ARC_PCI_BAR);
	if (pci_mapreg_map(pa, ARC_PCI_BAR, memtype, 0, &sc->sc_iot,
	    &sc->sc_ioh, NULL, &sc->sc_ios) != 0) {
		aprint_error(": unable to map system interface register\n");
		return 1;
	}

	if (pci_intr_map(pa, &ih) != 0) {
		aprint_error(": unable to map interrupt\n");
		goto unmap;
	}

	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
	    arc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to map interrupt [2]\n");
		goto unmap;
	}
	aprint_normal(": interrupting at %s\n",
	    pci_intr_string(pa->pa_pc, ih));

	return 0;

unmap:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
	return 1;
}

void
arc_unmap_pci_resources(struct arc_softc *sc)
{
	pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
	sc->sc_ios = 0;
}

int
arc_query_firmware(struct arc_softc *sc)
{
	struct arc_msg_firmware_info	fwinfo;
	char				string[81]; /* sizeof(vendor)*2+1 */

	mutex_enter(&sc->sc_mutex);
	if (arc_wait_eq(sc, ARC_REG_OUTB_ADDR1, ARC_REG_OUTB_ADDR1_FIRMWARE_OK,
	    ARC_REG_OUTB_ADDR1_FIRMWARE_OK) != 0) {
		aprint_debug("%s: timeout waiting for firmware ok\n",
		    device_xname(&sc->sc_dev));
		mutex_enter(&sc->sc_mutex);
		return 1;
	}

	if (arc_msg0(sc, ARC_REG_INB_MSG0_GET_CONFIG) != 0) {
		aprint_debug("%s: timeout waiting for get config\n",
		    device_xname(&sc->sc_dev));
		mutex_exit(&sc->sc_mutex);
		return 1;
	}

	if (arc_msg0(sc, ARC_REG_INB_MSG0_START_BGRB) != 0) {
		aprint_debug("%s: timeout waiting to start bg rebuild\n",
		    device_xname(&sc->sc_dev));
		mutex_exit(&sc->sc_mutex);
		return 1;
	}
	mutex_exit(&sc->sc_mutex);

	arc_read_region(sc, ARC_REG_MSGBUF, &fwinfo, sizeof(fwinfo));

	DNPRINTF(ARC_D_INIT, "%s: signature: 0x%08x\n",
	    device_xname(&sc->sc_dev), htole32(fwinfo.signature));

	if (htole32(fwinfo.signature) != ARC_FWINFO_SIGNATURE_GET_CONFIG) {
		aprint_error("%s: invalid firmware info from iop\n",
		    device_xname(&sc->sc_dev));
		return 1;
	}

	DNPRINTF(ARC_D_INIT, "%s: request_len: %d\n",
	    device_xname(&sc->sc_dev),
	    htole32(fwinfo.request_len));
	DNPRINTF(ARC_D_INIT, "%s: queue_len: %d\n",
	    device_xname(&sc->sc_dev),
	    htole32(fwinfo.queue_len));
	DNPRINTF(ARC_D_INIT, "%s: sdram_size: %d\n",
	    device_xname(&sc->sc_dev),
	    htole32(fwinfo.sdram_size));
	DNPRINTF(ARC_D_INIT, "%s: sata_ports: %d\n",
	    device_xname(&sc->sc_dev),
	    htole32(fwinfo.sata_ports));

	scsipi_strvis(string, 81, fwinfo.vendor, sizeof(fwinfo.vendor));
	DNPRINTF(ARC_D_INIT, "%s: vendor: \"%s\"\n",
	    device_xname(&sc->sc_dev), string);

	scsipi_strvis(string, 17, fwinfo.model, sizeof(fwinfo.model));

	aprint_normal("%s: Areca %s Host Adapter RAID controller\n",
	    device_xname(&sc->sc_dev), string);

	scsipi_strvis(string, 33, fwinfo.fw_version, sizeof(fwinfo.fw_version));
	DNPRINTF(ARC_D_INIT, "%s: version: \"%s\"\n",
	    device_xname(&sc->sc_dev), string);

	if (htole32(fwinfo.request_len) != ARC_MAX_IOCMDLEN) {
		aprint_error("%s: unexpected request frame size (%d != %d)\n",
		    device_xname(&sc->sc_dev),
		    htole32(fwinfo.request_len), ARC_MAX_IOCMDLEN);
		return 1;
	}

	sc->sc_req_count = htole32(fwinfo.queue_len);

	aprint_normal("%s: %d ports, %dMB SDRAM, firmware <%s>\n",
	    device_xname(&sc->sc_dev), htole32(fwinfo.sata_ports),
	    htole32(fwinfo.sdram_size), string);

	return 0;
}

#if NBIO > 0
static int
arc_bioctl(struct device *self, u_long cmd, void *addr)
{
	struct arc_softc *sc = device_private(self);
	int error = 0;

	switch (cmd) {
	case BIOCINQ:
		error = arc_bio_inq(sc, (struct bioc_inq *)addr);
		break;

	case BIOCVOL:
		error = arc_bio_vol(sc, (struct bioc_vol *)addr);
		break;

	case BIOCDISK:
		error = arc_bio_disk(sc, (struct bioc_disk *)addr);
		break;

	case BIOCALARM:
		error = arc_bio_alarm(sc, (struct bioc_alarm *)addr);
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

static int
arc_bio_alarm(struct arc_softc *sc, struct bioc_alarm *ba)
{
	uint8_t	request[2], reply[1];
	size_t	len;
	int	error = 0;

	switch (ba->ba_opcode) {
	case BIOC_SAENABLE:
	case BIOC_SADISABLE:
		request[0] = ARC_FW_SET_ALARM;
		request[1] = (ba->ba_opcode == BIOC_SAENABLE) ?
		    ARC_FW_SET_ALARM_ENABLE : ARC_FW_SET_ALARM_DISABLE;
		len = sizeof(request);

		break;

	case BIOC_SASILENCE:
		request[0] = ARC_FW_MUTE_ALARM;
		len = 1;

		break;

	case BIOC_GASTATUS:
		/* system info is too big/ugly to deal with here */
		return arc_bio_alarm_state(sc, ba);

	default:
		return EOPNOTSUPP;
	}

	arc_lock(sc);
	error = arc_msgbuf(sc, request, len, reply, sizeof(reply));
	arc_unlock(sc);

	if (error != 0)
		return error;

	switch (reply[0]) {
	case ARC_FW_CMD_OK:
		return 0;
	case ARC_FW_CMD_PASS_REQD:
		return EPERM;
	default:
		return EIO;
	}
}

static int
arc_bio_alarm_state(struct arc_softc *sc, struct bioc_alarm *ba)
{
	uint8_t			request = ARC_FW_SYSINFO;
	struct arc_fw_sysinfo	*sysinfo;
	int			error = 0;

	sysinfo = malloc(sizeof(struct arc_fw_sysinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);

	request = ARC_FW_SYSINFO;

	arc_lock(sc);
	error = arc_msgbuf(sc, &request, sizeof(request),
	    sysinfo, sizeof(struct arc_fw_sysinfo));
	arc_unlock(sc);

	if (error != 0)
		goto out;

	ba->ba_status = sysinfo->alarm;

out:
	free(sysinfo, M_DEVBUF);
	return error;
}


static int
arc_bio_inq(struct arc_softc *sc, struct bioc_inq *bi)
{
	uint8_t			request[2];
	struct arc_fw_sysinfo	*sysinfo;
	struct arc_fw_volinfo	*volinfo;
	int			maxvols, nvols = 0, i;
	int			error = 0;

	sysinfo = malloc(sizeof(struct arc_fw_sysinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	volinfo = malloc(sizeof(struct arc_fw_volinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);

	arc_lock(sc);

	request[0] = ARC_FW_SYSINFO;
	error = arc_msgbuf(sc, request, 1, sysinfo,
	    sizeof(struct arc_fw_sysinfo));
	if (error != 0)
		goto out;

	maxvols = sysinfo->max_volume_set;

	request[0] = ARC_FW_VOLINFO;
	for (i = 0; i < maxvols; i++) {
		request[1] = i;
		error = arc_msgbuf(sc, request, sizeof(request), volinfo,
		    sizeof(struct arc_fw_volinfo));
		if (error != 0)
			goto out;

		/*
		 * I can't find an easy way to see if the volume exists or not
		 * except to say that if it has no capacity then it isn't there.
		 * Ignore passthru volumes, bioc_vol doesn't understand them.
		 */
		if ((volinfo->capacity != 0 || volinfo->capacity2 != 0) &&
		    volinfo->raid_level != ARC_FW_VOL_RAIDLEVEL_PASSTHRU)
			nvols++;
	}

	strlcpy(bi->bi_dev, device_xname(&sc->sc_dev), sizeof(bi->bi_dev));
	bi->bi_novol = nvols;
out:
	arc_unlock(sc);
	free(volinfo, M_DEVBUF);
	free(sysinfo, M_DEVBUF);
	return error;
}

static int
arc_bio_getvol(struct arc_softc *sc, int vol, struct arc_fw_volinfo *volinfo)
{
	uint8_t			request[2];
	struct arc_fw_sysinfo	*sysinfo;
	int			error = 0;
	int			maxvols, nvols = 0, i;

	sysinfo = malloc(sizeof(struct arc_fw_sysinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);

	request[0] = ARC_FW_SYSINFO;
	error = arc_msgbuf(sc, request, 1, sysinfo,
	    sizeof(struct arc_fw_sysinfo));
	if (error != 0)
		goto out;

	maxvols = sysinfo->max_volume_set;

	request[0] = ARC_FW_VOLINFO;
	for (i = 0; i < maxvols; i++) {
		request[1] = i;
		error = arc_msgbuf(sc, request, sizeof(request), volinfo,
		    sizeof(struct arc_fw_volinfo));
		if (error != 0)
			goto out;

		if ((volinfo->capacity == 0 && volinfo->capacity2 == 0) ||
		    volinfo->raid_level == ARC_FW_VOL_RAIDLEVEL_PASSTHRU)
			continue;

		if (nvols == vol)
			break;

		nvols++;
	}

	if (nvols != vol ||
	    (volinfo->capacity == 0 && volinfo->capacity2 == 0) ||
	    volinfo->raid_level == ARC_FW_VOL_RAIDLEVEL_PASSTHRU) {
		error = ENODEV;
		goto out;
	}

out:
	free(sysinfo, M_DEVBUF);
	return error;
}

static int
arc_bio_vol(struct arc_softc *sc, struct bioc_vol *bv)
{
	struct arc_fw_volinfo	*volinfo;
	uint64_t		blocks;
	uint32_t		status;
	int			error = 0;

	volinfo = malloc(sizeof(struct arc_fw_volinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);

	arc_lock(sc);
	error = arc_bio_getvol(sc, bv->bv_volid, volinfo);
	arc_unlock(sc);

	if (error != 0)
		goto out;

	bv->bv_percent = -1;
	bv->bv_seconds = 0;

	status = htole32(volinfo->volume_status);
	if (status == 0x0) {
		if (htole32(volinfo->fail_mask) == 0x0)
			bv->bv_status = BIOC_SVONLINE;
		else
			bv->bv_status = BIOC_SVDEGRADED;
	} else if (status & ARC_FW_VOL_STATUS_NEED_REGEN) {
		bv->bv_status = BIOC_SVDEGRADED;
	} else if (status & ARC_FW_VOL_STATUS_FAILED) {
		bv->bv_status = BIOC_SVOFFLINE;
	} else if (status & ARC_FW_VOL_STATUS_INITTING) {
		bv->bv_status = BIOC_SVBUILDING;
		bv->bv_percent = htole32(volinfo->progress) / 10;
	} else if (status & ARC_FW_VOL_STATUS_REBUILDING) {
		bv->bv_status = BIOC_SVREBUILD;
		bv->bv_percent = htole32(volinfo->progress) / 10;
	}

	blocks = (uint64_t)htole32(volinfo->capacity2) << 32;
	blocks += (uint64_t)htole32(volinfo->capacity);
	bv->bv_size = blocks * ARC_BLOCKSIZE; /* XXX */

	switch (volinfo->raid_level) {
	case ARC_FW_VOL_RAIDLEVEL_0:
		bv->bv_level = 0;
		break;
	case ARC_FW_VOL_RAIDLEVEL_1:
		bv->bv_level = 1;
		break;
	case ARC_FW_VOL_RAIDLEVEL_3:
		bv->bv_level = 3;
		break;
	case ARC_FW_VOL_RAIDLEVEL_5:
		bv->bv_level = 5;
		break;
	case ARC_FW_VOL_RAIDLEVEL_6:
		bv->bv_level = 6;
		break;
	case ARC_FW_VOL_RAIDLEVEL_PASSTHRU:
	default:
		bv->bv_level = -1;
		break;
	}

	bv->bv_nodisk = volinfo->member_disks;
	strlcpy(bv->bv_dev, volinfo->set_name, sizeof(bv->bv_dev));

out:
	free(volinfo, M_DEVBUF);
	return error;
}

static int
arc_bio_disk(struct arc_softc *sc, struct bioc_disk *bd)
{
	uint8_t			request[2];
	struct arc_fw_volinfo	*volinfo;
	struct arc_fw_raidinfo	*raidinfo;
	struct arc_fw_diskinfo	*diskinfo;
	int			error = 0;
	uint64_t		blocks;
	char			model[81];
	char			serial[41];
	char			rev[17];

	volinfo = malloc(sizeof(struct arc_fw_volinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	raidinfo = malloc(sizeof(struct arc_fw_raidinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);
	diskinfo = malloc(sizeof(struct arc_fw_diskinfo), M_DEVBUF,
	    M_WAITOK|M_ZERO);

	arc_lock(sc);

	error = arc_bio_getvol(sc, bd->bd_volid, volinfo);
	if (error != 0)
		goto out;

	request[0] = ARC_FW_RAIDINFO;
	request[1] = volinfo->raid_set_number;
	error = arc_msgbuf(sc, request, sizeof(request), raidinfo,
	    sizeof(struct arc_fw_raidinfo));
	if (error != 0)
		goto out;

	if (bd->bd_diskid > raidinfo->member_devices) {
		error = ENODEV;
		goto out;
	}

	if (raidinfo->device_array[bd->bd_diskid] == 0xff) {
		/*
		 * the disk doesn't exist anymore. bio is too dumb to be
		 * able to display that, so put it on another bus
		 */
		bd->bd_channel = 1;
		bd->bd_target = 0;
		bd->bd_lun = 0;
		bd->bd_status = BIOC_SDOFFLINE;
		strlcpy(bd->bd_vendor, "disk missing", sizeof(bd->bd_vendor));
		goto out;
	}

	request[0] = ARC_FW_DISKINFO;
	request[1] = raidinfo->device_array[bd->bd_diskid];
	error = arc_msgbuf(sc, request, sizeof(request), diskinfo,
	    sizeof(struct arc_fw_diskinfo));
	if (error != 0)
		goto out;

#if 0
	bd->bd_channel = diskinfo->scsi_attr.channel;
	bd->bd_target = diskinfo->scsi_attr.target;
	bd->bd_lun = diskinfo->scsi_attr.lun;
#endif
	/*
	 * the firwmare doesnt seem to fill scsi_attr in, so fake it with
	 * the diskid.
	 */
	bd->bd_channel = 0;
	bd->bd_target = raidinfo->device_array[bd->bd_diskid];
	bd->bd_lun = 0;

	bd->bd_status = BIOC_SDONLINE;
	blocks = (uint64_t)htole32(diskinfo->capacity2) << 32;
	blocks += (uint64_t)htole32(diskinfo->capacity);
	bd->bd_size = blocks * ARC_BLOCKSIZE; /* XXX */

	scsipi_strvis(model, 81, diskinfo->model, sizeof(diskinfo->model));
	scsipi_strvis(serial, 41, diskinfo->serial, sizeof(diskinfo->serial));
	scsipi_strvis(rev, 17, diskinfo->firmware_rev,
	    sizeof(diskinfo->firmware_rev));

	snprintf(bd->bd_vendor, sizeof(bd->bd_vendor), "%s %s", model, rev);
	strlcpy(bd->bd_serial, serial, sizeof(bd->bd_serial));

out:
	arc_unlock(sc);
	free(diskinfo, M_DEVBUF);
	free(raidinfo, M_DEVBUF);
	free(volinfo, M_DEVBUF);
	return error;
}
#endif /* NBIO > 0 */

uint8_t
arc_msg_cksum(void *cmd, uint16_t len)
{
	uint8_t	*buf = cmd;
	uint8_t	cksum;
	int	i;

	cksum = (uint8_t)(len >> 8) + (uint8_t)len;
	for (i = 0; i < len; i++)
		cksum += buf[i];

	return cksum;
}


int
arc_msgbuf(struct arc_softc *sc, void *wptr, size_t wbuflen, void *rptr,
	   size_t rbuflen)
{
	uint8_t			rwbuf[ARC_REG_IOC_RWBUF_MAXLEN];
	uint8_t			*wbuf, *rbuf;
	int			wlen, wdone = 0, rlen, rdone = 0;
	struct arc_fw_bufhdr	*bufhdr;
	uint32_t		reg, rwlen;
	int			error = 0;
#ifdef ARC_DEBUG
	int			i;
#endif

	DNPRINTF(ARC_D_DB, "%s: arc_msgbuf wbuflen: %d rbuflen: %d\n",
	    device_xname(&sc->sc_dev), wbuflen, rbuflen);

	if (arc_read(sc, ARC_REG_OUTB_DOORBELL) != 0)
		return EBUSY;

	wlen = sizeof(struct arc_fw_bufhdr) + wbuflen + 1; /* 1 for cksum */
	wbuf = malloc(wlen, M_TEMP, M_WAITOK);

	rlen = sizeof(struct arc_fw_bufhdr) + rbuflen + 1; /* 1 for cksum */
	rbuf = malloc(rlen, M_TEMP, M_WAITOK);

	DNPRINTF(ARC_D_DB, "%s: arc_msgbuf wlen: %d rlen: %d\n",
	    device_xname(&sc->sc_dev), wlen, rlen);

	bufhdr = (struct arc_fw_bufhdr *)wbuf;
	bufhdr->hdr = arc_fw_hdr;
	bufhdr->len = htole16(wbuflen);
	memcpy(wbuf + sizeof(struct arc_fw_bufhdr), wptr, wbuflen);
	wbuf[wlen - 1] = arc_msg_cksum(wptr, wbuflen);

	reg = ARC_REG_OUTB_DOORBELL_READ_OK;

	do {
		if ((reg & ARC_REG_OUTB_DOORBELL_READ_OK) && wdone < wlen) {
			memset(rwbuf, 0, sizeof(rwbuf));
			rwlen = (wlen - wdone) % sizeof(rwbuf);
			memcpy(rwbuf, &wbuf[wdone], rwlen);

#ifdef ARC_DEBUG
			if (arcdebug & ARC_D_DB) {
				printf("%s: write %d:",
				    device_xname(&sc->sc_dev), rwlen);
				for (i = 0; i < rwlen; i++)
					printf(" 0x%02x", rwbuf[i]);
				printf("\n");
			}
#endif

			/* copy the chunk to the hw */
			arc_write(sc, ARC_REG_IOC_WBUF_LEN, rwlen);
			arc_write_region(sc, ARC_REG_IOC_WBUF, rwbuf,
			    sizeof(rwbuf));

			/* say we have a buffer for the hw */
			arc_write(sc, ARC_REG_INB_DOORBELL,
			    ARC_REG_INB_DOORBELL_WRITE_OK);

			wdone += rwlen;
		}

		while ((reg = arc_read(sc, ARC_REG_OUTB_DOORBELL)) == 0)
			arc_wait(sc);
		arc_write(sc, ARC_REG_OUTB_DOORBELL, reg);

		DNPRINTF(ARC_D_DB, "%s: reg: 0x%08x\n",
		    device_xname(&sc->sc_dev), reg);

		if ((reg & ARC_REG_OUTB_DOORBELL_WRITE_OK) && rdone < rlen) {
			rwlen = arc_read(sc, ARC_REG_IOC_RBUF_LEN);
			if (rwlen > sizeof(rwbuf)) {
				DNPRINTF(ARC_D_DB, "%s:  rwlen too big\n",
				    device_xname(&sc->sc_dev));
				error = EIO;
				goto out;
			}

			arc_read_region(sc, ARC_REG_IOC_RBUF, rwbuf,
			    sizeof(rwbuf));

			arc_write(sc, ARC_REG_INB_DOORBELL,
			    ARC_REG_INB_DOORBELL_READ_OK);

#ifdef ARC_DEBUG
			printf("%s:  len: %d+%d=%d/%d\n",
			    device_xname(&sc->sc_dev),
			    rwlen, rdone, rwlen + rdone, rlen);
			if (arcdebug & ARC_D_DB) {
				printf("%s: read:",
				    device_xname(&sc->sc_dev));
				for (i = 0; i < rwlen; i++)
					printf(" 0x%02x", rwbuf[i]);
				printf("\n");
			}
#endif

			if ((rdone + rwlen) > rlen) {
				DNPRINTF(ARC_D_DB, "%s:  rwbuf too big\n",
				    device_xname(&sc->sc_dev));
				error = EIO;
				goto out;
			}

			memcpy(&rbuf[rdone], rwbuf, rwlen);
			rdone += rwlen;
		}
	} while (rdone != rlen);

	bufhdr = (struct arc_fw_bufhdr *)rbuf;
	if (memcmp(&bufhdr->hdr, &arc_fw_hdr, sizeof(bufhdr->hdr)) != 0 ||
	    bufhdr->len != htole16(rbuflen)) {
		DNPRINTF(ARC_D_DB, "%s:  rbuf hdr is wrong\n",
		    device_xname(&sc->sc_dev));
		error = EIO;
		goto out;
	}

	memcpy(rptr, rbuf + sizeof(struct arc_fw_bufhdr), rbuflen);

	if (rbuf[rlen - 1] != arc_msg_cksum(rptr, rbuflen)) {
		DNPRINTF(ARC_D_DB, "%s:  invalid cksum\n",
		    device_xname(&sc->sc_dev));
		error = EIO;
		goto out;
	}

out:
	free(wbuf, M_TEMP);
	free(rbuf, M_TEMP);

	return error;
}

void
arc_lock(struct arc_softc *sc)
{	
	mutex_enter(&sc->sc_mutex);
	arc_write(sc, ARC_REG_INTRMASK, ~ARC_REG_INTRMASK_POSTQUEUE);
	sc->sc_talking = 1;
}

void
arc_unlock(struct arc_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_mutex));

	sc->sc_talking = 0;
	arc_write(sc, ARC_REG_INTRMASK,
	    ~(ARC_REG_INTRMASK_POSTQUEUE|ARC_REG_INTRMASK_DOORBELL));
	mutex_exit(&sc->sc_mutex);
}

void
arc_wait(struct arc_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_mutex));

	arc_write(sc, ARC_REG_INTRMASK,
	    ~(ARC_REG_INTRMASK_POSTQUEUE|ARC_REG_INTRMASK_DOORBELL));
	if (cv_timedwait_sig(&sc->sc_condvar, &sc->sc_mutex, hz) ==
	    EWOULDBLOCK)
		arc_write(sc, ARC_REG_INTRMASK, ~ARC_REG_INTRMASK_POSTQUEUE);
}

#if NBIO > 0
static void
arc_create_sensors(void *arg)
{
	struct arc_softc	*sc = arg;
	struct bioc_inq		bi;
	struct bioc_vol		bv;
	int			i;

	memset(&bi, 0, sizeof(bi));
	if (arc_bio_inq(sc, &bi) != 0) {
		aprint_error("%s: unable to query firmware for sensor info\n",
		    device_xname(&sc->sc_dev));
		kthread_exit(0);
	}

	sc->sc_nsensors = bi.bi_novol;
	/*
	 * There's no point to continue if there are no drives connected...
	 */
	if (!sc->sc_nsensors)
		kthread_exit(0);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensors = malloc(sizeof(envsys_data_t) * sc->sc_nsensors,
	    M_DEVBUF, M_WAITOK | M_ZERO);

	for (i = 0; i < sc->sc_nsensors; i++) {
		memset(&bv, 0, sizeof(bv));
		bv.bv_volid = i;
		if (arc_bio_vol(sc, &bv) != 0)
			goto bad;

		sc->sc_sensors[i].units = ENVSYS_DRIVE;
		sc->sc_sensors[i].monitor = true;
		sc->sc_sensors[i].flags = ENVSYS_FMONSTCHANGED;
		strlcpy(sc->sc_sensors[i].desc, bv.bv_dev,
		    sizeof(sc->sc_sensors[i].desc));
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensors[i]))
			goto bad;
	}

	sc->sc_sme->sme_name = device_xname(&sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = arc_refresh_sensors;
	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_debug("%s: unable to register with sysmon\n",
		    device_xname(&sc->sc_dev));
		goto bad;
	}
	kthread_exit(0);

bad:
	free(sc->sc_sensors, M_DEVBUF);
	sysmon_envsys_destroy(sc->sc_sme);
	kthread_exit(0);
}

static void
arc_refresh_sensors(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct arc_softc	*sc = sme->sme_cookie;
	struct bioc_vol		bv;

	memset(&bv, 0, sizeof(bv));
	bv.bv_volid = edata->sensor;

	if (arc_bio_vol(sc, &bv)) {
		edata->value_cur = ENVSYS_DRIVE_EMPTY;
		edata->state = ENVSYS_SINVALID;
		return;
	}

	switch (bv.bv_status) {
	case BIOC_SVOFFLINE:
		edata->value_cur = ENVSYS_DRIVE_FAIL;
		edata->state = ENVSYS_SCRITICAL;
		break;
	case BIOC_SVDEGRADED:
		edata->value_cur = ENVSYS_DRIVE_PFAIL;
		edata->state = ENVSYS_SCRITICAL;
		break;
	case BIOC_SVBUILDING:
		edata->value_cur = ENVSYS_DRIVE_REBUILD;
		edata->state = ENVSYS_SVALID;
		break;
	case BIOC_SVSCRUB:
	case BIOC_SVONLINE:
		edata->value_cur = ENVSYS_DRIVE_ONLINE;
		edata->state = ENVSYS_SVALID;
		break;
	case BIOC_SVINVALID:
		/* FALLTRHOUGH */
	default:
		edata->value_cur = ENVSYS_DRIVE_EMPTY; /* unknown state */
		edata->state = ENVSYS_SINVALID;
		break;
	}
}
#endif /* NBIO > 0 */

uint32_t
arc_read(struct arc_softc *sc, bus_size_t r)
{
	uint32_t			v;

	KASSERT(mutex_owned(&sc->sc_mutex));

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, r);

	DNPRINTF(ARC_D_RW, "%s: arc_read 0x%lx 0x%08x\n",
	    device_xname(&sc->sc_dev), r, v);

	return v;
}

void
arc_read_region(struct arc_softc *sc, bus_size_t r, void *buf, size_t len)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, len,
	    BUS_SPACE_BARRIER_READ);
	bus_space_read_region_4(sc->sc_iot, sc->sc_ioh, r,
	    (uint32_t *)buf, len >> 2);
}

void
arc_write(struct arc_softc *sc, bus_size_t r, uint32_t v)
{
	KASSERT(mutex_owned(&sc->sc_mutex));

	DNPRINTF(ARC_D_RW, "%s: arc_write 0x%lx 0x%08x\n",
	    device_xname(&sc->sc_dev), r, v);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

void
arc_write_region(struct arc_softc *sc, bus_size_t r, void *buf, size_t len)
{
	bus_space_write_region_4(sc->sc_iot, sc->sc_ioh, r,
	    (const uint32_t *)buf, len >> 2);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, len,
	    BUS_SPACE_BARRIER_WRITE);
}

int
arc_wait_eq(struct arc_softc *sc, bus_size_t r, uint32_t mask,
	    uint32_t target)
{
	int i;

	KASSERT(mutex_owned(&sc->sc_mutex));

	DNPRINTF(ARC_D_RW, "%s: arc_wait_eq 0x%lx 0x%08x 0x%08x\n",
	    device_xname(&sc->sc_dev), r, mask, target);

	for (i = 0; i < 10000; i++) {
		if ((arc_read(sc, r) & mask) == target)
			return 0;
		delay(1000);
	}

	return 1;
}

int
arc_wait_ne(struct arc_softc *sc, bus_size_t r, uint32_t mask,
	    uint32_t target)
{
	int i;

	DNPRINTF(ARC_D_RW, "%s: arc_wait_ne 0x%lx 0x%08x 0x%08x\n",
	    device_xname(&sc->sc_dev), r, mask, target);

	for (i = 0; i < 10000; i++) {
		if ((arc_read(sc, r) & mask) != target)
			return 0;
		delay(1000);
	}

	return 1;
}

int
arc_msg0(struct arc_softc *sc, uint32_t m)
{
	KASSERT(mutex_owned(&sc->sc_mutex));

	/* post message */
	arc_write(sc, ARC_REG_INB_MSG0, m);
	/* wait for the fw to do it */
	if (arc_wait_eq(sc, ARC_REG_INTRSTAT, ARC_REG_INTRSTAT_MSG0,
	    ARC_REG_INTRSTAT_MSG0) != 0)
		return 1;

	/* ack it */
	arc_write(sc, ARC_REG_INTRSTAT, ARC_REG_INTRSTAT_MSG0);

	return 0;
}

struct arc_dmamem *
arc_dmamem_alloc(struct arc_softc *sc, size_t size)
{
	struct arc_dmamem		*adm;
	int				nsegs;

	adm = malloc(sizeof(*adm), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (adm == NULL)
		return NULL;

	adm->adm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &adm->adm_map) != 0)
		goto admfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &adm->adm_seg,
	    1, &nsegs, BUS_DMA_NOWAIT) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &adm->adm_seg, nsegs, size,
	    &adm->adm_kva, BUS_DMA_NOWAIT|BUS_DMA_COHERENT) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, adm->adm_map, adm->adm_kva, size,
	    NULL, BUS_DMA_NOWAIT) != 0)
		goto unmap;

	memset(adm->adm_kva, 0, size);

	return adm;

unmap:
	bus_dmamem_unmap(sc->sc_dmat, adm->adm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &adm->adm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, adm->adm_map);
admfree:
	free(adm, M_DEVBUF);

	return NULL;
}

void
arc_dmamem_free(struct arc_softc *sc, struct arc_dmamem *adm)
{
	bus_dmamap_unload(sc->sc_dmat, adm->adm_map);
	bus_dmamem_unmap(sc->sc_dmat, adm->adm_kva, adm->adm_size);
	bus_dmamem_free(sc->sc_dmat, &adm->adm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, adm->adm_map);
	free(adm, M_DEVBUF);
}

int
arc_alloc_ccbs(struct arc_softc *sc)
{
	struct arc_ccb		*ccb;
	uint8_t			*cmd;
	int			i;

	TAILQ_INIT(&sc->sc_ccb_free);

	sc->sc_ccbs = malloc(sizeof(struct arc_ccb) * sc->sc_req_count,
	    M_DEVBUF, M_WAITOK|M_ZERO);

	sc->sc_requests = arc_dmamem_alloc(sc,
	    ARC_MAX_IOCMDLEN * sc->sc_req_count);
	if (sc->sc_requests == NULL) {
		aprint_error("%s: unable to allocate ccb dmamem\n",
		    device_xname(&sc->sc_dev));
		goto free_ccbs;
	}
	cmd = ARC_DMA_KVA(sc->sc_requests);

	for (i = 0; i < sc->sc_req_count; i++) {
		ccb = &sc->sc_ccbs[i];

		if (bus_dmamap_create(sc->sc_dmat, MAXPHYS, ARC_SGL_MAXLEN,
		    MAXPHYS, 0, 0, &ccb->ccb_dmamap) != 0) {
			aprint_error("%s: unable to create dmamap for ccb %d\n",
			    device_xname(&sc->sc_dev), i);
			goto free_maps;
		}

		ccb->ccb_sc = sc;
		ccb->ccb_id = i;
		ccb->ccb_offset = ARC_MAX_IOCMDLEN * i;

		ccb->ccb_cmd = (struct arc_io_cmd *)&cmd[ccb->ccb_offset];
		ccb->ccb_cmd_post = (ARC_DMA_DVA(sc->sc_requests) +
		    ccb->ccb_offset) >> ARC_REG_POST_QUEUE_ADDR_SHIFT;

		arc_put_ccb(sc, ccb);
	}

	return 0;

free_maps:
	while ((ccb = arc_get_ccb(sc)) != NULL)
	    bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmamap);
	arc_dmamem_free(sc, sc->sc_requests);

free_ccbs:
	free(sc->sc_ccbs, M_DEVBUF);

	return 1;
}

struct arc_ccb *
arc_get_ccb(struct arc_softc *sc)
{
	struct arc_ccb			*ccb;

	mutex_enter(&sc->sc_mutex);
	ccb = TAILQ_FIRST(&sc->sc_ccb_free);
	if (ccb != NULL)
		TAILQ_REMOVE(&sc->sc_ccb_free, ccb, ccb_link);
	mutex_exit(&sc->sc_mutex);
	
	return ccb;
}

void
arc_put_ccb(struct arc_softc *sc, struct arc_ccb *ccb)
{
	mutex_enter(&sc->sc_mutex);
	ccb->ccb_xs = NULL;
	memset(ccb->ccb_cmd, 0, ARC_MAX_IOCMDLEN);
	TAILQ_INSERT_TAIL(&sc->sc_ccb_free, ccb, ccb_link);
	mutex_exit(&sc->sc_mutex);
}
