/* $NetBSD: di.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: di.c,v 1.1 2026/01/09 22:54:29 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>
#include <sys/dvdio.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/wii.h>
#include <machine/wiiu.h>
#include <machine/pio.h>
#include "ahb.h"

#ifdef DI_DEBUG
#define DPRINTF(dv, fmt, ...)	device_printf(dv, fmt, ## __VA_ARGS__)
#else
#define DPRINTF(dv, fmt, ...)
#endif

#define DI_REG_SIZE	0x40

#define DISR		0x00
#define  DISR_BRKINT		__BIT(6)
#define  DISR_BRKINTMASK	__BIT(5)
#define  DISR_TCINT		__BIT(4)
#define  DISR_TCINTMASK		__BIT(3)
#define  DISR_DEINT		__BIT(2)
#define  DISR_DEINTMASK		__BIT(1)
#define  DISR_BRK		__BIT(0)
#define DICVR		0x04
#define  DICVR_CVRINT		__BIT(2)
#define  DICVR_CVRINTMASK	__BIT(1)
#define  DICVR_CVR		__BIT(0)
#define DICMDBUF0	0x08
#define DICMDBUF1	0x0c
#define DICMDBUF2	0x10
#define DIMAR		0x14
#define DILENGTH	0x18
#define DICR		0x1c
#define  DICR_DMA		__BIT(1)
#define  DICR_TSTART		__BIT(0)
#define DIMMBUF		0x20
#define DICFG		0x24

#define DI_CMD_INQUIRY			0x12000000
#define DI_CMD_REPORT_KEY(x)		(0xa4000000 | ((uint32_t)(x) << 16))
#define DI_CMD_READ_DVD_STRUCT(x)	(0xad000000 | ((uint32_t)(x) << 24))
#define DI_CMD_READ_DVD			0xd0000000
#define DI_CMD_REQUEST_ERROR		0xe0000000
#define DI_CMD_STOP_MOTOR		0xe3000000

#define DVDBLOCKSIZE	2048

#define	DI_IDLE_TIMEOUT_MS	30000

struct di_softc;

static int	di_match(device_t, cfdata_t, void *);
static void	di_attach(device_t, device_t, void *);

static bool	di_shutdown(device_t, int);

static int	di_intr(void *);
static void	di_timeout(void *);
static void	di_idle(void *);

static void	di_request(struct scsipi_channel *, scsipi_adapter_req_t,
			   void *);
static void	di_init_regs(struct di_softc *);
static void	di_reset(struct di_softc *, bool);

struct di_response_inquiry {
	uint16_t		revision_level;
	uint16_t		device_code;
	uint32_t		release_date;
	uint8_t			padding[24];
} __aligned(4);
CTASSERT(sizeof(struct di_response_inquiry) == 0x20);

struct di_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;

	struct scsipi_adapter	sc_adapter;
	struct scsipi_channel	sc_channel;

	struct scsipi_xfer	*sc_cur_xs;
	callout_t		sc_timeout;
	callout_t		sc_idle;
	int			sc_pamr;

	bus_dmamap_t		sc_dma_map;
	void			*sc_dma_addr;
	size_t			sc_dma_size;
	bus_dma_segment_t	sc_dma_segs[1];
};

#define WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

CFATTACH_DECL_NEW(di, sizeof(struct di_softc),
    di_match, di_attach, NULL, NULL);

static int
di_match(device_t parent, cfdata_t cf, void *aux)
{
	return !wiiu_native;
}

static void
di_attach(device_t parent, device_t self, void *aux)
{
	struct ahb_attach_args *aaa = aux;
	struct di_softc *sc = device_private(self);
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;
	int error, nsegs;

	sc->sc_dev = self;
	sc->sc_dmat = aaa->aaa_dmat;
	sc->sc_bst = aaa->aaa_bst;
	error = bus_space_map(sc->sc_bst, aaa->aaa_addr, DI_REG_SIZE,
	    0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error(": couldn't map registers (%d)\n", error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Drive Interface\n");

	callout_init(&sc->sc_timeout, 0);
	callout_setfunc(&sc->sc_timeout, di_timeout, sc);
	callout_init(&sc->sc_idle, 0);
	callout_setfunc(&sc->sc_idle, di_idle, sc);

	sc->sc_dma_size = MAXPHYS;
	error = bus_dmamem_alloc(sc->sc_dmat, sc->sc_dma_size, PAGE_SIZE, 0,
	    sc->sc_dma_segs, 1, &nsegs, BUS_DMA_WAITOK);
	if (error != 0) {
		aprint_error_dev(self, "bus_dmamem_alloc failed: %d\n", error);
		return;
	}
	error = bus_dmamem_map(sc->sc_dmat, sc->sc_dma_segs, nsegs,
	    sc->sc_dma_size, &sc->sc_dma_addr, BUS_DMA_WAITOK);
	if (error != 0) {
		aprint_error_dev(self, "bus_dmamem_map failed: %d\n", error);
		return;
	}
	error = bus_dmamap_create(sc->sc_dmat, sc->sc_dma_size, nsegs,
	    sc->sc_dma_size, 0, BUS_DMA_WAITOK, &sc->sc_dma_map);
	if (error != 0) {
		aprint_error_dev(self, "bus_dmamap_create failed: %d\n", error);
		return;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dma_map, sc->sc_dma_addr,
	    sc->sc_dma_size, NULL, BUS_DMA_WAITOK);
	if (error != 0) {
		aprint_error_dev(self, "bus_dmamap_load failed: %d\n", error);
		return;
	}

	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_nchannels = 1;
	adapt->adapt_request = di_request;
	adapt->adapt_minphys = minphys;
	adapt->adapt_dev = self;
	adapt->adapt_max_periph = 1;
	adapt->adapt_openings = 1;

	memset(chan, 0, sizeof(*chan));
	chan->chan_bustype = &scsi_bustype;
	chan->chan_ntargets = 2;
	chan->chan_nluns = 1;
	chan->chan_id = 0;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE;
	chan->chan_adapter = adapt;

	config_found(self, chan, scsiprint, CFARGS(.iattr = "scsi"));

	ahb_intr_establish(aaa->aaa_irq, IPL_BIO, di_intr, sc,
	    device_xname(self));

	di_init_regs(sc);
	callout_schedule(&sc->sc_idle, mstohz(DI_IDLE_TIMEOUT_MS));

	pmf_device_register1(self, NULL, NULL, di_shutdown);
}

static bool
di_shutdown(device_t dev, int how)
{
	struct di_softc *sc = device_private(dev);

	di_reset(sc, false);

	return true;
}

static void
di_sense(struct scsipi_xfer *xs, uint8_t skey, uint8_t asc, uint8_t ascq)
{
	struct scsi_sense_data *sense = &xs->sense.scsi_sense;

	xs->error = XS_SENSE;
	sense->response_code = SSD_RCODE_CURRENT | SSD_RCODE_VALID;
	sense->flags = skey;
	sense->asc = asc;
	sense->ascq = ascq;
}

static void
di_request_error_sync(struct di_softc *sc, struct scsipi_xfer *xs)
{
	uint32_t imm;
	int s;

	s = splbio();
	WR4(sc, DICMDBUF0, DI_CMD_REQUEST_ERROR);
	WR4(sc, DICMDBUF1, 0);
	WR4(sc, DICMDBUF2, 0);
	WR4(sc, DILENGTH, 4);
	WR4(sc, DICR, DICR_TSTART);
	while (((RD4(sc, DISR) & DISR_TCINT)) == 0) {
		delay(1);
	}
	imm = RD4(sc, DIMMBUF);
	splx(s);

	DPRINTF(sc->sc_dev, "ERR IMMBUF = 0x%08x\n", imm);
	di_sense(xs, (imm >> 16) & 0xff, (imm >> 8) & 0xff, imm & 0xff);
}

static int
di_transfer_error(struct di_softc *sc, struct scsipi_xfer *xs)
{
	if (xs == NULL) {
		return 0;
	}

	DPRINTF(sc->sc_dev, "transfer error\n");

	callout_stop(&sc->sc_timeout);
	di_request_error_sync(sc, xs);
	sc->sc_cur_xs = NULL;
	scsipi_done(xs);

	return 1;
}

static int
di_transfer_complete(struct di_softc *sc, struct scsipi_xfer *xs)
{
	struct scsipi_generic *cmd;
	struct scsipi_inquiry_data *inqbuf;
	struct scsipi_read_cd_cap_data *cdcap;
	struct di_response_inquiry *rinq;
	uint32_t imm;
	uint8_t *data;
	char buf[5];

	if (xs == NULL) {
		DPRINTF(sc->sc_dev, "no active transfer\n");
		return 0;
	}

	KASSERT(sc->sc_cur_xs == xs);

	cmd = xs->cmd;

	switch (cmd->opcode) {
	case INQUIRY:
		inqbuf = (struct scsipi_inquiry_data *)xs->data;
		rinq = sc->sc_dma_addr;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sizeof(*rinq),
		    BUS_DMASYNC_POSTREAD);

		DPRINTF(sc->sc_dev, "revision_level %#x "
				    "device_code %#x "
				    "release_date %#x\n",
				    rinq->revision_level,
				    rinq->device_code,
				    rinq->release_date);

		memset(inqbuf, 0, sizeof(*inqbuf));
		inqbuf->device = T_CDROM;
		inqbuf->dev_qual2 = SID_REMOVABLE;
		strncpy(inqbuf->vendor, "NINTENDO", sizeof(inqbuf->vendor));
		snprintf(inqbuf->product, sizeof(inqbuf->product), "%08x",
		    rinq->release_date);
		snprintf(buf, sizeof(buf), "%04x", rinq->revision_level);
		memcpy(inqbuf->revision, buf, sizeof(inqbuf->revision));
		xs->resid = 0;
		break;

	case SCSI_TEST_UNIT_READY:
	case SCSI_REQUEST_SENSE:
		imm = RD4(sc, DIMMBUF);
		DPRINTF(sc->sc_dev, "TUR IMMBUF = 0x%08x\n", imm);
		switch ((imm >> 24) & 0xff) {
		case 0:
			di_sense(xs, (imm >> 16) & 0xff, (imm >> 8) & 0xff,
			    imm & 0xff);
			break;
		default:
			di_sense(xs, SKEY_MEDIUM_ERROR, 0, 0);
			break;
		}
		break;

	case SCSI_READ_6_COMMAND:
	case READ_10:
	case GPCMD_REPORT_KEY:
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, xs->datalen,
		    BUS_DMASYNC_POSTREAD);
		memcpy(xs->data, sc->sc_dma_addr, xs->datalen);
		xs->resid = 0;
		break;

	case GPCMD_READ_DVD_STRUCTURE:
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, DVDBLOCKSIZE,
		    BUS_DMASYNC_POSTREAD);
		memcpy(xs->data + 4, sc->sc_dma_addr, xs->datalen - 4);
		xs->resid = 0;
		break;

	case READ_CD_CAPACITY:
		cdcap = (struct scsipi_read_cd_cap_data *)xs->data;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, DVDBLOCKSIZE,
		    BUS_DMASYNC_POSTREAD);
		data = sc->sc_dma_addr;
		_lto4b(DVDBLOCKSIZE, cdcap->length);
		memcpy(cdcap->addr, &data[8], sizeof(cdcap->addr));
		break;
	}

	sc->sc_cur_xs = NULL;
	scsipi_done(xs);

	return 1;
}

static int
di_intr(void *priv)
{
	struct di_softc *sc = priv;
	uint32_t sr, cvr;
	int ret = 0;

	sr = RD4(sc, DISR);
	cvr = RD4(sc, DICVR);

	if ((sr & DISR_DEINT) != 0) {
		ret |= di_transfer_error(sc, sc->sc_cur_xs);
	} else if ((sr & DISR_TCINT) != 0) {
		ret |= di_transfer_complete(sc, sc->sc_cur_xs);
	}

	if ((cvr & DICVR_CVRINT) != 0) {
		DPRINTF(sc->sc_dev, "drive %s\n",
		    (cvr & DICVR_CVR) == 0 ? "closed" : "opened");
		ret |= 1;
	}

	WR4(sc, DISR, sr);
	WR4(sc, DICVR, cvr);

	return ret;
}

static void
di_timeout(void *priv)
{
	struct di_softc *sc = priv;
	int s;

	s = splbio();
	if (sc->sc_cur_xs != NULL) {
		struct scsipi_xfer *xs = sc->sc_cur_xs;

		DPRINTF(sc->sc_dev, "command %#x timeout, DISR = %#x\n",
		    xs->cmd->opcode, RD4(sc, DISR));
		xs->error = XS_TIMEOUT;
		scsipi_done(xs);

		sc->sc_cur_xs = NULL;
	}
	splx(s);
}

static void
di_idle(void *priv)
{
	struct di_softc *sc = priv;

	if ((RD4(sc, DICVR) & DICVR_CVR) != 0) {
		/* Cover is opened, nothing to do. */
		return;
	}

	di_reset(sc, false);
}

static void
di_start_request(struct di_softc *sc, struct scsipi_xfer *xs)
{
	KASSERT(sc->sc_cur_xs == NULL);
	sc->sc_cur_xs = xs;
	if (xs->timeout != 0) {
		callout_schedule(&sc->sc_timeout, mstohz(xs->timeout) + 1);
	} else {
		DPRINTF(sc->sc_dev, "WARNING: xfer with no timeout!\n");
		callout_schedule(&sc->sc_timeout, mstohz(15000));
	}
}

static void
di_init_regs(struct di_softc *sc)
{
	WR4(sc, DISR, DISR_BRKINT |
		      DISR_TCINT | DISR_TCINTMASK |
		      DISR_DEINT | DISR_DEINTMASK);
	WR4(sc, DICVR, DICVR_CVRINT | DICVR_CVRINTMASK);
}

static void
di_reset(struct di_softc *sc, bool spinup)
{
	uint32_t val;
	int s;

	DPRINTF(sc->sc_dev, "reset spinup=%d\n", spinup);

	s = splhigh();

	if (spinup) {
		out32(HW_GPIOB_OUT, in32(HW_GPIOB_OUT) & ~__BIT(GPIO_DI_SPIN));
	} else {
		out32(HW_GPIOB_OUT, in32(HW_GPIOB_OUT) | __BIT(GPIO_DI_SPIN));
	}

	val = in32(HW_RESETS);
	out32(HW_RESETS, val & ~RSTB_IODI);
	delay(12);
	out32(HW_RESETS, val | RSTB_IODI);

	WR4(sc, DISR, DISR_BRKINT |
		      DISR_TCINT | DISR_TCINTMASK |
		      DISR_DEINT | DISR_DEINTMASK);
	WR4(sc, DICVR, DICVR_CVRINT | DICVR_CVRINTMASK);

	splx(s);
}

static void
di_stop_motor(struct di_softc *sc, struct scsipi_xfer *xs, bool eject)
{
	uint32_t cmdflags = 0;
	int s;

	if (eject) {
		cmdflags |= 1 << 17;
	}

	s = splbio();
	WR4(sc, DICMDBUF0, DI_CMD_STOP_MOTOR | cmdflags);
	WR4(sc, DICMDBUF1, 0);
	WR4(sc, DICMDBUF2, 0);
	WR4(sc, DILENGTH, 4);
	WR4(sc, DICR, DICR_TSTART);
	di_start_request(sc, xs);
	splx(s);
}

static void
di_request(struct scsipi_channel *chan, scsipi_adapter_req_t req, void *arg)
{
	struct di_softc *sc = device_private(chan->chan_adapter->adapt_dev);
	struct scsipi_xfer *xs;
	struct scsipi_generic *cmd;
	struct scsipi_start_stop *ss;
	struct scsi_prevent_allow_medium_removal *pamr;
	uint32_t blkno;
	int s;

	if (req != ADAPTER_REQ_RUN_XFER) {
		return;
	}

	callout_stop(&sc->sc_idle);

	KASSERT(sc->sc_cur_xs == NULL);

	xs = arg;
	cmd = xs->cmd;

	switch (cmd->opcode) {
	case INQUIRY:
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map,
		    0, sizeof(struct di_response_inquiry),
		    BUS_DMASYNC_PREREAD);

		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_INQUIRY);
		WR4(sc, DICMDBUF1, 0);
		WR4(sc, DILENGTH, sizeof(struct di_response_inquiry));
		WR4(sc, DIMAR, sc->sc_dma_segs[0].ds_addr);
		WR4(sc, DICR, DICR_TSTART | DICR_DMA);
		di_start_request(sc, xs);
		splx(s);
		break;

	case SCSI_TEST_UNIT_READY:
	case SCSI_REQUEST_SENSE:
		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_REQUEST_ERROR);
		WR4(sc, DICMDBUF1, 0);
		WR4(sc, DICMDBUF2, 0);
		WR4(sc, DILENGTH, 4);
		WR4(sc, DICR, DICR_TSTART);
		di_start_request(sc, xs);
		splx(s);
		break;

	case SCSI_READ_6_COMMAND:
	case READ_10:
		if (cmd->opcode == SCSI_READ_6_COMMAND) {
			blkno = _3btol(((struct scsi_rw_6 *)cmd)->addr);
		} else {
			KASSERT(cmd->opcode == READ_10);
			blkno = _4btol(((struct scsipi_rw_10 *)cmd)->addr);
		}

		if (xs->datalen == 0) {
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map,
		    0, xs->datalen, BUS_DMASYNC_PREREAD);

		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_READ_DVD);
		WR4(sc, DICMDBUF1, blkno);
		WR4(sc, DICMDBUF2, howmany(xs->datalen, DVDBLOCKSIZE));
		WR4(sc, DILENGTH, roundup(xs->datalen, DVDBLOCKSIZE));
		WR4(sc, DIMAR, sc->sc_dma_segs[0].ds_addr);
		WR4(sc, DICR, DICR_TSTART | DICR_DMA);
		di_start_request(sc, xs);
		splx(s);
		break;

	case GPCMD_READ_DVD_STRUCTURE:
		if (xs->datalen == 0) {
			DPRINTF(sc->sc_dev, "zero datalen\n");
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map,
		    0, xs->datalen, BUS_DMASYNC_PREREAD);

		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_READ_DVD_STRUCT(cmd->bytes[6]));
		WR4(sc, DICMDBUF1, 0);
		WR4(sc, DICMDBUF2, 0);
		WR4(sc, DILENGTH, roundup(xs->datalen, DVDBLOCKSIZE));
		WR4(sc, DIMAR, sc->sc_dma_segs[0].ds_addr);
		WR4(sc, DICR, DICR_TSTART | DICR_DMA);
		di_start_request(sc, xs);
		splx(s);
		break;

	case GPCMD_REPORT_KEY:
		if (xs->datalen == 0) {
			DPRINTF(sc->sc_dev, "zero datalen\n");
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map,
		    0, xs->datalen, BUS_DMASYNC_PREREAD);

		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_REPORT_KEY(cmd->bytes[9] >> 2));
		WR4(sc, DICMDBUF1, _4btol(&cmd->bytes[1]));
		WR4(sc, DICMDBUF2, 0);
		WR4(sc, DILENGTH, roundup(xs->datalen, 0x20));
		WR4(sc, DIMAR, sc->sc_dma_segs[0].ds_addr);
		WR4(sc, DICR, DICR_TSTART | DICR_DMA);
		di_start_request(sc, xs);
		splx(s);
		break;

	case READ_CD_CAPACITY:
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map,
		    0, DVDBLOCKSIZE, BUS_DMASYNC_PREREAD);

		s = splbio();
		WR4(sc, DICMDBUF0, DI_CMD_READ_DVD_STRUCT(DVD_STRUCT_PHYSICAL));
		WR4(sc, DICMDBUF1, 0);
		WR4(sc, DICMDBUF2, 0);
		WR4(sc, DILENGTH, DVDBLOCKSIZE);
		WR4(sc, DIMAR, sc->sc_dma_segs[0].ds_addr);
		WR4(sc, DICR, DICR_TSTART | DICR_DMA);
		di_start_request(sc, xs);
		splx(s);
		break;

	case GET_CONFIGURATION:
		memset(xs->data, 0, sizeof(struct scsipi_get_conf_data));
		xs->resid = 0;
		scsipi_done(xs);
		break;

	case READ_TOC:
		memset(xs->data, 0, sizeof(struct scsipi_toc_header));
		xs->resid = 0;
		scsipi_done(xs);
		break;

	case READ_TRACKINFO:
	case READ_DISCINFO:
		di_sense(xs, SKEY_ILLEGAL_REQUEST, 0, 0);
		scsipi_done(xs);
		break;

	case START_STOP:
		ss = (struct scsipi_start_stop *)cmd;
		if (ss->how == SSS_START) {
			di_reset(sc, true);
			scsipi_done(xs);
		} else {
			di_stop_motor(sc, xs, (ss->how & SSS_LOEJ) != 0);
		}
		break;

	case SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL:
		pamr = (struct scsi_prevent_allow_medium_removal *)cmd;
		sc->sc_pamr = pamr->how;
		scsipi_done(xs);
		break;

	default:
		DPRINTF(sc->sc_dev, "unsupported opcode %#x\n", cmd->opcode);
		scsipi_done(xs);
	}

	if (!sc->sc_pamr) {
		callout_schedule(&sc->sc_idle, mstohz(DI_IDLE_TIMEOUT_MS));
	}
}
