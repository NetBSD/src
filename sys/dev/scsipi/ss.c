/*	$NetBSD: ss.c,v 1.46 2003/01/21 00:24:20 simonb Exp $	*/

/*
 * Copyright (c) 1995 Kenneth Stailey.  All rights reserved.
 *   modified for configurable scanner support by Joachim Koenig
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
 *	This product includes software developed by Kenneth Stailey.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ss.c,v 1.46 2003/01/21 00:24:20 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/scanio.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_scanner.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/ssvar.h>

#include <dev/scsipi/ss_mustek.h>

#define SSMODE(z)	( minor(z)       & 0x03)
#define SSUNIT(z)	((minor(z) >> 4)       )
#define SSNMINOR 16

/*
 * If the mode is 3 (e.g. minor = 3,7,11,15)
 * then the device has been openned to set defaults
 * This mode does NOT ALLOW I/O, only ioctls
 */
#define MODE_REWIND	0
#define MODE_NONREWIND	1
#define MODE_CONTROL	3

int ssmatch(struct device *, struct cfdata *, void *);
void ssattach(struct device *, struct device *, void *);
int ssdetach(struct device *self, int flags);
int ssactivate(struct device *self, enum devact act);

CFATTACH_DECL(ss, sizeof(struct ss_softc),
    ssmatch, ssattach, ssdetach, ssactivate);

extern struct cfdriver ss_cd;

dev_type_open(ssopen);
dev_type_close(ssclose);
dev_type_read(ssread);
dev_type_ioctl(ssioctl);

const struct cdevsw ss_cdevsw = {
	ssopen, ssclose, ssread, nowrite, ssioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

void    ssstrategy __P((struct buf *));
void    ssstart __P((struct scsipi_periph *));
void	ssminphys __P((struct buf *));

const struct scsipi_periphsw ss_switch = {
	NULL,
	ssstart,
	NULL,
	NULL,
};

struct scsipi_inquiry_pattern ss_patterns[] = {
	{T_SCANNER, T_FIXED,
	 "",         "",                 ""},
	{T_SCANNER, T_REMOV,
	 "",         "",                 ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C1130A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C1750A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C2500A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C2520A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C5110A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "C7670A          ", ""},
	{T_PROCESSOR, T_FIXED,
	 "HP      ", "", ""},
};

int
ssmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)ss_patterns, sizeof(ss_patterns) / sizeof(ss_patterns[0]),
	    sizeof(ss_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 * If it is a know special, call special attach routine to install
 * special handlers into the ss_softc structure
 */
void
ssattach(struct device *parent, struct device *self, void *aux)
{
	struct ss_softc *ss = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("ssattach: "));

	ss->flags |= SSF_AUTOCONF;

	/*
	 * Store information needed to contact our base driver
	 */
	ss->sc_periph = periph;
	periph->periph_dev = &ss->sc_dev;
	periph->periph_switch = &ss_switch;

	printf("\n");

	/*
	 * Set up the buf queue for this device
	 */
	bufq_alloc(&ss->buf_queue, BUFQ_FCFS);

	/*
	 * look for non-standard scanners with help of the quirk table
	 * and install functions for special handling
	 */
	SC_DEBUG(periph, SCSIPI_DB2, ("ssattach:\n"));
	if (memcmp(sa->sa_inqbuf.vendor, "MUSTEK", 6) == 0)
		mustek_attach(ss, sa);
	if (memcmp(sa->sa_inqbuf.vendor, "HP      ", 8) == 0 &&
	    memcmp(sa->sa_inqbuf.product, "ScanJet 5300C", 13) != 0)
		scanjet_attach(ss, sa);
	if (ss->special == NULL) {
		/* XXX add code to restart a SCSI2 scanner, if any */
	}

	ss->flags &= ~SSF_AUTOCONF;
}

int
ssdetach(struct device *self, int flags)
{
	struct ss_softc *ss = (struct ss_softc *) self;
	struct buf *bp;
	int s, cmaj, mn;

	/* locate the major number */
	cmaj = cdevsw_lookup_major(&ss_cdevsw);

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_GET(&ss->buf_queue)) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	bufq_free(&ss->buf_queue);

	/* Kill off any pending commands. */
	scsipi_kill_pending(ss->sc_periph);

	splx(s);

	/* Nuke the vnodes for any open instances */
	mn = SSUNIT(self->dv_unit);
	vdevgone(cmaj, mn, mn+SSNMINOR-1, VCHR);

	return (0);
}

int
ssactivate(struct device *self, enum devact act)
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVE.
		 */
		break;
	}
	return (rv);
}

/*
 * open the device.
 */
int
ssopen(dev_t dev, int flag, int mode, struct proc *p)
{
	int unit;
	u_int ssmode;
	int error;
	struct ss_softc *ss;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;

	unit = SSUNIT(dev);
	if (unit >= ss_cd.cd_ndevs)
		return (ENXIO);
	ss = ss_cd.cd_devs[unit];
	if (!ss)
		return (ENXIO);

	if ((ss->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	ssmode = SSMODE(dev);

	periph = ss->sc_periph;
	adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(periph, SCSIPI_DB1, ("open: dev=0x%x (unit %d (of %d))\n", dev,
	    unit, ss_cd.cd_ndevs));

	if (periph->periph_flags & PERIPH_OPEN) {
		printf("%s: already open\n", ss->sc_dev.dv_xname);
		return (EBUSY);
	}

	if ((error = scsipi_adapter_addref(adapt)) != 0)
		return (error);

	/*
	 * Catch any unit attention errors.
	 *
	 * XS_CTL_IGNORE_MEDIA_CHANGE: when you have an ADF, some scanners
	 * consider paper to be a changeable media
	 *
	 */
	error = scsipi_test_unit_ready(periph,
	    XS_CTL_IGNORE_MEDIA_CHANGE | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    (ssmode == MODE_CONTROL ? XS_CTL_IGNORE_NOT_READY : 0));
	if (error)
		goto bad;

	periph->periph_flags |= PERIPH_OPEN;	/* unit attn now errors */

	/*
	 * If the mode is 3 (e.g. minor = 3,7,11,15)
	 * then the device has been opened to set defaults
	 * This mode does NOT ALLOW I/O, only ioctls
	 */
	if (ssmode == MODE_CONTROL)
		return (0);

	SC_DEBUG(periph, SCSIPI_DB2, ("open complete\n"));
	return (0);

bad:
	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;
	return (error);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
ssclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct ss_softc *ss = ss_cd.cd_devs[SSUNIT(dev)];
	struct scsipi_periph *periph = ss->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int error;

	SC_DEBUG(ss->sc_periph, SCSIPI_DB1, ("closing\n"));

	if (SSMODE(dev) == MODE_REWIND) {
		if (ss->special && ss->special->rewind_scanner) {
			/* call special handler to rewind/abort scan */
			error = (ss->special->rewind_scanner)(ss);
			if (error)
				return (error);
		} else {
			/* XXX add code to restart a SCSI2 scanner, if any */
		}
		ss->sio.scan_window_size = 0;
		ss->flags &= ~SSF_TRIGGERED;
	}

	scsipi_wait_drain(periph);

	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;

	return (0);
}

/*
 * trim the size of the transfer if needed,
 * called by physio
 * basically the smaller of our min and the scsi driver's
 * minphys
 */
void
ssminphys(struct buf *bp)
{
	struct ss_softc *ss = ss_cd.cd_devs[SSUNIT(bp->b_dev)];
	struct scsipi_periph *periph = ss->sc_periph;

	(*periph->periph_channel->chan_adapter->adapt_minphys)(bp);

	/*
	 * trim the transfer further for special devices this is
	 * because some scanners only read multiples of a line at a
	 * time, also some cannot disconnect, so the read must be
	 * short enough to happen quickly
	 */
	if (ss->special && ss->special->minphys)
		(ss->special->minphys)(ss, bp);
}

/*
 * Do a read on a device for a user process.
 * Prime scanner at start of read, check uio values, call ssstrategy
 * via physio for the actual transfer.
 */
int
ssread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct ss_softc *ss = ss_cd.cd_devs[SSUNIT(dev)];
	int error;

	if ((ss->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	/* if the scanner has not yet been started, do it now */
	if (!(ss->flags & SSF_TRIGGERED)) {
		if (ss->special && ss->special->trigger_scanner) {
			error = (ss->special->trigger_scanner)(ss);
			if (error)
				return (error);
		}
		ss->flags |= SSF_TRIGGERED;
	}

	return (physio(ssstrategy, NULL, dev, B_READ, ssminphys, uio));
}

/*
 * Actually translate the requested transfer into one the physical
 * driver can understand The transfer is described by a buf and will
 * include only one physical transfer.
 */
void
ssstrategy(bp)
	struct buf *bp;
{
	struct ss_softc *ss = ss_cd.cd_devs[SSUNIT(bp->b_dev)];
	struct scsipi_periph *periph = ss->sc_periph;
	int s;

	SC_DEBUG(ss->sc_periph, SCSIPI_DB1,
	    ("ssstrategy %ld bytes @ blk %d\n", bp->b_bcount, bp->b_blkno));

	/*
	 * If the device has been made invalid, error out
	 */
	if ((ss->sc_dev.dv_flags & DVF_ACTIVE) == 0) {
		bp->b_flags |= B_ERROR;
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto done;
	}

	/* If negative offset, error */
	if (bp->b_blkno < 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EINVAL;
		goto done;
	}

	if (bp->b_bcount > ss->sio.scan_window_size)
		bp->b_bcount = ss->sio.scan_window_size;

	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	s = splbio();

	/*
	 * Place it in the queue of activities for this scanner
	 * at the end (a bit silly because we only have on user..
	 * (but it could fork()))
	 */
	BUFQ_PUT(&ss->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 * (All a bit silly if we're only allowing 1 open but..)
	 */
	ssstart(ss->sc_periph);

	splx(s);
	return;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * ssstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer required. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (ssstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 * ssstart() is called at splbio
 */
void
ssstart(periph)
	struct scsipi_periph *periph;
{
	struct ss_softc *ss = (void *)periph->periph_dev;
	struct buf *bp;

	SC_DEBUG(periph, SCSIPI_DB2, ("ssstart "));
	/*
	 * See if there is a buf to do and we are not already
	 * doing one
	 */
	while (periph->periph_active < periph->periph_openings) {
		/* if a special awaits, let it proceed first */
		if (periph->periph_flags & PERIPH_WAITING) {
			periph->periph_flags &= ~PERIPH_WAITING;
			wakeup((caddr_t)periph);
			return;
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		if ((bp = BUFQ_GET(&ss->buf_queue)) == NULL)
			return;

		if (ss->special && ss->special->read) {
			(ss->special->read)(ss, bp);
		} else {
			/* generic scsi2 scanner read */
			/* XXX add code for SCSI2 scanner read */
		}
	}
}

/*
 * Perform special action on behalf of the user;
 * knows about the internals of this device
 */
int
ssioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct ss_softc *ss = ss_cd.cd_devs[SSUNIT(dev)];
	int error = 0;
	struct scan_io *sio;

	if ((ss->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	switch (cmd) {
	case SCIOCGET:
		if (ss->special && ss->special->get_params) {
			/* call special handler */
			error = (ss->special->get_params)(ss);
			if (error)
				return (error);
		} else {
			/* XXX add code for SCSI2 scanner, if any */
			return (EOPNOTSUPP);
		}
		memcpy(addr, &ss->sio, sizeof(struct scan_io));
		break;
	case SCIOCSET:
		sio = (struct scan_io *)addr;

		if (ss->special && ss->special->set_params) {
			/* call special handler */
			error = (ss->special->set_params)(ss, sio);
			if (error)
				return (error);
		} else {
			/* XXX add code for SCSI2 scanner, if any */
			return (EOPNOTSUPP);
		}
		break;
	case SCIOCRESTART:
		if (ss->special && ss->special->rewind_scanner ) {
			/* call special handler */
			error = (ss->special->rewind_scanner)(ss);
			if (error)
				return (error);
		} else
			/* XXX add code for SCSI2 scanner, if any */
			return (EOPNOTSUPP);
		ss->flags &= ~SSF_TRIGGERED;
		break;
#ifdef NOTYET
	case SCAN_USE_ADF:
		break;
#endif
	default:
		return (scsipi_do_ioctl(ss->sc_periph, dev, cmd, addr,
		    flag, p));
	}
	return (error);
}
