/*	$NetBSD: ch.c,v 1.40 1999/09/30 22:57:53 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/chio.h> 
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/poll.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_changer.h>
#include <dev/scsipi/scsiconf.h>

#define CHRETRIES	2
#define CHUNIT(x)	(minor((x)))

struct ch_softc {
	struct device	sc_dev;		/* generic device info */
	struct scsipi_link *sc_link;	/* link in the SCSI bus */

	u_int		sc_events;	/* event bitmask */
	struct selinfo	sc_selq;	/* select/poll queue for events */

	int		sc_flags;	/* misc. info */

	int		sc_picker;	/* current picker */

	/*
	 * The following information is obtained from the
	 * element address assignment page.
	 */
	int		sc_firsts[4];	/* firsts, indexed by CHET_* */
	int		sc_counts[4];	/* counts, indexed by CHET_* */

	/*
	 * The following mask defines the legal combinations
	 * of elements for the MOVE MEDIUM command.
	 */
	u_int8_t	sc_movemask[4];

	/*
	 * As above, but for EXCHANGE MEDIUM.
	 */
	u_int8_t	sc_exchangemask[4];

	/*
	 * Quirks; see below.
	 */
	int		sc_settledelay;	/* delay for settle */

};

/* sc_flags */
#define CHF_ROTATE		0x01	/* picker can rotate */

/* Autoconfiguration glue */
int	chmatch __P((struct device *, struct cfdata *, void *));
void	chattach __P((struct device *, struct device *, void *));

struct cfattach ch_ca = {
	sizeof(struct ch_softc), chmatch, chattach
};

extern struct cfdriver ch_cd;

struct scsipi_inquiry_pattern ch_patterns[] = {
	{T_CHANGER, T_REMOV,
	 "",		"",		""},
};

/* SCSI glue */
int	ch_interpret_sense __P((struct scsipi_xfer *));

struct scsipi_device ch_switch = {
	ch_interpret_sense,	/* check our error handler first */
	NULL,			/* no queue; our commands are synchronous */
	NULL,			/* have no async handler */
	NULL,			/* nothing to be done when xfer is done */
};

int	ch_move __P((struct ch_softc *, struct changer_move_request *));
int	ch_exchange __P((struct ch_softc *, struct changer_exchange_request *));
int	ch_position __P((struct ch_softc *, struct changer_position_request *));
int	ch_ielem __P((struct ch_softc *));
int	ch_ousergetelemstatus __P((struct ch_softc *, int, u_int8_t *));
int	ch_usergetelemstatus __P((struct ch_softc *,
	    struct changer_element_status_request *));
int	ch_getelemstatus __P((struct ch_softc *, int, int, void *,
	    size_t, int));
int	ch_setvoltag __P((struct ch_softc *,
	    struct changer_set_voltag_request *));
int	ch_get_params __P((struct ch_softc *, int));
void	ch_get_quirks __P((struct ch_softc *,
	    struct scsipi_inquiry_pattern *));
void	ch_event __P((struct ch_softc *, u_int));
int	ch_map_element __P((struct ch_softc *, u_int16_t, int *, int *));

void	ch_voltag_convert_in __P((const struct changer_volume_tag *,
	    struct changer_voltag *));
int	ch_voltag_convert_out __P((const struct changer_voltag *,
	    struct changer_volume_tag *));

/*
 * SCSI changer quirks.
 */
struct chquirk {
	struct	scsipi_inquiry_pattern cq_match; /* device id pattern */
	int	cq_settledelay;	/* settle delay, in seconds */
};

struct chquirk chquirks[] = {
	{{T_CHANGER, T_REMOV,
	  "SPECTRA",	"9000",		"0200"},
	 75},
};

int
chmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)ch_patterns, sizeof(ch_patterns) / sizeof(ch_patterns[0]),
	    sizeof(ch_patterns[0]), &priority);

	return (priority);
}

void
chattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ch_softc *sc = (struct ch_softc *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *link = sa->sa_sc_link;

	/* Glue into the SCSI bus */
	sc->sc_link = link;
	link->device = &ch_switch;
	link->device_softc = sc;
	link->openings = 1;

	printf("\n");

	/*
	 * Find out our device's quirks.
	 */
	ch_get_quirks(sc, &sa->sa_inqbuf);

	/*
	 * Some changers require a long time to settle out, to do
	 * tape inventory, for instance.
	 */
	if (sc->sc_settledelay) {
		printf("%s: waiting %d seconds for changer to settle...\n",
		    sc->sc_dev.dv_xname, sc->sc_settledelay);
		delay(1000000 * sc->sc_settledelay);
	}

	/*
	 * Get information about the device.  Note we can't use
	 * interrupts yet.
	 */
	if (ch_get_params(sc, XS_CTL_DISCOVERY|XS_CTL_IGNORE_MEDIA_CHANGE))
		printf("%s: offline\n", sc->sc_dev.dv_xname);
	else {
#define PLURAL(c)	(c) == 1 ? "" : "s"
		printf("%s: %d slot%s, %d drive%s, %d picker%s, %d portal%s\n",
		    sc->sc_dev.dv_xname,
		    sc->sc_counts[CHET_ST], PLURAL(sc->sc_counts[CHET_ST]),
		    sc->sc_counts[CHET_DT], PLURAL(sc->sc_counts[CHET_DT]),
		    sc->sc_counts[CHET_MT], PLURAL(sc->sc_counts[CHET_MT]),
		    sc->sc_counts[CHET_IE], PLURAL(sc->sc_counts[CHET_IE]));
#undef PLURAL
#ifdef CHANGER_DEBUG
		printf("%s: move mask: 0x%x 0x%x 0x%x 0x%x\n",
		    sc->sc_dev.dv_xname,
		    sc->sc_movemask[CHET_MT], sc->sc_movemask[CHET_ST],
		    sc->sc_movemask[CHET_IE], sc->sc_movemask[CHET_DT]);
		printf("%s: exchange mask: 0x%x 0x%x 0x%x 0x%x\n",
		    sc->sc_dev.dv_xname,
		    sc->sc_exchangemask[CHET_MT], sc->sc_exchangemask[CHET_ST],
		    sc->sc_exchangemask[CHET_IE], sc->sc_exchangemask[CHET_DT]);
#endif /* CHANGER_DEBUG */
	}

	/* Default the current picker. */
	sc->sc_picker = sc->sc_firsts[CHET_MT];
}

int
chopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct ch_softc *sc;
	int unit, error;

	unit = CHUNIT(dev);
	if ((unit >= ch_cd.cd_ndevs) ||
	    ((sc = ch_cd.cd_devs[unit]) == NULL))
		return (ENXIO);

	/*
	 * Only allow one open at a time.
	 */
	if (sc->sc_link->flags & SDEV_OPEN)
		return (EBUSY);

	if ((error = scsipi_adapter_addref(sc->sc_link)) != 0)
		return (error);

	sc->sc_link->flags |= SDEV_OPEN;

	/*
	 * Make sure the unit is on-line.  If a UNIT ATTENTION
	 * occurs, we will mark that an Init-Element-Status is
	 * needed in ch_get_params().
	 *
	 * We ignore NOT READY in case e.g a magazine isn't actually
	 * loaded into the changer or a tape isn't in the drive.
	 */
	error = scsipi_test_unit_ready(sc->sc_link, XS_CTL_IGNORE_NOT_READY);
	if (error)
		goto bad;

	/*
	 * Make sure our parameters are up to date.
	 */
	if ((error = ch_get_params(sc, 0)) != 0)
		goto bad;

	return (0);

 bad:
	scsipi_adapter_delref(sc->sc_link);
	sc->sc_link->flags &= ~SDEV_OPEN;
	return (error);
}

int
chclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];

	scsipi_wait_drain(sc->sc_link);

	scsipi_adapter_delref(sc->sc_link);

	sc->sc_events = 0;

	sc->sc_link->flags &= ~SDEV_OPEN;
	return (0);
}

int
chread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];
	int error;

	if (uio->uio_resid != CHANGER_EVENT_SIZE)
		return (EINVAL);

	/*
	 * Read never blocks; if there are no events pending, we just
	 * return an all-clear bitmask.
	 */
	error = uiomove(&sc->sc_events, CHANGER_EVENT_SIZE, uio);
	if (error == 0)
		sc->sc_events = 0;
	return (error);
}

int
chioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];
	int error = 0;

	/*
	 * If this command can change the device's state, we must
	 * have the device open for writing.
	 */
	switch (cmd) {
	case CHIOGPICKER:
	case CHIOGPARAMS:
	case OCHIOGSTATUS:
		break;

	default:
		if ((flags & FWRITE) == 0)
			return (EBADF);
	}

	switch (cmd) {
	case CHIOMOVE:
		error = ch_move(sc, (struct changer_move_request *)data);
		break;

	case CHIOEXCHANGE:
		error = ch_exchange(sc,
		    (struct changer_exchange_request *)data);
		break;

	case CHIOPOSITION:
		error = ch_position(sc,
		    (struct changer_position_request *)data);
		break;

	case CHIOGPICKER:
		*(int *)data = sc->sc_picker - sc->sc_firsts[CHET_MT];
		break;

	case CHIOSPICKER:
	    {
		int new_picker = *(int *)data;

		if (new_picker > (sc->sc_counts[CHET_MT] - 1))
			return (EINVAL);
		sc->sc_picker = sc->sc_firsts[CHET_MT] + new_picker;
		break;
	    }

	case CHIOGPARAMS:
	    {
		struct changer_params *cp = (struct changer_params *)data;

		cp->cp_curpicker = sc->sc_picker - sc->sc_firsts[CHET_MT];
		cp->cp_npickers = sc->sc_counts[CHET_MT];
		cp->cp_nslots = sc->sc_counts[CHET_ST];
		cp->cp_nportals = sc->sc_counts[CHET_IE];
		cp->cp_ndrives = sc->sc_counts[CHET_DT];
		break;
	    }

	case CHIOIELEM:
		error = ch_ielem(sc);
		break;

	case OCHIOGSTATUS:
	    {
		struct ochanger_element_status_request *cesr =
		    (struct ochanger_element_status_request *)data;

		error = ch_ousergetelemstatus(sc, cesr->cesr_type,
		    cesr->cesr_data);
		break;
	    }

	case CHIOGSTATUS:
		error = ch_usergetelemstatus(sc,
		    (struct changer_element_status_request *)data);
		break;

	case CHIOSVOLTAG:
		error = ch_setvoltag(sc,
		    (struct changer_set_voltag_request *)data);
		break;

	/* Implement prevent/allow? */

	default:
		error = scsipi_do_ioctl(sc->sc_link, dev, cmd, data, flags, p);
		break;
	}

	return (error);
}

int
chpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];
	int revents;

	revents = events & (POLLOUT | POLLWRNORM);

	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return (revents);

	if (sc->sc_events == 0)
		revents |= events & (POLLIN | POLLRDNORM);
	else
		selrecord(p, &sc->sc_selq);

	return (revents);
}

int
ch_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_link *sc_link = xs->sc_link;
	struct scsipi_sense_data *sense = &xs->sense.scsi_sense;
	struct ch_softc *sc = sc_link->device_softc;
	int error, retval = SCSIRET_CONTINUE;

	/*
	 * If it isn't an extended or extended/defered error, let
	 * the generic code handle it.
	 */
	if ((sense->error_code & SSD_ERRCODE) != 0x70 &&
	    (sense->error_code & SSD_ERRCODE) != 0x71)
		return (retval);

	if ((sense->flags & SSD_KEY) == SKEY_UNIT_ATTENTION) {
		/*
		 * The element status has possibly changed, usually because
		 * an operator has opened the door.  We need to initialize
		 * the element status.  If we haven't gotten our params yet,
		 * then we are about to (we are getting here via chopen()).
		 * Just notify ch_get_params() that we need to do an
		 * Init-Element-Status.  Otherwise, we need to call
		 * ch_get_params() ourselves.
		 */
		retval = SCSIRET_RETRY;
		if (sc->sc_link->flags & SDEV_MEDIA_LOADED) {
			sc->sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if ((xs->xs_control &
			     XS_CTL_IGNORE_MEDIA_CHANGE) == 0) {
				error = ch_get_params(sc, 0);
				if (error)
					retval = error;
			}
		}

		/*
		 * Enqueue an Element-Status-Changed event, and wake up
		 * any processes waiting for them.
		 */
		if ((xs->xs_control & XS_CTL_IGNORE_MEDIA_CHANGE) == 0)
			ch_event(sc, CHEV_ELEMENT_STATUS_CHANGED);
	}

	return (retval);
}

void
ch_event(sc, event)
	struct ch_softc *sc;
	u_int event;
{

	sc->sc_events |= event;
	selwakeup(&sc->sc_selq);
}

int
ch_move(sc, cm)
	struct ch_softc *sc;
	struct changer_move_request *cm;
{
	struct scsi_move_medium cmd;
	u_int16_t fromelem, toelem;

	/*
	 * Check arguments.
	 */
	if ((cm->cm_fromtype > CHET_DT) || (cm->cm_totype > CHET_DT))
		return (EINVAL);
	if ((cm->cm_fromunit > (sc->sc_counts[cm->cm_fromtype] - 1)) ||
	    (cm->cm_tounit > (sc->sc_counts[cm->cm_totype] - 1)))
		return (ENODEV);

	/*
	 * Check the request against the changer's capabilities.
	 */
	if ((sc->sc_movemask[cm->cm_fromtype] & (1 << cm->cm_totype)) == 0)
		return (ENODEV);

	/*
	 * Calculate the source and destination elements.
	 */
	fromelem = sc->sc_firsts[cm->cm_fromtype] + cm->cm_fromunit;
	toelem = sc->sc_firsts[cm->cm_totype] + cm->cm_tounit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = MOVE_MEDIUM;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(fromelem, cmd.src);
	_lto2b(toelem, cmd.dst);
	if (cm->cm_flags & CM_INVERT)
		cmd.flags |= MOVE_MEDIUM_INVERT;

	/*
	 * Send command to changer.
	 */
	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), NULL, 0, CHRETRIES,
	    100000, NULL, 0));
}

int
ch_exchange(sc, ce)
	struct ch_softc *sc;
	struct changer_exchange_request *ce;
{
	struct scsi_exchange_medium cmd;
	u_int16_t src, dst1, dst2;

	/*
	 * Check arguments.
	 */
	if ((ce->ce_srctype > CHET_DT) || (ce->ce_fdsttype > CHET_DT) ||
	    (ce->ce_sdsttype > CHET_DT))
		return (EINVAL);
	if ((ce->ce_srcunit > (sc->sc_counts[ce->ce_srctype] - 1)) ||
	    (ce->ce_fdstunit > (sc->sc_counts[ce->ce_fdsttype] - 1)) ||
	    (ce->ce_sdstunit > (sc->sc_counts[ce->ce_sdsttype] - 1)))
		return (ENODEV);

	/*
	 * Check the request against the changer's capabilities.
	 */
	if (((sc->sc_exchangemask[ce->ce_srctype] &
	     (1 << ce->ce_fdsttype)) == 0) ||
	    ((sc->sc_exchangemask[ce->ce_fdsttype] &
	     (1 << ce->ce_sdsttype)) == 0))
		return (ENODEV);

	/*
	 * Calculate the source and destination elements.
	 */
	src = sc->sc_firsts[ce->ce_srctype] + ce->ce_srcunit;
	dst1 = sc->sc_firsts[ce->ce_fdsttype] + ce->ce_fdstunit;
	dst2 = sc->sc_firsts[ce->ce_sdsttype] + ce->ce_sdstunit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = EXCHANGE_MEDIUM;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(src, cmd.src);
	_lto2b(dst1, cmd.fdst);
	_lto2b(dst2, cmd.sdst);
	if (ce->ce_flags & CE_INVERT1)
		cmd.flags |= EXCHANGE_MEDIUM_INV1;
	if (ce->ce_flags & CE_INVERT2)
		cmd.flags |= EXCHANGE_MEDIUM_INV2;

	/*
	 * Send command to changer.
	 */
	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), NULL, 0, CHRETRIES,
	    100000, NULL, 0));
}

int
ch_position(sc, cp)
	struct ch_softc *sc;
	struct changer_position_request *cp;
{
	struct scsi_position_to_element cmd;
	u_int16_t dst;

	/*
	 * Check arguments.
	 */
	if (cp->cp_type > CHET_DT)
		return (EINVAL);
	if (cp->cp_unit > (sc->sc_counts[cp->cp_type] - 1))
		return (ENODEV);

	/*
	 * Calculate the destination element.
	 */
	dst = sc->sc_firsts[cp->cp_type] + cp->cp_unit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = POSITION_TO_ELEMENT;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(dst, cmd.dst);
	if (cp->cp_flags & CP_INVERT)
		cmd.flags |= POSITION_TO_ELEMENT_INVERT;

	/*
	 * Send command to changer.
	 */
	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), NULL, 0, CHRETRIES,
	    100000, NULL, 0));
}

/*
 * Perform a READ ELEMENT STATUS on behalf of the user, and return to
 * the user only the data the user is interested in.  This returns the
 * old data format.
 */
int
ch_ousergetelemstatus(sc, chet, uptr)
	struct ch_softc *sc;
	int chet;
	u_int8_t *uptr;
{
	struct read_element_status_header *st_hdrp, st_hdr;
	struct read_element_status_page_header *pg_hdrp;
	struct read_element_status_descriptor *desc;
	size_t size, desclen;
	caddr_t data;
	int avail, i, error = 0;
	u_int8_t user_data;

	/*
	 * If there are no elements of the requested type in the changer,
	 * the request is invalid.
	 */
	if (sc->sc_counts[chet] == 0)
		return (EINVAL);

	/*
	 * Do the request the user wants, but only read the status header.
	 * This will tell us the amount of storage we must allocate in
	 * order to read all data.
	 */
	error = ch_getelemstatus(sc, sc->sc_firsts[chet],
	    sc->sc_counts[chet], &st_hdr, sizeof(st_hdr), 0);
	if (error)
		return (error);

	size = sizeof(struct read_element_status_header) +
	    _3btol(st_hdr.nbytes);

	/*
	 * We must have at least room for the status header and
	 * one page header (since we only ask for one element type
	 * at a time).
	 */
	if (size < (sizeof(struct read_element_status_header) +
	    sizeof(struct read_element_status_page_header)))
		return (EIO);

	/*
	 * Allocate the storage and do the request again.
	 */
	data = malloc(size, M_DEVBUF, M_WAITOK);
	error = ch_getelemstatus(sc, sc->sc_firsts[chet],
	    sc->sc_counts[chet], data, size, 0);
	if (error)
		goto done;

	st_hdrp = (struct read_element_status_header *)data;
	pg_hdrp = (struct read_element_status_page_header *)((u_long)st_hdrp +
	    sizeof(struct read_element_status_header));
	desclen = _2btol(pg_hdrp->edl);

	/*
	 * Fill in the user status array.
	 */
	avail = _2btol(st_hdrp->count);

	if (avail != sc->sc_counts[chet])
		printf("%s: warning, READ ELEMENT STATUS avail != count\n",
		    sc->sc_dev.dv_xname);

	desc = (struct read_element_status_descriptor *)((u_long)data +
	    sizeof(struct read_element_status_header) +
	    sizeof(struct read_element_status_page_header));
	for (i = 0; i < avail; ++i) {
		user_data = desc->flags1;
		error = copyout(&user_data, &uptr[i], avail);
		if (error)
			break;
		(u_long)desc += desclen;
	}

 done:
	if (data != NULL)
		free(data, M_DEVBUF);
	return (error);
}

/*
 * Perform a READ ELEMENT STATUS on behalf of the user.  This returns
 * the new (more complete) data format.
 */
int
ch_usergetelemstatus(sc, cesr)
	struct ch_softc *sc;
	struct changer_element_status_request *cesr;
{
	struct scsibus_softc *parent;
	struct scsipi_link *dtlink;
	struct device *dtdev;
	struct read_element_status_header *st_hdrp, st_hdr;
	struct read_element_status_page_header *pg_hdrp;
	struct read_element_status_descriptor *desc;
	struct changer_volume_tag *avol, *pvol;
	size_t size, desclen, stddesclen, offset;
	int first, avail, i, error = 0;
	caddr_t data;
	void *uvendptr;
	struct changer_element_status ces;

	/*
	 * Check arguments.
	 */
	if (cesr->cesr_type > CHET_DT)
		return (EINVAL);
	if (sc->sc_counts[cesr->cesr_type] == 0)
		return (ENODEV);
	if (cesr->cesr_unit > (sc->sc_counts[cesr->cesr_type] - 1))
		return (ENODEV);
	if (cesr->cesr_count >
	    (sc->sc_counts[cesr->cesr_type] + cesr->cesr_unit))
		return (EINVAL);

	/*
	 * Do the request the user wants, but only read the status header.
	 * This will tell us the amount of storage we must allocate
	 * in order to read all the data.
	 */
	error = ch_getelemstatus(sc, sc->sc_firsts[cesr->cesr_type] +
	    cesr->cesr_unit, cesr->cesr_count, &st_hdr, sizeof(st_hdr),
	    cesr->cesr_flags);
	if (error)
		return (error);

	size = sizeof(struct read_element_status_header) +
	    _3btol(st_hdr.nbytes);

	/*
	 * We must have at least room for the status header and
	 * one page header (since we only ask for oen element type
	 * at a time).
	 */
	if (size < (sizeof(struct read_element_status_header) +
	    sizeof(struct read_element_status_page_header)))
		return (EIO);

	/*
	 * Allocate the storage and do the request again.
	 */
	data = malloc(size, M_DEVBUF, M_WAITOK);
	error = ch_getelemstatus(sc, sc->sc_firsts[cesr->cesr_type] +
	    cesr->cesr_unit, cesr->cesr_count, data, size, cesr->cesr_flags);
	if (error)
		goto done;

	st_hdrp = (struct read_element_status_header *)data;
	pg_hdrp = (struct read_element_status_page_header *)((u_long)st_hdrp +
	    sizeof(struct read_element_status_header));
	desclen = _2btol(pg_hdrp->edl);

	/*
	 * Fill in the user status array.
	 */
	first = _2btol(st_hdrp->fear);
	if (first <  (sc->sc_firsts[cesr->cesr_type] + cesr->cesr_unit) ||
	    first >= (sc->sc_firsts[cesr->cesr_type] + cesr->cesr_unit +
		      cesr->cesr_count)) {
		error = EIO;
		goto done;
	}
	first -= sc->sc_firsts[cesr->cesr_type] + cesr->cesr_unit;

	avail = _2btol(st_hdrp->count);
	if (avail <= 0 || avail > cesr->cesr_count) {
		error = EIO;
		goto done;
	}

	offset = sizeof(struct read_element_status_header) +
		 sizeof(struct read_element_status_page_header);

	for (i = 0; i < cesr->cesr_count; i++) {
		memset(&ces, 0, sizeof(ces));
		if (i < first || i >= (first + avail)) {
			error = copyout(&ces, &cesr->cesr_data[i],
			    sizeof(ces));
			if (error)
				goto done;
		}

		desc = (struct read_element_status_descriptor *)
		    (data + offset);
		stddesclen = sizeof(struct read_element_status_descriptor);
		offset += desclen;

		ces.ces_flags = CESTATUS_STATUS_VALID;

		/*
		 * The SCSI flags conveniently map directly to the
		 * chio API flags.
		 */
		ces.ces_flags |= (desc->flags1 & 0x3f);

		ces.ces_asc = desc->sense_code;
		ces.ces_ascq = desc->sense_qual;

		/*
		 * For Data Transport elemenets, get the SCSI ID and LUN,
		 * and attempt to map them to a device name if they're
		 * on the same SCSI bus.
		 */
		if (desc->dt_scsi_flags & READ_ELEMENT_STATUS_DT_IDVALID) {
			ces.ces_target = desc->dt_scsi_addr;
			ces.ces_flags |= CESTATUS_TARGET_VALID;
		}
		if (desc->dt_scsi_flags & READ_ELEMENT_STATUS_DT_LUVALID) {
			ces.ces_lun = desc->dt_scsi_flags &
			    READ_ELEMENT_STATUS_DT_LUNMASK;
			ces.ces_flags |= CESTATUS_LUN_VALID;
		}
		if (desc->dt_scsi_flags & READ_ELEMENT_STATUS_DT_NOTBUS)
			ces.ces_flags |= CESTATUS_NOTBUS;
		else if ((ces.ces_flags &
			  (CESTATUS_TARGET_VALID|CESTATUS_LUN_VALID)) ==
			 (CESTATUS_TARGET_VALID|CESTATUS_LUN_VALID)) {
			parent = (struct scsibus_softc *)sc->sc_dev.dv_parent;
			if (ces.ces_target <= parent->sc_maxtarget &&
			    ces.ces_lun <= parent->sc_maxlun &&
			    (dtlink =
			     parent->sc_link[ces.ces_target][ces.ces_lun])
			     != NULL &&
			    (dtdev = dtlink->device_softc) != NULL) {
				strcpy(ces.ces_xname, dtdev->dv_xname);
				ces.ces_flags |= CESTATUS_XNAME_VALID;
			}
		}

		if (desc->flags2 & READ_ELEMENT_STATUS_INVERT)
			ces.ces_flags |= CESTATUS_INVERTED;

		if (desc->flags2 & READ_ELEMENT_STATUS_SVALID) {
			if (ch_map_element(sc, _2btol(desc->ssea),
			    &ces.ces_from_type, &ces.ces_from_unit))
				ces.ces_flags |= CESTATUS_FROM_VALID;
		}

		/*
		 * Extract volume tag information.
		 */
		switch (pg_hdrp->flags &
		    (READ_ELEMENT_STATUS_PVOLTAG|READ_ELEMENT_STATUS_AVOLTAG)) {
		case (READ_ELEMENT_STATUS_PVOLTAG|READ_ELEMENT_STATUS_AVOLTAG):
			pvol = (struct changer_volume_tag *)(desc + 1);
			avol = pvol + 1;
			break;

		case READ_ELEMENT_STATUS_PVOLTAG:
			pvol = (struct changer_volume_tag *)(desc + 1);
			avol = NULL;
			break;

		case READ_ELEMENT_STATUS_AVOLTAG:
			pvol = NULL;
			avol = (struct changer_volume_tag *)(desc + 1);
			break;

		default:
			avol = pvol = NULL;
			break;
		}

		if (pvol != NULL) {
			ch_voltag_convert_in(pvol, &ces.ces_pvoltag);
			ces.ces_flags |= CESTATUS_PVOL_VALID;
			stddesclen += sizeof(struct changer_volume_tag);
		}
		if (avol != NULL) {
			ch_voltag_convert_in(avol, &ces.ces_avoltag);
			ces.ces_flags |= CESTATUS_AVOL_VALID;
			stddesclen += sizeof(struct changer_volume_tag);
		}

		/*
		 * Compute vendor-specific length.  Note the 4 reserved
		 * bytes between the volume tags and the vendor-specific
		 * data.  Copy it out of the user wants it.
		 */
		stddesclen += 4;
		if (desclen > stddesclen)
			ces.ces_vendor_len = desclen - stddesclen;

		if (ces.ces_vendor_len != 0 && cesr->cesr_vendor_data != NULL) {
			error = copyin(&cesr->cesr_vendor_data[i], &uvendptr,
			    sizeof(uvendptr));
			if (error)
				goto done;
			error = copyout((void *)((u_long)desc + stddesclen),
			    uvendptr, ces.ces_vendor_len);
			if (error)
				goto done;
		}

		/*
		 * Now copy out the status descriptor we've constructed.
		 */
		error = copyout(&ces, &cesr->cesr_data[i], sizeof(ces));
		if (error)
			goto done;
	}

 done:
	if (data != NULL)
		free(data, M_DEVBUF);
	return (error);
}

int
ch_getelemstatus(sc, first, count, data, datalen, flags)
	struct ch_softc *sc;
	int first, count;
	void *data;
	size_t datalen;
	int flags;
{
	struct scsi_read_element_status cmd;

	/*
	 * Build SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = READ_ELEMENT_STATUS;
	cmd.byte2 = ELEMENT_TYPE_ALL;
	if (flags & CESR_VOLTAGS)
		cmd.byte2 |= READ_ELEMENT_STATUS_VOLTAG;
	_lto2b(first, cmd.sea);
	_lto2b(count, cmd.count);
	_lto3b(datalen, cmd.len);

	/*
	 * Send command to changer.
	 */
	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    (u_char *)data, datalen, CHRETRIES, 100000, NULL, XS_CTL_DATA_IN));
}

int
ch_setvoltag(sc, csvr)
	struct ch_softc *sc;
	struct changer_set_voltag_request *csvr;
{
	struct scsi_send_volume_tag cmd;
	struct changer_volume_tag voltag;
	void *data = NULL;
	size_t datalen = 0;
	int error;
	u_int16_t dst;

	/*
	 * Check arguments.
	 */
	if (csvr->csvr_type > CHET_DT)
		return (EINVAL);
	if (csvr->csvr_unit > (sc->sc_counts[csvr->csvr_type] - 1))
		return (ENODEV);

	dst = sc->sc_firsts[csvr->csvr_type] + csvr->csvr_unit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = SEND_VOLUME_TAG;
	_lto2b(dst, cmd.eaddr);

#define	ALTERNATE	(csvr->csvr_flags & CSVR_ALTERNATE)

	switch (csvr->csvr_flags & CSVR_MODE_MASK) {
	case CSVR_MODE_SET:
		cmd.sac = ALTERNATE ? SAC_ASSERT_ALT : SAC_ASSERT_PRIMARY;
		break;

	case CSVR_MODE_REPLACE:
		cmd.sac = ALTERNATE ? SAC_REPLACE_ALT : SAC_REPLACE_PRIMARY;
		break;

	case CSVR_MODE_CLEAR:
		cmd.sac = ALTERNATE ? SAC_UNDEFINED_ALT : SAC_UNDEFINED_PRIMARY;
		break;

	default:
		return (EINVAL);
	}

#undef ALTERNATE

	if (cmd.sac < SAC_UNDEFINED_PRIMARY) {
		error = ch_voltag_convert_out(&csvr->csvr_voltag, &voltag);
		if (error)
			return (error);
		data = &voltag;
		datalen = sizeof(voltag);
		_lto2b(datalen, cmd.length);
	}

	/*
	 * Send command to changer.
	 */
	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    (u_char *)data, datalen, CHRETRIES, 100000, NULL,
	    datalen ? XS_CTL_DATA_OUT : 0));
}

int
ch_ielem(sc)
	struct ch_softc *sc;
{
	int tmo;
	struct scsi_initialize_element_status cmd;

	/*
	 * Build SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = INITIALIZE_ELEMENT_STATUS;

	/*
	 * Send command to changer.
	 *
	 * The problem is, how long to allow for the command?
	 * It can take a *really* long time, and also depends
	 * on unknowable factors such as whether there are
	 * *almost* readable labels on tapes that a barcode
	 * reader is trying to decipher.
	 *
	 * I'm going to make this long enough to allow 5 minutes
	 * per element plus an initial 10 minute wait.
	 */
	tmo =	sc->sc_counts[CHET_MT] +
		sc->sc_counts[CHET_ST] +
		sc->sc_counts[CHET_IE] +
		sc->sc_counts[CHET_DT];
	tmo *= 5 * 60 * 1000;
	tmo += (10 * 60 * 1000);

	return (scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd),
	    NULL, 0, CHRETRIES, tmo, NULL, XS_CTL_IGNORE_ILLEGAL_REQUEST));
}

/*
 * Ask the device about itself and fill in the parameters in our
 * softc.
 */
int
ch_get_params(sc, scsiflags)
	struct ch_softc *sc;
	int scsiflags;
{
	struct scsi_mode_sense cmd;
	struct scsi_mode_sense_data {
		struct scsi_mode_header header;
		union {
			struct page_element_address_assignment ea;
			struct page_transport_geometry_parameters tg;
			struct page_device_capabilities cap;
		} pages;
	} sense_data;
	int error, from;
	u_int8_t *moves, *exchanges;

	/*
	 * Grab info from the element address assignment page.
	 */
	bzero(&cmd, sizeof(cmd));
	bzero(&sense_data, sizeof(sense_data));
	cmd.opcode = SCSI_MODE_SENSE;
	cmd.byte2 |= 0x08;	/* disable block descriptors */
	cmd.page = 0x1d;
	cmd.length = (sizeof(sense_data) & 0xff);
	error = scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), (u_char *)&sense_data,
	    sizeof(sense_data), CHRETRIES, 6000, NULL,
	    scsiflags | XS_CTL_DATA_IN);
	if (error) {
		printf("%s: could not sense element address page\n",
		    sc->sc_dev.dv_xname);
		return (error);
	}

	sc->sc_firsts[CHET_MT] = _2btol(sense_data.pages.ea.mtea);
	sc->sc_counts[CHET_MT] = _2btol(sense_data.pages.ea.nmte);
	sc->sc_firsts[CHET_ST] = _2btol(sense_data.pages.ea.fsea);
	sc->sc_counts[CHET_ST] = _2btol(sense_data.pages.ea.nse);
	sc->sc_firsts[CHET_IE] = _2btol(sense_data.pages.ea.fieea);
	sc->sc_counts[CHET_IE] = _2btol(sense_data.pages.ea.niee);
	sc->sc_firsts[CHET_DT] = _2btol(sense_data.pages.ea.fdtea);
	sc->sc_counts[CHET_DT] = _2btol(sense_data.pages.ea.ndte);

	/* XXX ask for page trasport geom */

	/*
	 * Grab info from the capabilities page.
	 */
	bzero(&cmd, sizeof(cmd));
	bzero(&sense_data, sizeof(sense_data));
	cmd.opcode = SCSI_MODE_SENSE;
	/*
	 * XXX: Note: not all changers can deal with disabled block descriptors
	 */
	cmd.byte2 = 0x08;	/* disable block descriptors */
	cmd.page = 0x1f;
	cmd.length = (sizeof(sense_data) & 0xff);
	error = scsipi_command(sc->sc_link,
	    (struct scsipi_generic *)&cmd, sizeof(cmd), (u_char *)&sense_data,
	    sizeof(sense_data), CHRETRIES, 6000, NULL,
	    scsiflags | XS_CTL_DATA_IN);
	if (error) {
		printf("%s: could not sense capabilities page\n",
		    sc->sc_dev.dv_xname);
		return (error);
	}

	bzero(sc->sc_movemask, sizeof(sc->sc_movemask));
	bzero(sc->sc_exchangemask, sizeof(sc->sc_exchangemask));
	moves = &sense_data.pages.cap.move_from_mt;
	exchanges = &sense_data.pages.cap.exchange_with_mt;
	for (from = CHET_MT; from <= CHET_DT; ++from) {
		sc->sc_movemask[from] = moves[from];
		sc->sc_exchangemask[from] = exchanges[from];
	}

	/*
	 * If we need to do an Init-Element-Status, do that now that
	 * we know what's in the changer.
	 */
	if ((scsiflags & XS_CTL_IGNORE_MEDIA_CHANGE) == 0) {
		if ((sc->sc_link->flags & SDEV_MEDIA_LOADED) == 0)
			error = ch_ielem(sc);
		if (error == 0)
			sc->sc_link->flags |= SDEV_MEDIA_LOADED;
		else
			sc->sc_link->flags &= ~SDEV_MEDIA_LOADED;
	}
	return (error);
}

void
ch_get_quirks(sc, inqbuf)
	struct ch_softc *sc;
	struct scsipi_inquiry_pattern *inqbuf;
{
	struct chquirk *match;
	int priority;

	sc->sc_settledelay = 0;

	match = (struct chquirk *)scsipi_inqmatch(inqbuf,
	    (caddr_t)chquirks,
	    sizeof(chquirks) / sizeof(chquirks[0]),
	    sizeof(chquirks[0]), &priority);
	if (priority != 0)
		sc->sc_settledelay = match->cq_settledelay;
}

int
ch_map_element(sc, elem, typep, unitp)
	struct ch_softc *sc;
	u_int16_t elem;
	int *typep, *unitp;
{
	int chet;

	for (chet = CHET_MT; chet <= CHET_DT; chet++) {
		if (elem >= sc->sc_firsts[chet] &&
		    elem < (sc->sc_firsts[chet] + sc->sc_counts[chet])) {
			*typep = chet;
			*unitp = elem - sc->sc_firsts[chet];
			return (1);
		}
	}
	return (0);
}

void
ch_voltag_convert_in(sv, cv)
	const struct changer_volume_tag *sv;
	struct changer_voltag *cv;
{
	int i;

	memset(cv, 0, sizeof(struct changer_voltag));

	/*
	 * Copy the volume tag string from the SCSI representation.
	 * Per the SCSI-2 spec, we stop at the first blank character.
	 */
	for (i = 0; i < sizeof(sv->volid); i++) {
		if (sv->volid[i] == ' ')
			break;
		cv->cv_tag[i] = sv->volid[i];
	}
	cv->cv_tag[i] = '\0';

	cv->cv_serial = _2btol(sv->volseq);
}

int
ch_voltag_convert_out(cv, sv)
	const struct changer_voltag *cv;
	struct changer_volume_tag *sv;
{
	int i;

	memset(sv, ' ', sizeof(struct changer_volume_tag));

	for (i = 0; i < sizeof(sv->volid); i++) {
		if (cv->cv_tag[i] == '\0')
			break;
		/*
		 * Limit the character set to what is suggested in
		 * the SCSI-2 spec.
		 */
		if ((cv->cv_tag[i] < '0' || cv->cv_tag[i] > '9') &&
		    (cv->cv_tag[i] < 'A' || cv->cv_tag[i] > 'Z') &&
		    (cv->cv_tag[i] != '_'))
			return (EINVAL);
		sv->volid[i] = cv->cv_tag[i];
	}

	_lto2b(cv->cv_serial, sv->volseq);

	return (0);
}
