/*	$NetBSD: umassbus.c,v 1.1 2001/04/13 12:24:10 augustss Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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


#include "atapibus.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>


#include <dev/usb/umassbus.h>
#include <dev/usb/umassvar.h>


#define UMASS_SCSIID_HOST	0x00
#define UMASS_SCSIID_DEVICE	0x01

#define UMASS_ATAPI_DRIVE	0

struct scsipi_device umass_dev =
{
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

Static int umass_scsipi_cmd(struct scsipi_xfer *xs);
Static void umass_scsipi_minphys(struct buf *bp);
Static int umass_scsipi_ioctl(struct scsipi_link *, u_long,
				   caddr_t, int, struct proc *);
Static int umass_scsipi_getgeom(struct scsipi_link *link,
			      struct disk_parms *, u_long sectors);

Static void umass_scsipi_cb(struct umass_softc *sc, void *priv,
				     int residue, int status);
Static void umass_scsipi_sense_cb(struct umass_softc *sc, void *priv,
				       int residue, int status);

Static int scsipiprint(void *aux, const char *pnp);
#if NATAPIBUS > 0
Static void umass_atapi_probedev(struct atapibus_softc *, int);
#endif


int
umass_attach_bus(struct umass_softc *sc)
{
	/*
	 * Fill in the adapter.
	 */
	sc->bus.sc_adapter.scsipi_cmd = umass_scsipi_cmd;
	sc->bus.sc_adapter.scsipi_minphys = umass_scsipi_minphys;
	sc->bus.sc_adapter.scsipi_ioctl = umass_scsipi_ioctl;
	sc->bus.sc_adapter.scsipi_getgeom = umass_scsipi_getgeom;
	
	/*
	 * fill in the prototype scsipi_link.
	 */
	switch (sc->proto & PROTO_COMMAND) {
	case PROTO_RBC:
	case PROTO_SCSI:
		sc->bus.u.sc_link.type = BUS_SCSI;
		sc->bus.u.sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
		sc->bus.u.sc_link.adapter_softc = sc;
		sc->bus.u.sc_link.scsipi_scsi.adapter_target = UMASS_SCSIID_HOST;
		sc->bus.u.sc_link.adapter = &sc->bus.sc_adapter;
		sc->bus.u.sc_link.device = &umass_dev;
		sc->bus.u.sc_link.openings = 1;
		sc->bus.u.sc_link.scsipi_scsi.max_target = UMASS_SCSIID_DEVICE;
		sc->bus.u.sc_link.scsipi_scsi.max_lun = sc->maxlun;

		break;

#if NATAPIBUS > 0
	case PROTO_UFI:
	case PROTO_ATAPI:
		sc->bus.u.aa.sc_aa.aa_type = T_ATAPI;
		sc->bus.u.aa.sc_aa.aa_channel = 0;
		sc->bus.u.aa.sc_aa.aa_openings = 1;
		sc->bus.u.aa.sc_aa.aa_drv_data = &sc->bus.u.aa.sc_aa_drive;
		sc->bus.u.aa.sc_aa.aa_bus_private = &sc->bus.sc_atapi_adapter;
		sc->bus.sc_atapi_adapter.atapi_probedev = umass_atapi_probedev;
		sc->bus.sc_atapi_adapter.atapi_kill_pending = scsi_kill_pending;

		if (sc->quirks & NO_TEST_UNIT_READY)
			sc->bus.u.sc_link.quirks |= ADEV_NOTUR;
		break;
#endif

	default:
		printf("%s: proto=0x%x not supported yet\n", 
		       USBDEVNAME(sc->sc_dev), sc->proto);
		return (1);
	}

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	sc->bus.sc_child = config_found(&sc->sc_dev, &sc->bus.u, scsipiprint);
	if (sc->bus.sc_child == NULL) {
		/* Not an error, just not a complete success. */
		return (1);
	}

	return (0);
}

Static int
scsipiprint(void *aux, const char *pnp)
{
	struct scsipi_link *l = aux;

	if (l->type == BUS_SCSI)
		return (scsiprint(aux, pnp));
	else {
#if NATAPIBUS > 0
		struct ata_atapi_attach *aa_link = aux;
#endif
		if (pnp)
			printf("atapibus at %s", pnp);
#if NATAPIBUS > 0
		printf(" channel %d", aa_link->aa_channel);
#endif
		return (UNCONF);
	}
}

int
umass_detach_bus(struct umass_softc *sc, int flags)
{
	if (sc->bus.sc_child != NULL)
		return config_detach(sc->bus.sc_child, flags);
	else
		return 0;
}

int
umass_activate(struct device *self, enum devact act)
{
	struct umass_softc *sc = (struct umass_softc *) self;
	int rv = 0;

	DPRINTF(UDMASS_USB, ("%s: umass_activate: %d\n",
	    USBDEVNAME(sc->sc_dev), act));

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if (sc->bus.sc_child == NULL)
			break;
		rv = config_deactivate(sc->bus.sc_child);
		DPRINTF(UDMASS_USB, ("%s: umass_activate: child "
		    "returned %d\n", USBDEVNAME(sc->sc_dev), rv));
		if (rv == 0)
			sc->sc_dying = 1;
		break;
	}
	return (rv);
}

Static int
umass_scsipi_cmd(struct scsipi_xfer *xs)
{
	struct scsipi_link *sc_link = xs->sc_link;
	struct umass_softc *sc = sc_link->adapter_softc;
	struct scsipi_generic *cmd, trcmd;
	int cmdlen;
	int dir;
#ifdef UMASS_DEBUG
	microtime(&sc->tv);
#endif

	DIF(UDMASS_UPPER, sc_link->flags |= DEBUGLEVEL);

	DPRINTF(UDMASS_CMD, ("%s: umass_scsi_cmd: at %lu.%06lu: %d:%d "
	    "xs=%p cmd=0x%02x datalen=%d (quirks=0x%x, poll=%d)\n",
	    USBDEVNAME(sc->sc_dev), sc->tv.tv_sec, sc->tv.tv_usec,
	    sc_link->scsipi_scsi.target, sc_link->scsipi_scsi.lun,
	    xs, xs->cmd->opcode, xs->datalen,
	    sc_link->quirks, xs->xs_control & XS_CTL_POLL));
#if defined(USB_DEBUG) && defined(SCSIDEBUG)
	if (umassdebug & UDMASS_SCSI)
		show_scsipi_xs(xs);
	else if (umassdebug & ~UDMASS_CMD)
		show_scsipi_cmd(xs);
#endif

	if (sc->sc_dying) {
		xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}

#ifdef UMASS_DEBUG
	if (sc_link->type == BUS_ATAPI ? 
	    sc_link->scsipi_atapi.drive != UMASS_ATAPI_DRIVE : 
	    sc_link->scsipi_scsi.target != UMASS_SCSIID_DEVICE) {
		DPRINTF(UDMASS_SCSI, ("%s: wrong SCSI ID %d\n",
		    USBDEVNAME(sc->sc_dev),
		    sc_link->scsipi_scsi.target));
		xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}
#endif

	/* XXX should use transform */

	if (xs->cmd->opcode == START_STOP &&
	    (sc->quirks & NO_START_STOP)) {
		/*printf("%s: START_STOP\n", USBDEVNAME(sc->sc_dev));*/
		xs->error = XS_NOERROR;
		goto done;
	}

	if (xs->cmd->opcode == INQUIRY &&
	    (sc->quirks & FORCE_SHORT_INQUIRY)) {
		/* some drives wedge when asked for full inquiry information. */
		memcpy(&trcmd, cmd, sizeof trcmd);
		trcmd.bytes[4] = SHORT_INQUIRY_LENGTH;
		cmd = &trcmd;
	}

	dir = DIR_NONE;
	if (xs->datalen) {
		switch (xs->xs_control & (XS_CTL_DATA_IN | XS_CTL_DATA_OUT)) {
		case XS_CTL_DATA_IN:
			dir = DIR_IN;
			break;
		case XS_CTL_DATA_OUT:
			dir = DIR_OUT;
			break;
		}
	}

	if (xs->datalen > UMASS_MAX_TRANSFER_SIZE) {
		printf("umass_cmd: large datalen, %d\n", xs->datalen);
		xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}

	cmd = xs->cmd;
	cmdlen = xs->cmdlen;

	if (xs->xs_control & XS_CTL_POLL) {
		/* Use sync transfer. XXX Broken! */
		DPRINTF(UDMASS_SCSI, ("umass_scsi_cmd: sync dir=%d\n", dir));
		sc->sc_xfer_flags = USBD_SYNCHRONOUS;
		sc->bus.sc_sync_status = USBD_INVAL;
		sc->transfer(sc, sc_link->scsipi_scsi.lun, cmd, cmdlen,
			     xs->data, xs->datalen, dir, 0, xs);
		sc->sc_xfer_flags = 0;
		DPRINTF(UDMASS_SCSI, ("umass_scsi_cmd: done err=%d\n", 
				      sc->sc_sync_status));
		switch (sc->bus.sc_sync_status) {
		case USBD_NORMAL_COMPLETION:
			xs->error = XS_NOERROR;
			break;
		case USBD_TIMEOUT:
			xs->error = XS_TIMEOUT;
			break;
		default:
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}
		goto done;
	} else {
		DPRINTF(UDMASS_SCSI, ("umass_scsi_cmd: async dir=%d, cmdlen=%d"
				      " datalen=%d\n",
				      dir, cmdlen, xs->datalen));
		sc->transfer(sc, sc_link->scsipi_scsi.lun, cmd, cmdlen,
		    xs->data, xs->datalen, dir, umass_scsipi_cb, xs);
		return (SUCCESSFULLY_QUEUED);
	}

	/* Return if command finishes early. */
 done:
	xs->xs_status |= XS_STS_DONE;
	scsipi_done(xs);
	if (xs->xs_control & XS_CTL_POLL)
		return (COMPLETE);
	else
		return (SUCCESSFULLY_QUEUED);
}

Static void
umass_scsipi_minphys(struct buf *bp)
{
#ifdef DIAGNOSTIC
	if (bp->b_bcount <= 0) {
		printf("umass_scsipi_minphys count(%ld) <= 0\n",
		       bp->b_bcount);
		bp->b_bcount = UMASS_MAX_TRANSFER_SIZE;
	}
#endif
	if (bp->b_bcount > UMASS_MAX_TRANSFER_SIZE)
		bp->b_bcount = UMASS_MAX_TRANSFER_SIZE;
	minphys(bp);
}

int
umass_scsipi_ioctl(struct scsipi_link *link, u_long cmd, caddr_t arg,
		   int flag, struct proc *p)
{
	/*struct umass_softc *sc = link->adapter_softc;*/

	switch (cmd) {
#if 0
	case SCBUSIORESET:
		ccb->ccb_h.status = CAM_REQ_INPROG;
		umass_reset(sc, umass_cam_cb, (void *) ccb);
		return (0);
#endif
	default:
		return (ENOTTY);
	}
}

Static int
umass_scsipi_getgeom(struct scsipi_link *sc_link, struct disk_parms *dp,
		     u_long sectors)
{
	struct umass_softc *sc = sc_link->adapter_softc;

	/* If it's not a floppy, we don't know what to do. */
	if (!(sc->proto & PROTO_UFI))
		return (0);

	switch (sectors) {
	case 1440:
		/* Most likely a single density 3.5" floppy. */
		dp->heads = 2;
		dp->sectors = 9;
		dp->cyls = 80;
		return (1);
	case 2880:
		/* Most likely a double density 3.5" floppy. */
		dp->heads = 2;
		dp->sectors = 18;
		dp->cyls = 80;
		return (1);
	default:
		return (0);
	}
}

Static void
umass_scsipi_cb(struct umass_softc *sc, void *priv, int residue, int status)
{
	struct scsipi_xfer *xs = priv;
	struct scsipi_link *sc_link = xs->sc_link;
	int cmdlen;
	int s;
#ifdef UMASS_DEBUG
	struct timeval tv;
	u_int delta;
	microtime(&tv);
	delta = (tv.tv_sec - sc->tv.tv_sec) * 1000000 + tv.tv_usec - sc->tv.tv_usec;
#endif

	DPRINTF(UDMASS_CMD,("umass_scsipi_cb: at %lu.%06lu, delta=%u: xs=%p residue=%d"
	    " status=%d\n", tv.tv_sec, tv.tv_usec, delta, xs, residue, status));

	xs->resid = residue;

	switch (status) {
	case STATUS_CMD_OK:
		xs->error = XS_NOERROR;
		break;

	case STATUS_CMD_UNKNOWN:
	case STATUS_CMD_FAILED:
		/* fetch sense data */
		memset(&sc->bus.sc_sense_cmd, 0, sizeof(sc->bus.sc_sense_cmd));
		sc->bus.sc_sense_cmd.opcode = REQUEST_SENSE;
		sc->bus.sc_sense_cmd.byte2 = sc_link->scsipi_scsi.lun <<
		    SCSI_CMD_LUN_SHIFT;
		sc->bus.sc_sense_cmd.length = sizeof(xs->sense);

		cmdlen = sizeof(sc->bus.sc_sense_cmd);
		if (sc->proto & PROTO_UFI) /* XXX */
			cmdlen = UFI_COMMAND_LENGTH;
		sc->transfer(sc, sc_link->scsipi_scsi.lun,
			     &sc->bus.sc_sense_cmd, cmdlen,
			     &xs->sense, sizeof(xs->sense), DIR_IN,
			     umass_scsipi_sense_cb, xs);
		return;

	case STATUS_WIRE_FAILED:
		xs->error = XS_RESET;
		break;

	default:
		panic("%s: Unknown status %d in umass_scsipi_cb\n",
			USBDEVNAME(sc->sc_dev), status);
	}

	xs->xs_status |= XS_STS_DONE;

	DPRINTF(UDMASS_CMD,("umass_scsipi_cb: at %lu.%06lu: return xs->error="
            "%d, xs->xs_status=0x%x xs->resid=%d\n",
	     tv.tv_sec, tv.tv_usec,
	     xs->error, xs->xs_status, xs->resid));

	s = splbio();
	scsipi_done(xs);
	splx(s);
}

/* 
 * Finalise a completed autosense operation
 */
Static void
umass_scsipi_sense_cb(struct umass_softc *sc, void *priv, int residue,
		      int status)
{
	struct scsipi_xfer *xs = priv;
	int s;

	DPRINTF(UDMASS_CMD,("umass_scsipi_sense_cb: xs=%p residue=%d "
		"status=%d\n", xs, residue, status));

	switch (status) {
	case STATUS_CMD_OK:
	case STATUS_CMD_UNKNOWN:
		/* getting sense data succeeded */
		if (xs->cmd->opcode == INQUIRY && (xs->resid < xs->datalen
		    || ((sc->quirks & RS_NO_CLEAR_UA) /* XXX */) )) {
			/*
			 * Some drivers return SENSE errors even after INQUIRY.
			 * The upper layer doesn't like that.
			 */
			xs->error = XS_NOERROR;
			break;
		}
		/* XXX look at residue */
		if (residue == 0 || residue == 14)/* XXX */
			xs->error = XS_SENSE;
		else
			xs->error = XS_SHORTSENSE;
		break;
	default:
		DPRINTF(UDMASS_SCSI, ("%s: Autosense failed, status %d\n",
			USBDEVNAME(sc->sc_dev), status));
		xs->error = XS_DRIVER_STUFFUP;
		break;
	}

	xs->xs_status |= XS_STS_DONE;

	DPRINTF(UDMASS_CMD,("umass_scsipi_sense_cb: return xs->error=%d, "
		"xs->xs_status=0x%x xs->resid=%d\n", xs->error, xs->xs_status,
		xs->resid));

	s = splbio();
	scsipi_done(xs);
	splx(s);
}

#if NATAPIBUS > 0
Static void
umass_atapi_probedev(struct atapibus_softc *atapi, int target)
{
	struct scsipi_link *sc_link;
	struct scsipibus_attach_args sa;
	struct ata_drive_datas *drvp = &atapi->sc_drvs[target];
	char vendor[33], product[65], revision[17];
	struct scsipi_inquiry_data inqbuf;

	DPRINTF(UDMASS_SCSI,("umass_atapi_probedev: atapi=%p target=%d\n",
			     atapi, target));

	if (target != UMASS_ATAPI_DRIVE)	/* only probe drive 0 */
		return;

	if (atapi->sc_link[target])
		return;

	sc_link = malloc(sizeof(*sc_link), M_DEVBUF, M_NOWAIT); 
	if (sc_link == NULL) {
		printf("%s: can't allocate link for drive %d\n",
		       atapi->sc_dev.dv_xname, target);
		return;       
	}
	*sc_link = *atapi->adapter_link;

	DIF(UDMASS_UPPER, sc_link->flags |= DEBUGLEVEL);

	/* Fill generic parts of the link. */
	sc_link->active = 0;
	sc_link->scsipi_atapi.drive = target;
	sc_link->device = &umass_dev;
	TAILQ_INIT(&sc_link->pending_xfers);

	DPRINTF(UDMASS_SCSI, ("umass_atapi_probedev: doing inquiry\n"));
	/* Now go ask the device all about itself. */
	memset(&inqbuf, 0, sizeof(inqbuf));
	if (scsipi_inquire(sc_link, &inqbuf, XS_CTL_DISCOVERY) != 0) {
		DPRINTF(UDMASS_SCSI, ("umass_atapi_probedev: scsipi_inquire "
				      "failed\n"));
		free(sc_link, M_DEVBUF);
		return;
	}

	scsipi_strvis(vendor, 33, inqbuf.vendor, 8);
	scsipi_strvis(product, 65, inqbuf.product, 16);
	scsipi_strvis(revision, 17, inqbuf.revision, 4);

	sa.sa_sc_link = sc_link;
	sa.sa_inqbuf.type = inqbuf.device;
	sa.sa_inqbuf.removable = inqbuf.dev_qual2 & SID_REMOVABLE ?
	    T_REMOV : T_FIXED;
	if (sa.sa_inqbuf.removable)
		sc_link->flags |= SDEV_REMOVABLE;
	/* XXX how? sc_link->scsipi_atapi.cap |= ACAP_LEN;*/
	sa.sa_inqbuf.vendor = vendor;
	sa.sa_inqbuf.product = product;
	sa.sa_inqbuf.revision = revision;
	sa.sa_inqptr = NULL;

	DPRINTF(UDMASS_SCSI, ("umass_atapi_probedev: doing atapi_probedev on "
			      "'%s' '%s' '%s'\n", vendor, product, revision));
	drvp->drv_softc = atapi_probedev(atapi, target, sc_link, &sa);
	/* atapi_probedev() frees the scsipi_link when there is no device. */
}
#endif
