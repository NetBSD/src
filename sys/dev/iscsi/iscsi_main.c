/*	$netBSD: iscsi_main.c,v 1.1.1.1 2011/05/02 07:01:11 agc Exp $	*/

/*-
 * Copyright (c) 2004,2005,2006,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
#include "iscsi_globals.h"

#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include "ioconf.h"

/*------------------------- Global Variables ------------------------*/

extern struct cfdriver iscsi_cd;

#if defined(ISCSI_DEBUG)
int iscsi_debug_level = ISCSI_DEBUG;
#endif

bool iscsi_detaching;

/* the list of sessions */
session_list_t iscsi_sessions = TAILQ_HEAD_INITIALIZER(iscsi_sessions);

/* the number of active send threads (for cleanup thread) */
uint32_t iscsi_num_send_threads = 0;

/* Our node name, alias, and ISID */
uint8_t iscsi_InitiatorName[ISCSI_STRING_LENGTH] = "";
uint8_t iscsi_InitiatorAlias[ISCSI_STRING_LENGTH] = "";
login_isid_t iscsi_InitiatorISID;

/******************************************************************************/

/*
   System interface: autoconf and device structures
*/

static void iscsi_attach(device_t parent, device_t self, void *aux);
static int iscsi_match(device_t, cfdata_t, void *);
static int iscsi_detach(device_t, int);

struct iscsi_softc {
	device_t		dev;
	kmutex_t		lock;
	TAILQ_HEAD(, iscsifd)	fds;
};

CFATTACH_DECL_NEW(iscsi, sizeof(struct iscsi_softc), iscsi_match, iscsi_attach,
			  iscsi_detach, NULL);


static dev_type_open(iscsiopen);
static int iscsiclose(struct file *);

static const struct fileops iscsi_fileops = {
	.fo_ioctl = iscsiioctl,
	.fo_close = iscsiclose,
};

struct cdevsw iscsi_cdevsw = {
	.d_open = iscsiopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

/******************************************************************************/

STATIC void iscsi_scsipi_request(struct scsipi_channel *,
                                 scsipi_adapter_req_t, void *);
STATIC void iscsi_minphys(struct buf *);

/******************************************************************************/

/*******************************************************************************
* Open and Close device interfaces. We don't really need them, because we don't
* have to keep track of device opens and closes from userland. But apps can't
* call ioctl without a handle to the device, and the kernel doesn't hand out
* handles without an open routine in the driver. So here they are in all their
* glory...
*******************************************************************************/

int
iscsiopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct iscsifd *d;
	struct iscsi_softc *sc;
	struct file *fp;
	int error, fd, unit;

	unit = minor(dev);

	DEB(99, ("ISCSI Open unit=%d\n",unit));

	sc = device_lookup_private(&iscsi_cd, unit);
	if (sc == NULL)
		return ENXIO;

	if ((error = fd_allocfile(&fp, &fd)) != 0)
		return error;

	d = kmem_alloc(sizeof(*d), KM_SLEEP);
	d->dev = sc->dev;
	d->unit = unit;

	mutex_enter(&sc->lock);
	if (iscsi_detaching) {
		mutex_exit(&sc->lock);
		kmem_free(d, sizeof(*d));
		DEB(99, ("ISCSI Open aborting\n"));
		fd_abort(curproc, fp, fd);
		return ENXIO;
	}
	TAILQ_INSERT_TAIL(&sc->fds, d, link);
	mutex_exit(&sc->lock);

	return fd_clone(fp, fd, flag, &iscsi_fileops, d);
}

static int
iscsiclose(struct file *fp)
{
	struct iscsifd *d = fp->f_iscsi;
	struct iscsi_softc *sc;

	sc = device_lookup_private(&iscsi_cd, d->unit);
	if (sc == NULL) {
		DEBOUT(("%s: Cannot find private data\n",__func__));
		return ENXIO;
	}

	mutex_enter(&sc->lock);
	TAILQ_REMOVE(&sc->fds, d, link);
	mutex_exit(&sc->lock);

	kmem_free(d, sizeof(*d));
	fp->f_iscsi = NULL;

	DEB(99, ("ISCSI Close\n"));
	return 0;
}

/******************************************************************************/

/*
 * The config Match routine.
 *    Not much to do here, either - this is a pseudo-device.
 */

static int
iscsi_match(device_t self, cfdata_t cfdata, void *arg)
{
	return 1;
}

/*
 * iscsiattach:
 *    Only called when statically configured into a kernel
 */
void
iscsiattach(int n)
{
	int err;
	cfdata_t cf;

	err = config_cfattach_attach(iscsi_cd.cd_name, &iscsi_ca);
	if (err) {
		aprint_error("%s: couldn't register cfattach: %d\n",
		    iscsi_cd.cd_name, err);
		config_cfdriver_detach(&iscsi_cd);
		return;
	}

	if (n > 1)
		aprint_error("%s: only one device supported\n",
		    iscsi_cd.cd_name);

	cf = kmem_alloc(sizeof(struct cfdata), KM_NOSLEEP);
	if (cf == NULL) {
		aprint_error("%s: couldn't allocate cfdata\n",
		    iscsi_cd.cd_name);
		return;
	}
	cf->cf_name = iscsi_cd.cd_name;
	cf->cf_atname = iscsi_cd.cd_name;
	cf->cf_unit = 0;
	cf->cf_fstate = FSTATE_NOTFOUND;

	(void)config_attach_pseudo(cf);
	return;
}

/*
 * iscsi_attach:
 *    One-time inits go here. Not much for now, probably even less later.
 */
static void
iscsi_attach(device_t parent, device_t self, void *aux)
{
	struct iscsi_softc *sc;

	DEB(1, ("ISCSI: iscsi_attach, parent=%p, self=%p, aux=%p\n", parent,
			self, aux));
	sc = (struct iscsi_softc *) device_private(self);
	sc->dev = self;

	TAILQ_INIT(&sc->fds);
	mutex_init(&sc->lock, MUTEX_DEFAULT, IPL_NONE);

	iscsi_detaching = false;
	iscsi_init_cleanup();

	aprint_normal("%s: attached.  major = %d\n", iscsi_cd.cd_name,
	    cdevsw_lookup_major(&iscsi_cdevsw));
}

/*
 * iscsi_detach:
 *    Cleanup.
 */
static int
iscsi_detach(device_t self, int flags)
{
	struct iscsi_softc *sc;
	int error;

	DEB(1, ("ISCSI: detach\n"));
	sc = (struct iscsi_softc *) device_private(self);

	mutex_enter(&sc->lock);
	if (!TAILQ_EMPTY(&sc->fds)) {
		mutex_exit(&sc->lock);
		return EBUSY;
	}
	iscsi_detaching = true;
	mutex_exit(&sc->lock);

	error = kill_all_sessions();
	if (error)
		return error;

	error = iscsi_destroy_cleanup();
	if (error)
		return error;

	mutex_destroy(&sc->lock);

	return 0;
}

/******************************************************************************/

typedef struct quirktab_t {
	const char	*tgt;
	const char	*iqn;
	uint32_t	 quirks;
} quirktab_t;

static const quirktab_t	quirktab[] = {
	{ "StarWind", "iqn.2008-08.com.starwindsoftware", PQUIRK_ONLYBIG },
	{ "UNH", "iqn.2002-10.edu.unh.",
	    PQUIRK_NOBIGMODESENSE |
	    PQUIRK_NOMODESENSE |
	    PQUIRK_NOSYNCCACHE },
	{ "NetBSD", "iqn.1994-04.org.netbsd.", 0 },
	{ "Unknown", "unknown", 0 },
	{ NULL, NULL, 0 }
};

/* loop through the quirktab looking for a match on target name */
static const quirktab_t *
getquirks(const char *iqn)
{
	const quirktab_t	*qp;
	size_t iqnlen, quirklen;

	if (iqn == NULL)
		iqn = "unknown";
	iqnlen = strlen(iqn);
	for (qp = quirktab ; qp->iqn ; qp++) {
		quirklen = strlen(qp->iqn);
		if (quirklen > iqnlen)
			continue;
		if (memcmp(qp->iqn, iqn, quirklen) == 0)
			break;
	}
	return qp;
}

/******************************************************************************/

/*
 * map_session
 *    This (indirectly) maps the existing LUNs for a target to SCSI devices
 *    by going through config_found to tell any child drivers that there's
 *    a new adapter.
 *    Note that each session is equivalent to a SCSI adapter.
 *
 *    Parameter:  the session pointer
 *
 *    Returns:    1 on success, 0 on failure
 *
 * ToDo: Figuring out how to handle more than one LUN. It appears that
 *    the NetBSD SCSI LUN discovery doesn't use "report LUNs", and instead
 *    goes through the LUNs sequentially, stopping somewhere on the way if it
 *    gets an error. We may have to do some LUN mapping in here if this is
 *    really how things work.
 */

int
map_session(session_t *session, device_t dev)
{
	struct scsipi_adapter *adapt = &session->sc_adapter;
	struct scsipi_channel *chan = &session->sc_channel;
	const quirktab_t	*tgt;

	mutex_enter(&session->lock);
	session->send_window = max(2, window_size(session, CCBS_FOR_SCSIPI));
	mutex_exit(&session->lock);

	/*
	 * Fill in the scsipi_adapter.
	 */
	adapt->adapt_dev = dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_request = iscsi_scsipi_request;
	adapt->adapt_minphys = iscsi_minphys;
	adapt->adapt_openings = session->send_window;
	adapt->adapt_max_periph = CCBS_FOR_SCSIPI;
	adapt->adapt_flags = SCSIPI_ADAPT_MPSAFE;

	/*
	 * Fill in the scsipi_channel.
	 */
	if ((tgt = getquirks(chan->chan_name)) == NULL) {
		tgt = getquirks("unknown");
	}
	chan->chan_name = tgt->tgt;
	chan->chan_defquirks = tgt->quirks;
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_flags = SCSIPI_CHAN_NOSETTLE | SCSIPI_CHAN_CANGROW;
	chan->chan_ntargets = 1;
	chan->chan_nluns = 16;		/* ToDo: ??? */
	chan->chan_id = session->id;

	session->child_dev = config_found(dev, chan, scsiprint);

	return session->child_dev != NULL;
}


/*
 * unmap_session
 *    This (indirectly) unmaps the existing all LUNs for a target by
 *    telling the config system that the adapter has detached.
 *
 *    Parameter:  the session pointer
 *
 *    Returns:    1 on success, 0 on failure
 */

int
unmap_session(session_t *session)
{
	device_t dev;
	int rv = 1;

	if ((dev = session->child_dev) != NULL) {
		session->child_dev = NULL;
		if (config_detach(dev, 0))
			rv = 0;
	}

	return rv;
}

/*
 * grow_resources
 *    Try to grow openings up to current window size
 */
static void
grow_resources(session_t *session)
{
	struct scsipi_adapter *adapt = &session->sc_adapter;
	int win;

	mutex_enter(&session->lock);
	if (session->refcount < CCBS_FOR_SCSIPI &&
	    session->send_window < CCBS_FOR_SCSIPI) {
		win = window_size(session, CCBS_FOR_SCSIPI - session->refcount);
		if (win > session->send_window) {
			session->send_window++;
			adapt->adapt_openings++;
			DEB(5, ("Grow send window to %d\n", session->send_window));
		}
	}
	mutex_exit(&session->lock);
}

/******************************************************************************/

/*****************************************************************************
 * SCSI interface routines
 *****************************************************************************/

/*
 * iscsi_scsipi_request:
 *    Perform a request for the SCSIPI layer.
 */

void
iscsi_scsipi_request(struct scsipi_channel *chan, scsipi_adapter_req_t req,
					 void *arg)
{
	struct scsipi_adapter *adapt = chan->chan_adapter;
	struct scsipi_xfer *xs;
	session_t *session;
	int flags;
	struct scsipi_xfer_mode *xm;
	int error;

	session = (session_t *) adapt;	/* adapter is first field in session */

	error = ref_session(session);

	switch (req) {
	case ADAPTER_REQ_RUN_XFER:
		DEB(9, ("ISCSI: scsipi_request RUN_XFER\n"));
		xs = arg;
		flags = xs->xs_control;

		if (error) {
			DEB(9, ("ISCSI: refcount too high: %d, winsize %d\n",
				session->refcount, session->send_window));
			xs->error = XS_BUSY;
			xs->status = XS_BUSY;
			scsipi_done(xs);
			return;
		}

		if ((flags & XS_CTL_POLL) != 0) {
			xs->error = XS_DRIVER_STUFFUP;
			DEBOUT(("Run Xfer request with polling\n"));
			scsipi_done(xs);
			break;
		}
		/*
		 * NOTE: It appears that XS_CTL_DATA_UIO is not actually used anywhere.
		 * Since it really would complicate matters to handle offsets
		 * into scatter-gather lists, and a number of other drivers don't
		 * handle uio-based data as well, XS_CTL_DATA_UIO isn't
		 * implemented in this driver (at least for now).
		 */
		if (flags & XS_CTL_DATA_UIO) {
			xs->error = XS_DRIVER_STUFFUP;
			DEBOUT(("Run Xfer with data in UIO\n"));
			scsipi_done(xs);
			break;
		}

		send_run_xfer(session, xs);
		DEB(15, ("scsipi_req returns, refcount = %d\n", session->refcount));
		return;

	case ADAPTER_REQ_GROW_RESOURCES:
		DEB(5, ("ISCSI: scsipi_request GROW_RESOURCES\n"));
		grow_resources(session);
		break;

	case ADAPTER_REQ_SET_XFER_MODE:
		DEB(5, ("ISCSI: scsipi_request SET_XFER_MODE\n"));
		xm = (struct scsipi_xfer_mode *)arg;
		xm->xm_mode = PERIPH_CAP_TQING;
		scsipi_async_event(chan, ASYNC_EVENT_XFER_MODE, xm);
		break;

	default:
		DEBOUT(("ISCSI: scsipi_request with invalid REQ code %d\n", req));
		break;
	}

	if (!error)
		unref_session(session);
}

/* cap the transfer at 64K */
#define ISCSI_MAX_XFER	65536

/*
 * iscsi_minphys:
 *    Limit a transfer to our maximum transfer size.
 */

void
iscsi_minphys(struct buf *bp)
{
	if (bp->b_bcount > ISCSI_MAX_XFER) {
		bp->b_bcount = ISCSI_MAX_XFER;
	}
}

/*****************************************************************************
 * SCSI job execution helper routines
 *****************************************************************************/

/*
 * iscsi_done:
 *
 * A CCB has completed execution.  Pass the status back to the
 * upper layer.
 */
void
iscsi_done(ccb_t *ccb)
{
	struct scsipi_xfer *xs = ccb->xs;
	DEB(9, ("iscsi_done\n"));

	if (xs != NULL) {
		ccb->xs = NULL;
		xs->resid = ccb->residual;

		switch (ccb->status) {
		case ISCSI_STATUS_SUCCESS:
			xs->error = XS_NOERROR;
			xs->status = SCSI_OK;
			break;

		case ISCSI_STATUS_CHECK_CONDITION:
			xs->error = XS_SENSE;
			xs->status = SCSI_CHECK;
			break;

		case ISCSI_STATUS_TARGET_BUSY:
		case ISCSI_STATUS_NO_RESOURCES:
			DEBC(ccb->connection, 5, ("target busy, ccb %p\n", ccb));
			xs->error = XS_BUSY;
			xs->status = SCSI_BUSY;
			break;

		case ISCSI_STATUS_SOCKET_ERROR:
		case ISCSI_STATUS_TIMEOUT:
			xs->error = XS_SELTIMEOUT;
			xs->status = SCSI_BUSY;
			break;

		case ISCSI_STATUS_QUEUE_FULL:
			DEBC(ccb->connection, 5, ("queue full, ccb %p\n", ccb));
			xs->error = XS_BUSY;
			xs->status = SCSI_QUEUE_FULL;
			break;

		default:
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}

		unref_session(ccb->session);

		DEB(99, ("Calling scsipi_done (%p), err = %d\n", xs, xs->error));
		scsipi_done(xs);
		DEB(99, ("scsipi_done returned\n"));
	} else {
		DEBOUT(("ISCSI: iscsi_done CCB %p without XS\n", ccb));
	}
}

SYSCTL_SETUP(sysctl_iscsi_setup, "ISCSI subtree setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, &node,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "iscsi",
		SYSCTL_DESCR("iscsi controls"),
		NULL, 0, NULL, 0,
		CTL_HW, CTL_CREATE, CTL_EOL);

#ifdef ISCSI_DEBUG
	sysctl_createv(clog, 0, &node, NULL,
		CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		CTLTYPE_INT, "debug",
		SYSCTL_DESCR("debug level"),
		NULL, 0,  &iscsi_debug_level, sizeof(iscsi_debug_level),
		CTL_CREATE, CTL_EOL);
#endif
}


/* Kernel Module support */

#include <sys/module.h>

MODULE(MODULE_CLASS_DRIVER, iscsi, NULL); /* Possibly a builtin module */

#ifdef _MODULE
static const struct cfiattrdata ibescsi_info = { "scsi", 1,
	{{"channel", "-1", -1},}
};

static const struct cfiattrdata *const iscsi_attrs[] = { &ibescsi_info, NULL };

CFDRIVER_DECL(iscsi, DV_DULL, iscsi_attrs);

static struct cfdata iscsi_cfdata[] = {
	{
		.cf_name = "iscsi",
		.cf_atname = "iscsi",
		.cf_unit = 0,		/* Only unit 0 is ever used  */
		.cf_fstate = FSTATE_NOTFOUND,
		.cf_loc = NULL,
		.cf_flags = 0,
		.cf_pspec = NULL,
	},
	{ NULL, NULL, 0, 0, NULL, 0, NULL }
};
#endif

static int
iscsi_modcmd(modcmd_t cmd, void *arg)
{
#ifdef _MODULE
	devmajor_t cmajor = NODEVMAJOR, bmajor = NODEVMAJOR;
	int error;
	static struct sysctllog *clog;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_cfdriver_attach(&iscsi_cd);
		if (error) {
			return error;
		}

		error = config_cfattach_attach(iscsi_cd.cd_name, &iscsi_ca);
		if (error) {
			config_cfdriver_detach(&iscsi_cd);
			aprint_error("%s: unable to register cfattach\n",
				iscsi_cd.cd_name);
			return error;
		}

		error = config_cfdata_attach(iscsi_cfdata, 1);
		if (error) {
			aprint_error("%s: unable to attach cfdata\n",
				iscsi_cd.cd_name);
			config_cfattach_detach(iscsi_cd.cd_name, &iscsi_ca);
			config_cfdriver_detach(&iscsi_cd);
			return error;
		}

		error = devsw_attach(iscsi_cd.cd_name, NULL, &bmajor,
			&iscsi_cdevsw, &cmajor);
		if (error) {
			aprint_error("%s: unable to register devsw\n",
				iscsi_cd.cd_name);
			config_cfdata_detach(iscsi_cfdata);
			config_cfattach_detach(iscsi_cd.cd_name, &iscsi_ca);
			config_cfdriver_detach(&iscsi_cd);
			return error;
		}

		if (config_attach_pseudo(iscsi_cfdata) == NULL) {
			aprint_error("%s: config_attach_pseudo failed\n",
				iscsi_cd.cd_name);
			config_cfattach_detach(iscsi_cd.cd_name, &iscsi_ca);
			config_cfdriver_detach(&iscsi_cd);
			return ENXIO;
		}

		sysctl_iscsi_setup(&clog);
#endif
		return 0;
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_cfdata_detach(iscsi_cfdata);
		if (error)
			return error;

		sysctl_teardown(&clog);

		config_cfattach_detach(iscsi_cd.cd_name, &iscsi_ca);
		config_cfdriver_detach(&iscsi_cd);
		devsw_detach(NULL, &iscsi_cdevsw);
#endif
		return 0;
		break;

	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
		break;

	default:
		return ENOTTY;
		break;
	}
}
