/*	$NetBSD: sbpscsi.c,v 1.8 2005/02/21 00:29:07 thorpej Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: sbpscsi.c,v 1.8 2005/02/21 00:29:07 thorpej Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/limits.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/sbp2reg.h>
#include <dev/ieee1394/sbp2var.h>
#include <dev/ieee1394/sbpscsireg.h>
#include <dev/ieee1394/sbpscsivar.h>

#ifdef SBPSCSI_DEBUG
#define DPRINTF(x)      if (sbpscsidebug) printf x
#define DPRINTFN(n,x)   if (sbpscsidebug>(n)) printf x
int     sbpscsidebug = 3;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

static int sbpscsi_match(struct device *, struct cfdata *, void *);
static void sbpscsi_attach(struct device *, struct device *, void *);
static int sbpscsi_detach(struct device *, int);
static void sbpscsi_scsipi_request(struct scsipi_channel *,
    scsipi_adapter_req_t, void *);
static void sbpscsi_status(struct sbp2_status *, void *);
static void sbpscsi_timeout(void *);
static void sbpscsi_minphys(struct buf *);

CFATTACH_DECL(sbpscsi, sizeof(struct sbpscsi_softc),
    sbpscsi_match, sbpscsi_attach, sbpscsi_detach, NULL);

static int
sbpscsi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct p1212_dir **udirs = aux;
	struct sbp2* sbp2;
	struct sbp2_lun *lun;
	int ret;

	sbp2 = NULL;
	ret = 0;
	if (sbp2_match(*udirs)) {
		sbp2 = sbp2_init((struct ieee1394_softc *) parent, *udirs);
		TAILQ_FOREACH(lun, &sbp2->luns, lun_list) {
			if ((lun->cmd_spec_id == SBPSCSI_COMMAND_SET_SPEC_ID) &&
			    (lun->cmd_set == SBPSCSI_COMMAND_SET))
				ret = 1;
		}
	}
	if (sbp2)
		sbp2_free(sbp2);
	return ret;
}

static void
sbpscsi_attach(struct device *parent, struct device *self, void *aux)
{
	struct p1212_dir **udirs = aux;
	struct sbpscsi_softc *sc = (struct sbpscsi_softc *)self;
	struct ieee1394_softc *psc = (struct ieee1394_softc *) parent;
	int found;

	found = 0;

	if (sbp2_match(*udirs)) {
		sc->sbp2 = sbp2_init(psc, *udirs);
		found = 1;
	}
	
	if (!found) {
		DPRINTF(("Can't match an SBP capable scsi lun?"));
		return;
	}		
	
	sc->sc_adapter.adapt_dev = &sc->sc_dev;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_request = sbpscsi_scsipi_request;
	sc->sc_adapter.adapt_minphys = sbpscsi_minphys;
	sc->sc_adapter.adapt_openings = 8; /*Start with some. Grow as needed.*/
	
	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_defquirks = PQUIRK_ONLYBIG;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_flags = SCSIPI_CHAN_CANGROW | SCSIPI_CHAN_NOSETTLE;

	sc->sc_channel.chan_ntargets = 1;
	sc->sc_channel.chan_nluns = sc->sbp2->luncnt;
	sc->sc_channel.chan_id = 1;

	printf("\n");

	sc->sc_bus = config_found(&sc->sc_dev, &sc->sc_channel, scsiprint);
	
	return;
}

static int
sbpscsi_detach(struct device *self, int flags)
{
	struct sbpscsi_softc *sc = (struct sbpscsi_softc *)self;

	if (sc->sbp2)
		sbp2_free(sc->sbp2);
	if (sc->sc_bus)
		config_detach(sc->sc_bus, 0);
	return 0;
}

static void
sbpscsi_scsipi_request(struct scsipi_channel *channel, scsipi_adapter_req_t req,
    void *arg)
{
	int i;
	void *handle;
	struct sbpscsi_softc *sc =
	    (struct sbpscsi_softc *)channel->chan_adapter->adapt_dev;
	struct sbp2 *sbp2 = sc->sbp2;
	struct scsipi_xfer *xs = arg;
	struct sbp2_cmd cmd;
	
	DPRINTFN(1, ("Called sbpscsi_scsipi_request\n"));

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		DPRINTFN(1, ("Got req_run_xfer\n"));
		DPRINTFN(1, ("xs control: 0x%08x, timeout: %d\n", 
		    xs->xs_control, xs->timeout));
		DPRINTFN(1, ("opcode: 0x%02x\n", (int)xs->cmd->opcode));
		for (i = 0; i < 15; i++)
			DPRINTFN(1, ("0x%02x ",(int)xs->cmd->bytes[i]));
		DPRINTFN(1, ("\n"));
		if (xs->xs_control & XS_CTL_RESET) {
			sbp2_reset_lun(sbp2, xs->xs_periph->periph_lun);
			return;
		}
		if (xs->cmdlen > SBPSCSI_SBP2_MAX_CDB) {
			DPRINTF(("sbp doesn't support cdb's larger than %d "
				    "bytes\n", SBPSCSI_SBP2_MAX_CDB));
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
			return;
		}
		cmd.lun = xs->xs_periph->periph_lun;
		cmd.data = xs->data;
		cmd.datalen = xs->datalen;
		memset(cmd.cmd, 0, 12);
		cmd.cmd[0] = xs->cmd->opcode;
		for (i = 0; i < xs->cmdlen; i++)
			cmd.cmd[i + 1] = xs->cmd->bytes[i];
		cmd.cmdlen = 12;
		if (xs->xs_control & XS_CTL_DATA_IN)
			cmd.rw = SBP_WRITE;
		if (xs->xs_control & XS_CTL_DATA_OUT)
			cmd.rw = SBP_READ;
		cmd.cb = sbpscsi_status;
		cmd.cb_arg = xs;
		handle = sbp2_runcmd(sbp2, &cmd);
		if (handle)
			callout_reset(&xs->xs_callout, mstohz(xs->timeout),
			    sbpscsi_timeout, handle);
		else {
			DPRINTF(("Got an error from sbp2_runcmd\n"));
			xs->error = XS_DRIVER_STUFFUP;
			scsipi_done(xs);
		}
		break;
	case ADAPTER_REQ_GROW_RESOURCES:
		DPRINTFN(1, ("Got req_grow_resources\n"));
		break;
	case ADAPTER_REQ_SET_XFER_MODE:
		DPRINTFN(1, ("Got set xfer mode\n"));
		break;
	default:
		panic("Unknown request: %d\n", (int)req);
	}
}

static void
sbpscsi_timeout(void *arg)
{
	struct scsipi_xfer *xs;

	xs = sbp2_abort(arg);

	xs->error = XS_TIMEOUT;
	scsipi_done(xs);
}

static void
sbpscsi_status(struct sbp2_status *status, void *arg)
{
	struct scsipi_xfer *xs = arg;
	u_int8_t smft, vflag, iflag, mflag, eflag;
	
	callout_stop(&xs->xs_callout);

	DPRINTFN(1, ("status: resp 0x%04x, sbp_status 0x%04x\n", status->resp,
	    status->sbp_status));

	if ((status->resp != SBP2_STATUS_REQUEST_COMPLETE) ||
	    (status->sbp_status != SBP2_STATUS_NOERROR)) {
#ifdef SBPSCSI_DEBUG
#endif
	} else {
		xs->error = XS_NOERROR;
		xs->resid = 0;
	}
	
	if (status->datalen) {
		xs->sense.scsi_sense.response_code =
		    SBPSCSI_STATUS_GET_STATUS(status->data[0]);
		vflag = SBPSCSI_STATUS_GET_VFLAG(status->data[0]);
		mflag = SBPSCSI_STATUS_GET_MFLAG(status->data[0]);
		eflag = SBPSCSI_STATUS_GET_EFLAG(status->data[0]);
		iflag = SBPSCSI_STATUS_GET_IFLAG(status->data[0]);
		smft = SBPSCSI_STATUS_GET_SMFT(status->data[0]);
		
		if (iflag)
			xs->sense.scsi_sense.flags |= SSD_ILI;
		if (eflag)
			xs->sense.scsi_sense.flags |= SSD_EOM;
		if (mflag)
			xs->sense.scsi_sense.flags |= SSD_FILEMARK;
		xs->sense.scsi_sense.flags |=
		    SBPSCSI_STATUS_GET_SENSEKEY(status->data[0]);
		if (((smft == 0) || (smft == 1)) && vflag) {
			xs->sense.scsi_sense.info[0] =
			    (status->data[1] & 0xff000000) >> 24;
			xs->sense.scsi_sense.info[1] =
			    (status->data[1] & 0x00ff0000) >> 16;
			xs->sense.scsi_sense.info[2] =
			    (status->data[1] & 0x0000ff00) >> 8;
			xs->sense.scsi_sense.info[3] =
			    status->data[1] & 0x000000ff;
		}
		xs->sense.scsi_sense.csi[0] =
		    (status->data[2] & 0xff000000) >> 24;
		xs->sense.scsi_sense.csi[1] =
		    (status->data[2] & 0x00ff0000) >> 16;
		xs->sense.scsi_sense.csi[2] =
		    (status->data[2] & 0x0000ff00) >> 8;
		xs->sense.scsi_sense.csi[3] =
		    status->data[2] & 0x000000ff;
		xs->sense.scsi_sense.asc =
		    SBPSCSI_STATUS_GET_SENSECODE(status->data[0]);
		xs->sense.scsi_sense.ascq =
		    SBPSCSI_STATUS_GET_SENSEQUAL(status->data[0]);
		xs->sense.scsi_sense.fru =
		    SBPSCSI_STATUS_GET_FRU(status->data[3]);
		xs->sense.scsi_sense.sks.sks_bytes[0] =
		    SBPSCSI_STATUS_GET_SENSE1(status->data[3]);
		xs->sense.scsi_sense.sks.sks_bytes[1] =
		    SBPSCSI_STATUS_GET_SENSE2(status->data[3]);
		xs->sense.scsi_sense.sks.sks_bytes[3] =
		    SBPSCSI_STATUS_GET_SENSE3(status->data[3]);
	}
	scsipi_done(xs);
}

static void
sbpscsi_minphys(bp)
        struct buf *bp;
{

	minphys(bp);
}
