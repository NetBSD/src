/* $NetBSD: vatapi.c,v 1.2.2.2 2018/06/25 07:25:46 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Reinoud Zandijk <reinoud@NetBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vatapi.c,v 1.2.2.2 2018/06/25 07:25:46 pgoyette Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/scsiio.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>	/* for SCSI status */
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/atapiconf.h>

#include "opt_scsi.h"

/* parameter? */
#define VDEV_ATAPI_DRIVE	0
#define MAX_SIZE		((1<<16))

/* forwards */
struct vatapi_softc;

static int	vatapi_match(device_t, cfdata_t, void *);
static void	vatapi_attach(device_t, device_t, void *);
static void	vatapi_callback(device_t self);

static void	vatapi_minphys(struct buf *bp);
static void	vatapi_kill_pending(struct scsipi_periph *periph);
static void	vatapi_scsipi_request(struct scsipi_channel *chan,
	scsipi_adapter_req_t req, void *arg);
static void	vatapi_probe_device(struct atapibus_softc *, int);

static void	vatapi_complete(void *arg);

/* for debugging */
#ifdef SCSIVERBOSE
void	scsipi_print_sense_data_real(struct scsi_sense_data *sense, int verbosity);
#endif


/* Note its one vdev, one adapter, one channel for now */
struct vatapi_softc {
	device_t	sc_dev;
	device_t	sc_pdev;

	int		sc_flags;
#define VATAPI_FLAG_POLLING	1
#define VATAPI_FLAG_INTR	2
	/* backing `device' with its active command */
	int		sc_fd;
	void		*sc_ih;
	struct scsipi_xfer *sc_xs;

	/* atapibus */
	device_t	sc_vatapibus;
	struct atapi_adapter    sc_atapi_adapter;
#define sc_adapter sc_atapi_adapter._generic
	struct scsipi_channel sc_channel;
};

CFATTACH_DECL_NEW(vatapi_thunkbus, sizeof(struct vatapi_softc),
    vatapi_match, vatapi_attach, NULL, NULL);


static const struct scsipi_bustype vatapi_bustype = {
	SCSIPI_BUSTYPE_ATAPI,
	atapi_scsipi_cmd,
	atapi_interpret_sense,
	atapi_print_addr,
	vatapi_kill_pending,
	NULL
};

int
vatapi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_VATAPI)
		return 0;

	return 1;
}

static void
vatapi_attach(device_t parent, device_t self, void *opaque)
{
	struct vatapi_softc *sc = device_private(self);
	struct thunkbus_attach_args *taa = opaque;

	sc->sc_dev = self;
	sc->sc_pdev = parent;

	/* open device */
	sc->sc_fd = thunk_open(taa->u.vdev.path, O_RDWR, 0);
	if (sc->sc_fd < 0) {
		aprint_error(": error %d opening path\n", thunk_geterrno());
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_ih = softint_establish(SOFTINT_BIO,
		vatapi_complete, sc);

	/* rest of the configuration is deferred */
	config_interrupts(self, vatapi_callback);
}

static void
vatapi_callback(device_t self)
{
	struct vatapi_softc *sc = device_private(self);
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan  = &sc->sc_channel;

	/* set up the scsipi adapter and channel */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = sc->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_request   = vatapi_scsipi_request;
	adapt->adapt_minphys   = vatapi_minphys;
	adapt->adapt_flags     = 0; //SCSIPI_ADAPT_POLL_ONLY;
	sc->sc_atapi_adapter.atapi_probe_device = vatapi_probe_device;

	memset(chan,  0, sizeof(*chan));
	chan->chan_adapter    = adapt;
	chan->chan_bustype    = &vatapi_bustype;
	chan->chan_channel    = 0;	/* location */
	chan->chan_flags      = SCSIPI_CHAN_OPENINGS;
	chan->chan_openings   = 1;
	chan->chan_max_periph = 1;
	chan->chan_ntargets   = 1;
	chan->chan_nluns      = 1;

	/* set polling */
	//sc->sc_flags = VATAPI_FLAG_POLLING;

	/* we `discovered' an atapi adapter */
	sc->sc_vatapibus = config_found_ia(sc->sc_dev, "atapi", chan,
		atapiprint);
}


/* XXX why is it be called minphys, when it enforces maxphys? */
static void
vatapi_minphys(struct buf *bp)
{
	if (bp->b_bcount > MAX_SIZE)
		bp->b_bcount = MAX_SIZE;
	minphys(bp);
}

/*
 * ATAPI device probing
 */
static void
vatapi_probe_device(struct atapibus_softc *atapi, int target)
{
	struct scsipi_channel *chan = atapi->sc_channel;
	struct scsipi_periph *periph;
	struct scsipibus_attach_args sa;
	char vendor[33], product[65], revision[17];
	struct scsipi_inquiry_data inqbuf;

	if (target != VDEV_ATAPI_DRIVE)	/* only probe drive 0 */
		return;

	/* skip if already attached */
	if (scsipi_lookup_periph(chan, target, 0) != NULL)
		return;

	/* allocate and set up periph structure */
	periph = scsipi_alloc_periph(M_NOWAIT);
	if (periph == NULL) {
		aprint_error_dev(atapi->sc_dev,
		    "can't allocate link for drive %d\n", target);
		return;
	}
	periph->periph_channel = chan;
	periph->periph_switch = &atapi_probe_periphsw;
	periph->periph_target = target;
	periph->periph_quirks = chan->chan_defquirks;

	/* inquiry */
	aprint_verbose("%s: inquiry devices\n", __func__);
	memset(&inqbuf, 0, sizeof(inqbuf));
	if (scsipi_inquire(periph, &inqbuf, XS_CTL_DISCOVERY) != 0) {
		aprint_error_dev(atapi->sc_dev, ": scsipi_inquire failed\n");
		free(periph, M_DEVBUF);
		return;
	}

#define scsipi_strvis(a, al, b, bl) strlcpy(a, b, al)
	scsipi_strvis(vendor, 33, inqbuf.vendor, 8);
	scsipi_strvis(product, 65, inqbuf.product, 16);
	scsipi_strvis(revision, 17, inqbuf.revision, 4);
#undef scsipi_strvis

	sa.sa_periph = periph;
	sa.sa_inqbuf.type = inqbuf.device;
	sa.sa_inqbuf.removable = inqbuf.dev_qual2 & SID_REMOVABLE ?
	    T_REMOV : T_FIXED;
	if (sa.sa_inqbuf.removable)
		periph->periph_flags |= PERIPH_REMOVABLE;
	sa.sa_inqbuf.vendor = vendor;
	sa.sa_inqbuf.product = product;
	sa.sa_inqbuf.revision = revision;
	sa.sa_inqptr = NULL;

	/* ATAPI demands only READ10 and higher IIRC */
	periph->periph_quirks |= PQUIRK_ONLYBIG;

	aprint_verbose(": probedev on vendor '%s' product '%s' revision '%s'\n",
		vendor, product, revision);

	atapi_probe_device(atapi, target, periph, &sa);
	/* atapi_probe_device() frees the periph when there is no device.*/
}

/*
 * Issue a request for a periph.
 */
static void
vatapi_scsipi_request(struct scsipi_channel *chan,
	scsipi_adapter_req_t req, void *arg)
{
	device_t sc_dev = chan->chan_adapter->adapt_dev;
	struct vatapi_softc *sc = device_private(sc_dev);
 	struct scsipi_xfer *xs = arg;

	switch (req) {
 	case ADAPTER_REQ_GROW_RESOURCES:
 	case ADAPTER_REQ_SET_XFER_MODE:
		return;
	case ADAPTER_REQ_RUN_XFER :
		KASSERT(sc->sc_xs == NULL);

		/* queue the command */
		KASSERT(sc->sc_xs == NULL);
		sc->sc_xs = xs;
		softint_schedule(sc->sc_ih);
	}
}


static void
vatapi_report_problem(scsireq_t *kreq)
{
#ifdef SCSIVERBOSE
	printf("vatapi cmd failed: ");
	for (int i = 0; i < kreq->cmdlen; i++) {
		printf("%02x ", kreq->cmd[i]);
	}
	printf("\n");
	scsipi_print_sense_data_real(
		(struct scsi_sense_data *) kreq->sense, 1);
#endif
}


static void
vatapi_complete(void *arg)
{
	struct vatapi_softc *sc = arg;
 	struct scsipi_xfer *xs = sc->sc_xs;
	scsireq_t kreq;

	memset(&kreq, 0, sizeof(kreq));
	memcpy(kreq.cmd, xs->cmd, xs->cmdlen);
	kreq.cmdlen = xs->cmdlen;
	kreq.databuf = xs->data;		/* in virt? */
	kreq.datalen = xs->datalen;
	kreq.timeout = xs->timeout;

	kreq.flags = (xs->xs_control & XS_CTL_DATA_IN) ?
		SCCMD_READ : SCCMD_WRITE;

	kreq.senselen = sizeof(struct scsi_sense_data);

	xs->error = XS_SHORTSENSE;
	/* this is silly, but better make sure */
	thunk_assert_presence((vaddr_t) kreq.databuf,
		(size_t) kreq.datalen);

	if (thunk_ioctl(sc->sc_fd, SCIOCCOMMAND, &kreq) != -1) {
		switch (kreq.retsts) {
		case SCCMD_OK :
			xs->resid = 0;
			xs->error = 0;
			break;
		case SCCMD_TIMEOUT :
			break;
		case SCCMD_BUSY :
			break;
		case SCCMD_SENSE :
			xs->error = XS_SHORTSENSE;	/* ATAPI */
			memcpy(&xs->sense.scsi_sense, kreq.sense,
				sizeof(struct scsi_sense_data));
			vatapi_report_problem(&kreq);
			break;
		default:
			thunk_printf("unhandled/unknown retstst %d\n", kreq.retsts);
			break;
		}
	} else {
		printf("thunk_ioctl == -1, errno %d\n", thunk_geterrno());
	}
	sc->sc_xs = NULL;
	scsipi_done(xs);
}


/* 
 * Kill off all pending xfers for a periph.
 *
 * Must be called at splbio().
 */
static void
vatapi_kill_pending(struct scsipi_periph *periph)
{
	/* do we need to do anything ? (yet?) */
	printf("%s: target %d\n", __func__, periph->periph_target);
}

