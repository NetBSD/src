/*	$NetBSD: fwscsi.c,v 1.2.4.2 2002/03/16 16:01:06 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: fwscsi.c,v 1.2.4.2 2002/03/16 16:01:06 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/buf.h>

#include <machine/bus.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/std/ieee1212reg.h>
#include <dev/ieee1394/sbp2reg.h>
#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/ieee1394reg.h>

/* XXX */

#include <dev/ieee1394/fwnodevar.h>
#include <dev/ieee1394/fwnodereg.h>

static int fwscsi_match(struct device *, struct cfdata *, void *);
static void fwscsi_attach(struct device *, struct device *, void *);
static int fwscsi_detach(struct device *, int);
static void fwscsi_scsipi_request(struct scsipi_channel *, scsipi_adapter_req_t,
    void *);
static void fwscsi_scsipi_minphys(struct buf *);

struct sbp2_orb {
	TAILQ_ENTRY(sbp2_orb) orb_list;
};

struct fwscsi_softc {
	struct device sc_dev;
	struct scsipi_adapter sc_adapter;
        struct scsipi_channel sc_channel;

	u_int8_t sc_speed;
	u_int32_t sc_maxpayload;

	u_int64_t sc_csrbase;
	u_int64_t sc_mgmtreg;
	int sc_loginid;

	struct device *sc_bus;
	
	TAILQ_HEAD(, sbp2_orb) sc_orbs;
};

struct cfattach fwscsi_ca = {
	sizeof(struct fwscsi_softc), fwscsi_match, fwscsi_attach, fwscsi_detach
};

static int
fwscsi_match(struct device *parent, struct cfdata *match, void *aux)
{
	/*struct p1212_dir **udirs = aux;*/
	
	return 0;
}

static void
fwscsi_attach(struct device *parent, struct device *self, void *aux)
{
/*	struct p1212_dir **udirs = aux;*/
	struct ieee1394_softc *psc = (struct ieee1394_softc *)parent;
	struct ieee1394_softc *buspsc =
	    (struct ieee1394_softc *)parent->dv_parent;
	struct fwscsi_softc *sc = (struct fwscsi_softc *)self;

/*	sc->sc_csrbase = devcap->dev_cmdptr;
	sc->sc_loginid = devcap->dev_spec;
	sc->sc_mgmtreg = CSR_BASE + ((u_int32_t *)devcap->dev_data)[0] * 4;*/

	sc->sc_speed = psc->sc1394_link_speed;
	sc->sc_maxpayload = buspsc->sc1394_max_receive - 1;
	
	sc->sc_adapter.adapt_dev = &sc->sc_dev;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_request = fwscsi_scsipi_request;
	sc->sc_adapter.adapt_minphys = fwscsi_scsipi_minphys;
	sc->sc_adapter.adapt_openings = 8; 
	
	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_flags = SCSIPI_CHAN_CANGROW | SCSIPI_CHAN_NOSETTLE;
	sc->sc_channel.chan_ntargets = 2;
	sc->sc_channel.chan_nluns = SBP2_MAX_LUNS;
	sc->sc_channel.chan_id = 1;

	TAILQ_INIT(&sc->sc_orbs);
	
	printf("\n");

	sc->sc_bus = config_found(&sc->sc_dev, &sc->sc_channel, scsiprint);
	
	return;
}

static int
fwscsi_detach(struct device *self, int flags)
{
	struct fwscsi_softc *sc = (struct fwscsi_softc *)self;

	if (sc->sc_bus)
		config_detach(sc->sc_bus, 0);
	return 0;
}

static void
fwscsi_scsipi_request(struct scsipi_channel *channel,
    scsipi_adapter_req_t req, void *arg)
{
	/*struct scsipi_adapter *adapt = channel->chan_adapter;
	       struct fwscsi_softc *sc = (struct fwscsi_softc *)adapt->adapt_dev;*/
	struct scsipi_xfer *xs = arg;
	int i;
	
	printf("Called fwscsi_scsipi_request\n");

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		xs->error = XS_DRIVER_STUFFUP;
		printf("Got req_run_xfer\n");
		printf("xs control: 0x%08x, timeout: %d\n", xs->xs_control,
		    xs->timeout);
		printf("opcode: 0x%02x\n", (int)xs->cmd->opcode);
		for (i = 0; i < 15; i++)
			printf("0x%02x ",(int)xs->cmd->bytes[i]);
		printf("\n");
		scsipi_done(xs);
		break;
	case ADAPTER_REQ_GROW_RESOURCES:
		printf("Got req_grow_resources\n");
		break;
	case ADAPTER_REQ_SET_XFER_MODE:
		printf("Got set xfer mode\n");
		break;
	default:
		panic("Unknown request: %d\n", (int)req);
	}
}

static void
fwscsi_scsipi_minphys(struct buf *buf)
{
	if (buf->b_bcount > SBP2_MAX_TRANS)
		buf->b_bcount = SBP2_MAX_TRANS;
	minphys(buf);
}
