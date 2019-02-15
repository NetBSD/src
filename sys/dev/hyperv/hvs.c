/*	$NetBSD: hvs.c,v 1.1 2019/02/15 08:54:01 nonaka Exp $	*/
/*	$OpenBSD: hvs.c,v 1.17 2017/08/10 17:22:48 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2012,2016 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * Copyright (c) 2017 Mike Belopuhov <mike@esdenera.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
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
 * The OpenBSD port was done under funding by Esdenera Networks GmbH.
 */

/* #define HVS_DEBUG_IO */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hvs.c,v 1.1 2019/02/15 08:54:01 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <uvm/uvm_extern.h>

#include <dev/hyperv/vmbusvar.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsipi_all.h>

#define HVS_PROTO_VERSION_WIN6		0x200
#define HVS_PROTO_VERSION_WIN7		0x402
#define HVS_PROTO_VERSION_WIN8		0x501
#define HVS_PROTO_VERSION_WIN8_1	0x600
#define HVS_PROTO_VERSION_WIN10		0x602

#define HVS_MSG_IODONE			0x01
#define HVS_MSG_DEVGONE			0x02
#define HVS_MSG_ENUMERATE		0x0b

#define HVS_REQ_SCSIIO			0x03
#define HVS_REQ_STARTINIT		0x07
#define HVS_REQ_FINISHINIT		0x08
#define HVS_REQ_QUERYPROTO		0x09
#define HVS_REQ_QUERYPROPS		0x0a
#define HVS_REQ_CREATEMULTICHANNELS	0x0d

struct hvs_cmd_hdr {
	uint32_t		hdr_op;
	uint32_t		hdr_flags;
	uint32_t		hdr_status;
#define cmd_op			cmd_hdr.hdr_op
#define cmd_flags		cmd_hdr.hdr_flags
#define cmd_status		cmd_hdr.hdr_status
} __packed;

/* Negotiate version */
struct hvs_cmd_ver {
	struct hvs_cmd_hdr	cmd_hdr;
	uint16_t		cmd_ver;
	uint16_t		cmd_rev;
} __packed;

/* Query channel properties */
struct hvs_chp {
	uint16_t		chp_proto;
	uint8_t			chp_path;
	uint8_t			chp_target;
	uint16_t		chp_maxchan;
	uint16_t		chp_port;
	uint32_t		chp_chflags;
#define CHP_CHFLAGS_MULTI_CHANNEL	0x1
	uint32_t		chp_maxfer;
	uint64_t		chp_chanid;
} __packed;

struct hvs_cmd_chp {
	struct hvs_cmd_hdr	cmd_hdr;
	struct hvs_chp		cmd_chp;
} __packed;

#define SENSE_DATA_LEN_WIN7		18
#define SENSE_DATA_LEN			20
#define MAX_SRB_DATA			20

/* SCSI Request Block */
struct hvs_srb {
	uint16_t		srb_reqlen;
	uint8_t			srb_iostatus;
	uint8_t			srb_scsistatus;

	uint8_t			srb_initiator;
	uint8_t			srb_bus;
	uint8_t			srb_target;
	uint8_t			srb_lun;

	uint8_t			srb_cdblen;
	uint8_t			srb_senselen;
	uint8_t			srb_direction;
	uint8_t			_reserved;

	uint32_t		srb_datalen;
	uint8_t			srb_data[MAX_SRB_DATA];
} __packed;

#define SRB_DATA_WRITE			0
#define SRB_DATA_READ			1
#define SRB_DATA_NONE			2

#define SRB_STATUS_PENDING		0x00
#define SRB_STATUS_SUCCESS		0x01
#define SRB_STATUS_ABORTED		0x02
#define SRB_STATUS_ERROR		0x04
#define SRB_STATUS_INVALID_LUN		0x20
#define SRB_STATUS_QUEUE_FROZEN		0x40
#define SRB_STATUS_AUTOSENSE_VALID	0x80

#define SRB_FLAGS_QUEUE_ACTION_ENABLE		0x00000002
#define SRB_FLAGS_DISABLE_DISCONNECT		0x00000004
#define SRB_FLAGS_DISABLE_SYNCH_TRANSFER	0x00000008
#define SRB_FLAGS_BYPASS_FROZEN_QUEUE		0x00000010
#define SRB_FLAGS_DISABLE_AUTOSENSE		0x00000020
#define SRB_FLAGS_DATA_IN			0x00000040
#define SRB_FLAGS_DATA_OUT			0x00000080
#define SRB_FLAGS_NO_DATA_TRANSFER		0x00000000
#define SRB_FLAGS_NO_QUEUE_FREEZE		0x00000100
#define SRB_FLAGS_ADAPTER_CACHE_ENABLE		0x00000200
#define SRB_FLAGS_FREE_SENSE_BUFFER		0x00000400

struct hvs_cmd_io {
	struct hvs_cmd_hdr	cmd_hdr;
	struct hvs_srb		cmd_srb;
	/* Win8 extensions */
	uint16_t		_reserved;
	uint8_t			cmd_qtag;
	uint8_t			cmd_qaction;
	uint32_t		cmd_srbflags;
	uint32_t		cmd_timeout;
	uint32_t		cmd_qsortkey;
} __packed;

#define HVS_CMD_SIZE			64

union hvs_cmd {
	struct hvs_cmd_hdr	cmd_hdr;
	struct hvs_cmd_ver	ver;
	struct hvs_cmd_chp	chp;
	struct hvs_cmd_io	io;
	uint16_t		multi_chans_cnt;
	uint8_t			pad[HVS_CMD_SIZE];
} __packed;

#define HVS_RING_SIZE			(20 * PAGE_SIZE)
#define HVS_MAX_CCB			128
#define HVS_MAX_SGE			(MAXPHYS / PAGE_SIZE + 1)

struct hvs_softc;

struct hvs_ccb {
	struct scsipi_xfer	*ccb_xfer;  /* associated transfer */
	union hvs_cmd		*ccb_cmd;   /* associated command */
	union hvs_cmd		ccb_rsp;    /* response */
	bus_dmamap_t		ccb_dmap;   /* transfer map */
	uint64_t		ccb_rid;    /* request id */
	struct vmbus_gpa_range	*ccb_sgl;
	int			ccb_nsge;
	void			(*ccb_done)(struct hvs_ccb *);
	void			*ccb_cookie;
	SIMPLEQ_ENTRY(hvs_ccb)	ccb_link;
};
SIMPLEQ_HEAD(hvs_ccb_queue, hvs_ccb);

struct hvs_config;

struct hvs_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;

	struct vmbus_channel	*sc_chan;

	const struct hvs_config	*sc_config;

	struct hvs_chp		sc_props;

	/* CCBs */
	int			sc_nccb;
	struct hvs_ccb		*sc_ccbs;
	struct hvs_ccb_queue	sc_ccb_fq;  /* free queue */
	kmutex_t		sc_ccb_fqlck;

	int			sc_bus;

	struct scsipi_adapter	sc_adapter;
	struct scsipi_channel	sc_channel;
	device_t		sc_scsibus;
#if notyet /* XXX subchannel */
	u_int			sc_nchan;
	struct vmbus_channel	*sc_sel_chan[MAXCPUS];
#endif
};

static int	hvs_match(device_t, cfdata_t, void *);
static void	hvs_attach(device_t, device_t, void *);
static int	hvs_detach(device_t, int);

CFATTACH_DECL_NEW(hvs, sizeof(struct hvs_softc),
    hvs_match, hvs_attach, hvs_detach, NULL);

static void	hvs_scsipi_request(struct scsipi_channel *,
		    scsipi_adapter_req_t, void *);
static void	hvs_scsi_cmd_done(struct hvs_ccb *);
static int	hvs_start(struct hvs_softc *, struct vmbus_channel *,
		    struct hvs_ccb *);
static int	hvs_poll(struct hvs_softc *, struct vmbus_channel *,
		    struct hvs_ccb *);
static void	hvs_poll_done(struct hvs_ccb *);
static void	hvs_intr(void *);
static void	hvs_scsi_probe(void *arg);
static void	hvs_scsi_done(struct scsipi_xfer *, int);

static int	hvs_connect(struct hvs_softc *);
static void	hvs_empty_done(struct hvs_ccb *);

static int	hvs_alloc_ccbs(struct hvs_softc *);
static void	hvs_free_ccbs(struct hvs_softc *);
static struct hvs_ccb *
		hvs_get_ccb(struct hvs_softc *);
static void	hvs_put_ccb(struct hvs_softc *, struct hvs_ccb *);

static const struct hvs_config {
	uint32_t	proto_version;
	uint16_t	reqlen;
	uint8_t		senselen;
	bool		fixup_wrong_response;
	bool		upgrade_spc2_to_spc3;
	bool		use_win8ext_flags;
} hvs_config_list[] = {
	{
		.proto_version = HVS_PROTO_VERSION_WIN10,
		.reqlen = sizeof(struct hvs_cmd_io),
		.senselen = SENSE_DATA_LEN,
		.fixup_wrong_response = false,
		.upgrade_spc2_to_spc3 = false,
		.use_win8ext_flags = true,
	},
	{
		.proto_version = HVS_PROTO_VERSION_WIN8_1,
		.reqlen = sizeof(struct hvs_cmd_io),
		.senselen = SENSE_DATA_LEN,
		.fixup_wrong_response = true,
		.upgrade_spc2_to_spc3 = true,
		.use_win8ext_flags = true,
	},
	{
		.proto_version = HVS_PROTO_VERSION_WIN8,
		.reqlen = sizeof(struct hvs_cmd_io),
		.senselen = SENSE_DATA_LEN,
		.fixup_wrong_response = true,
		.upgrade_spc2_to_spc3 = true,
		.use_win8ext_flags = true,
	},
	{
		.proto_version = HVS_PROTO_VERSION_WIN7,
		.reqlen = offsetof(struct hvs_cmd_io, _reserved),
		.senselen = SENSE_DATA_LEN_WIN7,
		.fixup_wrong_response = true,
		.upgrade_spc2_to_spc3 = false,
		.use_win8ext_flags = false,
	},
	{
		.proto_version = HVS_PROTO_VERSION_WIN6,
		.reqlen = offsetof(struct hvs_cmd_io, _reserved),
		.senselen = SENSE_DATA_LEN_WIN7,
		.fixup_wrong_response = false,
		.upgrade_spc2_to_spc3 = false,
		.use_win8ext_flags = false,
	},
};

#if notyet /* XXX subchannel */
static int hvs_chan_cnt;
#endif

static int
hvs_match(device_t parent, cfdata_t cf, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	if (memcmp(aa->aa_type, &hyperv_guid_ide, sizeof(*aa->aa_type)) != 0 &&
	    memcmp(aa->aa_type, &hyperv_guid_scsi, sizeof(*aa->aa_type)) != 0)
		return 0;
	return 1;
}

static void
hvs_attach(device_t parent, device_t self, void *aux)
{
	extern struct cfdata cfdata[];
	struct hvs_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;
	const char *bus;
	bool is_scsi;

	sc->sc_dev = self;
	sc->sc_chan = aa->aa_chan;
	sc->sc_dmat = sc->sc_chan->ch_sc->sc_dmat;
#if notyet /* XXX subchannel */
	sc->sc_nchan = 1;
	sc->sc_sel_chan[0] = sc->sc_chan;
#endif

	if (memcmp(aa->aa_type, &hyperv_guid_scsi, sizeof(*aa->aa_type)) == 0) {
		is_scsi = true;
		bus = "SCSI";
	} else {
		is_scsi = false;
		bus = "IDE";
	}

	aprint_naive("\n");
	aprint_normal(": Hyper-V StorVSC %s\n", bus);

	if (vmbus_channel_setdeferred(sc->sc_chan, device_xname(self))) {
		aprint_error_dev(self,
		    "failed to create the interrupt thread\n");
		return;
	}

	if (vmbus_channel_open(sc->sc_chan, HVS_RING_SIZE, &sc->sc_props,
	    sizeof(sc->sc_props), hvs_intr, sc)) {
		aprint_error_dev(self, "failed to open channel\n");
		return;
	}

	if (hvs_alloc_ccbs(sc))
		return;

	if (hvs_connect(sc))
		return;

	aprint_normal_dev(self, "protocol %u.%u\n",
	    (sc->sc_config->proto_version >> 8) & 0xff,
	    sc->sc_config->proto_version & 0xff);

	adapt = &sc->sc_adapter;
	adapt->adapt_dev = self;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = sc->sc_nccb;
	adapt->adapt_max_periph = adapt->adapt_openings;
	adapt->adapt_request = hvs_scsipi_request;
	adapt->adapt_minphys = minphys;
	adapt->adapt_flags = SCSIPI_ADAPT_MPSAFE;

	chan = &sc->sc_channel;
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;	/* XXX IDE/ATAPI */
	chan->chan_channel = 0;
	chan->chan_ntargets = 2;
	chan->chan_nluns = is_scsi ? 64 : 1;
	chan->chan_id = 0;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE;
	chan->chan_defquirks |= PQUIRK_ONLYBIG;

	sc->sc_scsibus = config_found(self, &sc->sc_channel, scsiprint);

	/*
	 * If the driver has successfully attached to an IDE device,
	 * we need to make sure that the same disk is not available to
	 * the system via pciide(4) or piixide(4) causing DUID conflicts
	 * and preventing system from booting.
	 */
	if (!is_scsi && sc->sc_scsibus != NULL) {
		static const char *disable_devices[] = {
			"wd",
		};
		size_t j;

		for (j = 0; j < __arraycount(disable_devices); j++) {
			const char *dev = disable_devices[j];
			size_t len = strlen(dev);
			int devno;

			for (devno = 0; cfdata[devno].cf_name != NULL; devno++) {
				cfdata_t cf = &cfdata[devno];

				if (strlen(cf->cf_name) != len ||
				    strncasecmp(dev, cf->cf_name, len) != 0 ||
				    cf->cf_fstate != FSTATE_STAR)
					continue;

				cf->cf_fstate = FSTATE_DSTAR;
			}
		}
	}
}

static int
hvs_detach(device_t self, int flags)
{

	/* XXX detach */

	return 0;
}

#define XS2DMA(xs) \
    ((((xs)->xs_control & XS_CTL_DATA_IN) ? BUS_DMA_READ : BUS_DMA_WRITE) | \
    (((xs)->xs_control & XS_CTL_NOSLEEP) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK) | \
    BUS_DMA_STREAMING)

#define XS2DMAPRE(xs) (((xs)->xs_control & XS_CTL_DATA_IN) ? \
    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE)

#define XS2DMAPOST(xs) (((xs)->xs_control & XS_CTL_DATA_IN) ? \
    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE)

static void
hvs_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t request,
    void *arg)
{
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct hvs_softc *sc = device_private(adapt->adapt_dev);
	struct scsipi_xfer *xs;
	struct scsipi_xfer_mode *xm;
	struct scsipi_periph *periph;
	struct hvs_ccb *ccb;
	union hvs_cmd cmd;
	struct hvs_cmd_io *io = &cmd.io;
	struct hvs_srb *srb = &io->cmd_srb;
	int i, error;

	switch (request) {
	default:
		device_printf(sc->sc_dev,
		    "%s: unhandled request %u\n", __func__, request);
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		/* Not supported. */
		return;

	case ADAPTER_REQ_SET_XFER_MODE:
		xm = arg;
		xm->xm_mode = PERIPH_CAP_TQING;
		xm->xm_period = 0;
		xm->xm_offset = 0;
		scsipi_async_event(chan, ASYNC_EVENT_XFER_MODE, xm);
		return;

	case ADAPTER_REQ_RUN_XFER:
		break;
	}

	xs = arg;

	if (xs->cmdlen > MAX_SRB_DATA) {
		device_printf(sc->sc_dev, "CDB is too big: %d\n",
		    xs->cmdlen);
		memset(&xs->sense, 0, sizeof(xs->sense));
		xs->sense.scsi_sense.response_code =
		    SSD_RCODE_VALID | SSD_RCODE_CURRENT;
		xs->sense.scsi_sense.flags = SSD_ILI;
		xs->sense.scsi_sense.asc = 0x20;
		hvs_scsi_done(xs, XS_SENSE);
		return;
	}

	ccb = hvs_get_ccb(sc);
	if (ccb == NULL) {
		device_printf(sc->sc_dev, "failed to allocate ccb\n");
		hvs_scsi_done(xs, XS_RESOURCE_SHORTAGE);
		return;
	}

	periph = xs->xs_periph;

	memset(&cmd, 0, sizeof(cmd));

	srb->srb_initiator = chan->chan_id;
	srb->srb_bus = sc->sc_bus;
	srb->srb_target = periph->periph_target - 1;
	srb->srb_lun = periph->periph_lun;
	srb->srb_cdblen = xs->cmdlen;
	memcpy(srb->srb_data, xs->cmd, xs->cmdlen);

	if (sc->sc_config->use_win8ext_flags) {
		io->cmd_timeout = 60;
		SET(io->cmd_srbflags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
	}

	switch (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
	case XS_CTL_DATA_IN:
		srb->srb_direction = SRB_DATA_READ;
		if (sc->sc_config->use_win8ext_flags)
			SET(io->cmd_srbflags, SRB_FLAGS_DATA_IN);
		break;
	case XS_CTL_DATA_OUT:
		srb->srb_direction = SRB_DATA_WRITE;
		if (sc->sc_config->use_win8ext_flags)
			SET(io->cmd_srbflags, SRB_FLAGS_DATA_OUT);
		break;
	default:
		srb->srb_direction = SRB_DATA_NONE;
		if (sc->sc_config->use_win8ext_flags)
			SET(io->cmd_srbflags, SRB_FLAGS_NO_DATA_TRANSFER);
		break;
	}

	srb->srb_datalen = xs->datalen;
	srb->srb_reqlen = sc->sc_config->reqlen;
	srb->srb_senselen = sc->sc_config->senselen;

	cmd.cmd_op = HVS_REQ_SCSIIO;
	cmd.cmd_flags = VMBUS_CHANPKT_FLAG_RC;

	if (xs->datalen > 0) {
		error = bus_dmamap_load(sc->sc_dmat, ccb->ccb_dmap, xs->data,
		    xs->datalen, NULL, XS2DMA(xs));
		if (error) {
			device_printf(sc->sc_dev,
			    "failed to load %d bytes (%d)\n", xs->datalen,
			    error);
			hvs_put_ccb(sc, ccb);
			hvs_scsi_done(xs, (error == ENOMEM || error == EAGAIN) ?
			    XS_RESOURCE_SHORTAGE : XS_DRIVER_STUFFUP);
			return;
		}
		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmap, 0, xs->datalen,
		    XS2DMAPRE(xs));

		ccb->ccb_sgl->gpa_len = xs->datalen;
		ccb->ccb_sgl->gpa_ofs = (vaddr_t)xs->data & PAGE_MASK;
		for (i = 0; i < ccb->ccb_dmap->dm_nsegs; i++)
			ccb->ccb_sgl->gpa_page[i] =
			    atop(ccb->ccb_dmap->dm_segs[i].ds_addr);
		ccb->ccb_nsge = ccb->ccb_dmap->dm_nsegs;
	} else
		ccb->ccb_nsge = 0;

	ccb->ccb_xfer = xs;
	ccb->ccb_cmd = &cmd;
	ccb->ccb_done = hvs_scsi_cmd_done;

#ifdef HVS_DEBUG_IO
	printf("%s: %u.%u: rid %"PRIu64" opcode %#x control %#x datalen %d\n",
	    device_xname(sc->sc_dev), periph->periph_target, periph->periph_lun,
	    ccb->ccb_rid, xs->cmd->opcode, xs->xs_control, xs->datalen);
#endif

	if (xs->xs_control & XS_CTL_POLL)
		error = hvs_poll(sc, sc->sc_chan, ccb);
	else
		error = hvs_start(sc, sc->sc_chan, ccb);
	if (error) {
		hvs_put_ccb(sc, ccb);
		hvs_scsi_done(xs, (error == ENOMEM || error == EAGAIN) ?
		    XS_RESOURCE_SHORTAGE : XS_DRIVER_STUFFUP);
	}
}

static int
hvs_start(struct hvs_softc *sc, struct vmbus_channel *chan, struct hvs_ccb *ccb)
{
	union hvs_cmd *cmd = ccb->ccb_cmd;
	int error;

	ccb->ccb_cmd = NULL;

	if (ccb->ccb_nsge > 0) {
		error = vmbus_channel_send_prpl(chan, ccb->ccb_sgl,
		    ccb->ccb_nsge, cmd, HVS_CMD_SIZE, ccb->ccb_rid);
		if (error) {
			device_printf(sc->sc_dev,
			    "failed to submit operation %x via prpl\n",
			    cmd->cmd_op);
			bus_dmamap_unload(sc->sc_dmat, ccb->ccb_dmap);
		}
	} else {
		error = vmbus_channel_send(chan, cmd, HVS_CMD_SIZE,
		    ccb->ccb_rid, VMBUS_CHANPKT_TYPE_INBAND,
		    VMBUS_CHANPKT_FLAG_RC);
		if (error)
			device_printf(sc->sc_dev,
			    "failed to submit operation %x\n", cmd->cmd_op);
	}

	return error;
}

static void
hvs_poll_done(struct hvs_ccb *ccb)
{
	int *rv = ccb->ccb_cookie;

	if (ccb->ccb_cmd) {
		memcpy(&ccb->ccb_rsp, ccb->ccb_cmd, HVS_CMD_SIZE);
		ccb->ccb_cmd = &ccb->ccb_rsp;
	} else
		memset(&ccb->ccb_rsp, 0, HVS_CMD_SIZE);

	*rv = 0;
}

static int
hvs_poll(struct hvs_softc *sc, struct vmbus_channel *chan, struct hvs_ccb *ccb)
{
	void (*done)(struct hvs_ccb *);
	void *cookie;
	int s, rv = 1;

	done = ccb->ccb_done;
	cookie = ccb->ccb_cookie;

	ccb->ccb_done = hvs_poll_done;
	ccb->ccb_cookie = &rv;

	if (hvs_start(sc, chan, ccb)) {
		ccb->ccb_cookie = cookie;
		ccb->ccb_done = done;
		return -1;
	}

	while (rv == 1) {
		delay(10);
		s = splbio();
		hvs_intr(sc);
		splx(s);
	}

	ccb->ccb_cookie = cookie;
	ccb->ccb_done = done;
	ccb->ccb_done(ccb);

	return 0;
}

static void
hvs_intr(void *xsc)
{
	struct hvs_softc *sc = xsc;
	struct hvs_ccb *ccb;
	union hvs_cmd cmd;
	uint64_t rid;
	uint32_t rlen;
	int error;

	for (;;) {
		error = vmbus_channel_recv(sc->sc_chan, &cmd, sizeof(cmd),
		    &rlen, &rid, 0);
		switch (error) {
		case 0:
			break;
		case EAGAIN:
			/* No more messages to process */
			return;
		default:
			device_printf(sc->sc_dev,
			    "error %d while receiving a reply\n", error);
			return;
		}
		if (rlen != sizeof(cmd)) {
			device_printf(sc->sc_dev, "short read: %u\n", rlen);
			return;
		}

#ifdef HVS_DEBUG_IO
		printf("%s: rid %"PRIu64" operation %u flags %#x status %#x\n",
		    device_xname(sc->sc_dev), rid, cmd.cmd_op, cmd.cmd_flags,
		    cmd.cmd_status);
#endif

		switch (cmd.cmd_op) {
		case HVS_MSG_IODONE:
			if (rid >= sc->sc_nccb) {
				device_printf(sc->sc_dev,
				    "invalid response %#"PRIx64"\n", rid);
				continue;
			}
			ccb = &sc->sc_ccbs[rid];
			ccb->ccb_cmd = &cmd;
			ccb->ccb_done(ccb);
			break;
		case HVS_MSG_ENUMERATE:
			hvs_scsi_probe(sc);
			break;
		default:
			device_printf(sc->sc_dev,
			    "operation %u is not implemented\n", cmd.cmd_op);
			break;
		}
	}
}

static int
is_inquiry_valid(struct scsipi_inquiry_data *inq)
{

	if ((inq->device & SID_TYPE) == T_NODEVICE)
		return 0;
	if ((inq->device & SID_QUAL) == SID_QUAL_LU_NOT_SUPP)
		return 0;
	return 1;
}

static void
fixup_inquiry(struct scsipi_xfer *xs, struct hvs_srb *srb)
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_channel *chan = periph->periph_channel;
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct hvs_softc *sc = device_private(adapt->adapt_dev);
	struct scsipi_inquiry_data *inq = (void *)xs->data;
	int datalen, resplen;
	char vendor[8];

	resplen = srb->srb_datalen >= 5 ? inq->additional_length + 5 : 0;
	datalen = MIN(resplen, srb->srb_datalen);

	/* Fixup wrong response from WS2012 */
	if (sc->sc_config->fixup_wrong_response &&
	    !is_inquiry_valid(inq) && datalen >= 4 &&
	    (inq->version == 0 || inq->response_format == 0)) {
		inq->version = 0x05; /* SPC-3 */
		inq->response_format = SID_FORMAT_ISO;
	} else if (datalen >= SCSIPI_INQUIRY_LENGTH_SCSI2) {
		/*
		 * Upgrade SPC2 to SPC3 if host is Win8 or WS2012 R2
		 * to support UNMAP feature.
		 */
		strnvisx(vendor, sizeof(vendor), inq->vendor, sizeof(vendor),
		    VIS_TRIM|VIS_SAFE|VIS_OCTAL);
		if (sc->sc_config->upgrade_spc2_to_spc3 &&
		    (inq->version & SID_ANSII) == 0x04 /* SPC-2 */ &&
		    !strncmp(vendor, "Msft", 4))
			inq->version = 0x05; /* SPC-3 */
	}
}

static void
hvs_scsi_cmd_done(struct hvs_ccb *ccb)
{
	struct scsipi_xfer *xs = ccb->ccb_xfer;
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_channel *chan = periph->periph_channel;
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct hvs_softc *sc = device_private(adapt->adapt_dev);
	union hvs_cmd *cmd = ccb->ccb_cmd;
	struct hvs_srb *srb;
	bus_dmamap_t map;
	int error;

	map = ccb->ccb_dmap;
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize, XS2DMAPOST(xs));
	bus_dmamap_unload(sc->sc_dmat, map);

	xs = ccb->ccb_xfer;
	srb = &cmd->io.cmd_srb;

	xs->status = srb->srb_scsistatus & 0xff;

	switch (xs->status) {
	case SCSI_OK:
		if ((srb->srb_iostatus & ~(SRB_STATUS_AUTOSENSE_VALID |
		    SRB_STATUS_QUEUE_FROZEN)) != SRB_STATUS_SUCCESS)
			error = XS_SELTIMEOUT;
		else
			error = XS_NOERROR;
		break;
	case SCSI_BUSY:
	case SCSI_QUEUE_FULL:
		device_printf(sc->sc_dev, "status %#x iostatus %#x (busy)\n",
		    srb->srb_scsistatus, srb->srb_iostatus);
		error = XS_BUSY;
		break;
	case SCSI_CHECK:
		if (srb->srb_iostatus & SRB_STATUS_AUTOSENSE_VALID) {
			memcpy(&xs->sense, srb->srb_data,
			    MIN(sizeof(xs->sense), srb->srb_senselen));
			error = XS_SENSE;
			break;
		}
		/* FALLTHROUGH */
	default:
		error = XS_DRIVER_STUFFUP;
		break;
	}

	if (error == XS_NOERROR) {
		if (xs->cmd->opcode == INQUIRY)
			fixup_inquiry(xs, srb);
		else if (srb->srb_direction != SRB_DATA_NONE)
			xs->resid = xs->datalen - srb->srb_datalen;
	}

	hvs_put_ccb(sc, ccb);
	hvs_scsi_done(xs, error);
}

static void
hvs_scsi_probe(void *arg)
{
	struct hvs_softc *sc = arg;

	if (sc->sc_scsibus != NULL)
		scsi_probe_bus((void *)sc->sc_scsibus, -1, -1);
}

static void
hvs_scsi_done(struct scsipi_xfer *xs, int error)
{

	xs->error = error;
	scsipi_done(xs);
}

static int
hvs_connect(struct hvs_softc *sc)
{
	union hvs_cmd ucmd;
	struct hvs_cmd_ver *cmd;
	struct hvs_chp *chp;
	struct hvs_ccb *ccb;
#if notyet /* XXX subchannel */
	struct vmbus_softc *vsc;
	struct vmbus_channel **subchan;
	uint32_t version;
	uint16_t max_subch, req_subch;
	bool support_multichannel = false;
#endif
	int i;

	ccb = hvs_get_ccb(sc);
	if (ccb == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to allocate ccb\n");
		return -1;
	}

	ccb->ccb_done = hvs_empty_done;

	cmd = (struct hvs_cmd_ver *)&ucmd;

	/*
	 * Begin initialization
	 */

	memset(&ucmd, 0, sizeof(ucmd));

	cmd->cmd_op = HVS_REQ_STARTINIT;
	cmd->cmd_flags = VMBUS_CHANPKT_FLAG_RC;

	ccb->ccb_cmd = &ucmd;
	if (hvs_poll(sc, sc->sc_chan, ccb)) {
		aprint_error_dev(sc->sc_dev,
		    "failed to send initialization command\n");
		goto error;
	}
	if (ccb->ccb_rsp.cmd_status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to initialize, status %#x\n",
		    ccb->ccb_rsp.cmd_status);
		goto error;
	}

	/*
	 * Negotiate protocol version
	 */

	memset(&ucmd, 0, sizeof(ucmd));

	cmd->cmd_op = HVS_REQ_QUERYPROTO;
	cmd->cmd_flags = VMBUS_CHANPKT_FLAG_RC;

	for (i = 0; i < __arraycount(hvs_config_list); i++) {
		cmd->cmd_ver = hvs_config_list[i].proto_version;

		ccb->ccb_cmd = &ucmd;
		if (hvs_poll(sc, sc->sc_chan, ccb)) {
			aprint_error_dev(sc->sc_dev,
			    "failed to send protocol query\n");
			goto error;
		}
		if (ccb->ccb_rsp.cmd_status == 0) {
			sc->sc_config = &hvs_config_list[i];
			break;
		}
	}
	if (sc->sc_config == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "failed to negotiate protocol version\n");
		goto error;
	}

	/*
	 * Query channel properties
	 */

	memset(&ucmd, 0, sizeof(ucmd));

	cmd->cmd_op = HVS_REQ_QUERYPROPS;
	cmd->cmd_flags = VMBUS_CHANPKT_FLAG_RC;

	ccb->ccb_cmd = &ucmd;
	if (hvs_poll(sc, sc->sc_chan, ccb)) {
		aprint_error_dev(sc->sc_dev,
		    "failed to send channel properties query\n");
		goto error;
	}
	if (ccb->ccb_rsp.cmd_op != HVS_MSG_IODONE ||
	    ccb->ccb_rsp.cmd_status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to obtain channel properties, status %#x\n",
		    ccb->ccb_rsp.cmd_status);
		goto error;
	}
	chp = &ccb->ccb_rsp.chp.cmd_chp;

	DPRINTF("%s: proto %#x path %u target %u maxchan %u port %u "
	    "chflags %#x maxfer %u chanid %#"PRIx64"\n",
	    device_xname(sc->sc_dev), chp->chp_proto, chp->chp_path,
	    chp->chp_target, chp->chp_maxchan, chp->chp_port,
	    chp->chp_chflags, chp->chp_maxfer, chp->chp_chanid);

#if notyet /* XXX subchannel */
	max_subch = chp->chp_maxchan;
	if (hvs_chan_cnt > 0 && hvs_chan_cnt < (max_subch + 1))
		max_subch = hvs_chan_cnt - 1;

	/* multi-channels feature is supported by WIN8 and above version */
	version = sc->sc_chan->ch_sc->sc_proto;
	if (version != VMBUS_VERSION_WIN7 && version != VMBUS_VERSION_WS2008 &&
	    ISSET(chp->chp_chflags, CHP_CHFLAGS_MULTI_CHANNEL))
		support_multichannel = true;
#endif

	/* XXX */
	sc->sc_bus = chp->chp_path;
	sc->sc_channel.chan_id = chp->chp_target;

	/*
	 * Finish initialization
	 */

	memset(&ucmd, 0, sizeof(ucmd));

	cmd->cmd_op = HVS_REQ_FINISHINIT;
	cmd->cmd_flags = VMBUS_CHANPKT_FLAG_RC;

	ccb->ccb_cmd = &ucmd;
	if (hvs_poll(sc, sc->sc_chan, ccb)) {
		aprint_error_dev(sc->sc_dev,
		    "failed to send initialization finish\n");
		goto error;
	}
	if (ccb->ccb_rsp.cmd_op != HVS_MSG_IODONE ||
	    ccb->ccb_rsp.cmd_status != 0) {
		aprint_error_dev(sc->sc_dev,
		    "failed to finish initialization, status %#x\n",
		    ccb->ccb_rsp.cmd_status);
		goto error;
	}

#if notyet /* XXX subchannel */
	if (support_multichannel && max_subch > 0 && ncpu > 1) {
		req_subch = MIN(max_subch, ncpu - 1);

		memset(&ucmd, 0, sizeof(ucmd));

		cmd->cmd_op = HVS_REQ_CREATEMULTICHANNELS;
		cmd->cmd_flags = VMBUS_CHANPKT_FLAG_RC;
		cmd->u.multi_chans_cnt = req_subch;

		ccb->ccb_cmd = &ucmd;
		if (hvs_poll(sc, sc->sc_chan, ccb)) {
			aprint_error_dev(sc->sc_dev,
			    "failed to send create multi-channel\n");
			goto out;
		}
		if (ccb->ccb_rsp.cmd_op != HVS_MSG_IODONE ||
		    ccb->ccb_rsp.cmd_status != 0) {
			aprint_error_dev(sc->sc_dev,
			    "failed to create multi-channel, status %#x\n",
			    ccb->ccb_rsp.cmd_status);
			goto out;
		}

		sc->sc_nchan = req_subch + 1;
		subchan = vmbus_subchan_get(sc->sc_chan, req_subch);
		for (i = 0; i < req_subch; i++)
			hsv_subchan_attach(sc, subchan[i]);
		vmbus_subchan_rel(subchan, req_subch);
		aprint_normal_dev(sc->sc_dev, "using %u channels\n",
		    sc->sc_nchan);
	}
out:
#endif
	hvs_put_ccb(sc, ccb);
	return 0;

error:
	hvs_put_ccb(sc, ccb);
	return -1;
}

static void
hvs_empty_done(struct hvs_ccb *ccb)
{
	/* nothing */
}

static int
hvs_alloc_ccbs(struct hvs_softc *sc)
{
	const int kmemflags = cold ? KM_NOSLEEP : KM_SLEEP;
	const int dmaflags = cold ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;
	int i, error;

	SIMPLEQ_INIT(&sc->sc_ccb_fq);
	mutex_init(&sc->sc_ccb_fqlck, MUTEX_DEFAULT, IPL_BIO);

	sc->sc_nccb = HVS_MAX_CCB;
	sc->sc_ccbs = kmem_zalloc(sc->sc_nccb * sizeof(struct hvs_ccb),
	    kmemflags);
	if (sc->sc_ccbs == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to allocate CCBs\n");
		return -1;
	}

	for (i = 0; i < sc->sc_nccb; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, HVS_MAX_SGE,
		    PAGE_SIZE, PAGE_SIZE, dmaflags, &sc->sc_ccbs[i].ccb_dmap);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "failed to create a CCB memory map (%d)\n", error);
			goto errout;
		}

		sc->sc_ccbs[i].ccb_sgl = kmem_zalloc(
		    sizeof(struct vmbus_gpa_range) * (HVS_MAX_SGE + 1),
		    kmemflags);
		if (sc->sc_ccbs[i].ccb_sgl == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "failed to allocate SGL array\n");
			goto errout;
		}

		sc->sc_ccbs[i].ccb_rid = i;
		hvs_put_ccb(sc, &sc->sc_ccbs[i]);
	}

	return 0;

 errout:
	hvs_free_ccbs(sc);
	return -1;
}

static void
hvs_free_ccbs(struct hvs_softc *sc)
{
	struct hvs_ccb *ccb;
	int i;

	for (i = 0; i < sc->sc_nccb; i++) {
		ccb = &sc->sc_ccbs[i];
		if (ccb->ccb_dmap == NULL)
			continue;

		bus_dmamap_sync(sc->sc_dmat, ccb->ccb_dmap, 0, 0,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, ccb->ccb_dmap);
		bus_dmamap_destroy(sc->sc_dmat, ccb->ccb_dmap);

		kmem_free(ccb->ccb_sgl,
		    sizeof(struct vmbus_gpa_range) * (HVS_MAX_SGE + 1));
	}

	kmem_free(sc->sc_ccbs, sc->sc_nccb * sizeof(struct hvs_ccb));
	sc->sc_ccbs = NULL;
	sc->sc_nccb = 0;
}

static struct hvs_ccb *
hvs_get_ccb(struct hvs_softc *sc)
{
	struct hvs_ccb *ccb;

	mutex_enter(&sc->sc_ccb_fqlck);
	ccb = SIMPLEQ_FIRST(&sc->sc_ccb_fq);
	if (ccb != NULL)
		SIMPLEQ_REMOVE_HEAD(&sc->sc_ccb_fq, ccb_link);
	mutex_exit(&sc->sc_ccb_fqlck);

	return ccb;
}

static void
hvs_put_ccb(struct hvs_softc *sc, struct hvs_ccb *ccb)
{

	ccb->ccb_cmd = NULL;
	ccb->ccb_xfer = NULL;
	ccb->ccb_done = NULL;
	ccb->ccb_cookie = NULL;
	ccb->ccb_nsge = 0;

	mutex_enter(&sc->sc_ccb_fqlck);
	SIMPLEQ_INSERT_HEAD(&sc->sc_ccb_fq, ccb, ccb_link);
	mutex_exit(&sc->sc_ccb_fqlck);
}
