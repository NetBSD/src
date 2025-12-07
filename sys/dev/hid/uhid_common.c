/*	$NetBSD: uhid_common.c,v 1.1 2025/12/07 10:05:10 jmcneill Exp $	*/

/*
 * Copyright (c) 1998, 2004, 2008, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna23.net).
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

/*
 * HID spec: http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uhid_common.c,v 1.1 2025/12/07 10:05:10 jmcneill Exp $");

#ifdef _KERNEL_OPT
#include "opt_hid.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/hid/hid.h>
#include <dev/hid/hidev.h>
#include <dev/hid/uhid.h>

#include <dev/usb/usbhid.h>

#include "ioconf.h"

#ifdef UHID_DEBUG
#define DPRINTF(x)	if (uhiddebug) printf x
#define DPRINTFN(n,x)	if (uhiddebug>(n)) printf x
int	uhiddebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define	UHIDUNIT(dev)	(minor(dev))
#define	UHID_CHUNK	128	/* chunk size for read */
#define	UHID_BSIZE	1020	/* buffer size */

static dev_type_open(uhidopen);
static dev_type_cancel(uhidcancel);
static dev_type_close(uhidclose);
static dev_type_read(uhidread);
static dev_type_write(uhidwrite);
static dev_type_ioctl(uhidioctl);
static dev_type_poll(uhidpoll);
static dev_type_kqfilter(uhidkqfilter);

const struct cdevsw uhid_cdevsw = {
	.d_open = uhidopen,
	.d_cancel = uhidcancel,
	.d_close = uhidclose,
	.d_read = uhidread,
	.d_write = uhidwrite,
	.d_ioctl = uhidioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = uhidpoll,
	.d_mmap = nommap,
	.d_kqfilter = uhidkqfilter,
	.d_discard = nodiscard,
	.d_cfdriver = &uhid_cd,
	.d_devtounit = dev_minor_unit,
	.d_flag = D_OTHER
};

static void uhid_intr(void *, void *, u_int);

void
uhid_attach_common(struct uhid_softc *sc)
{
	int size, repid;
	void *desc;

	selinit(&sc->sc_rsel);

	hidev_get_report_desc(sc->sc_hidev, &desc, &size);
	repid = sc->sc_report_id;
	sc->sc_isize = hid_report_size(desc, size, hid_input,   repid);
	sc->sc_osize = hid_report_size(desc, size, hid_output,  repid);
	sc->sc_fsize = hid_report_size(desc, size, hid_feature, repid);
	sc->sc_raw =  hid_is_collection(desc, size, repid,
	    HID_USAGE2(HUP_FIDO, HUF_U2FHID));

	aprint_naive("\n");
	aprint_normal(": input=%d, output=%d, feature=%d\n",
	       sc->sc_isize, sc->sc_osize, sc->sc_fsize);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	cv_init(&sc->sc_cv, "uhidrea");

	if (!pmf_device_register(sc->sc_dev, NULL, NULL)) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");
	}
}

int
uhid_detach_common(struct uhid_softc *sc)
{
	int maj, mn;

	DPRINTF(("uhid_detach: sc=%p\n", sc));

	pmf_device_deregister(sc->sc_dev);

	/* locate the major number */
	maj = cdevsw_lookup_major(&uhid_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = device_unit(sc->sc_dev);
	vdevgone(maj, mn, mn, VCHR);

	KASSERTMSG(sc->sc_open == UHID_CLOSED, "open=%d", (int)sc->sc_open);

	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_lock);
	seldestroy(&sc->sc_rsel);

	return 0;
}

static void
uhid_intr(void *cookie, void *data, u_int len)
{
	struct uhid_softc *sc = cookie;

#ifdef UHID_DEBUG
	if (uhiddebug > 5) {
		uint32_t i;

		DPRINTF(("uhid_intr: data ="));
		for (i = 0; i < len; i++)
			DPRINTF((" %02x", ((u_char *)data)[i]));
		DPRINTF(("\n"));
	}
#endif

	mutex_enter(&sc->sc_lock);
	(void)b_to_q(data, len, &sc->sc_q);

	DPRINTFN(5, ("uhid_intr: waking %p\n", &sc->sc_q));
	cv_broadcast(&sc->sc_cv);
	selnotify(&sc->sc_rsel, 0, NOTE_SUBMIT);
	if (atomic_load_relaxed(&sc->sc_async) != NULL) {
		mutex_enter(&proc_lock);
		if (sc->sc_async != NULL) {
			DPRINTFN(3, ("uhid_intr: sending SIGIO to %jd\n",
				(intmax_t)sc->sc_async->p_pid));
			psignal(sc->sc_async, SIGIO);
		}
		mutex_exit(&proc_lock);
	}
	mutex_exit(&sc->sc_lock);
}

static int
uhidopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));
	int error;

	DPRINTF(("uhidopen: sc=%p\n", sc));
	if (sc == NULL)
		return ENXIO;

	/*
	 * Try to open.  If closing in progress, give up.  If already
	 * open (or opening), fail -- opens are exclusive.
	 */
	mutex_enter(&sc->sc_lock);
	if (sc->sc_open != UHID_CLOSED || sc->sc_closing) {
		mutex_exit(&sc->sc_lock);
		return EBUSY;
	}
	sc->sc_open = UHID_OPENING;
	atomic_store_relaxed(&sc->sc_state, 0);
	mutex_exit(&sc->sc_lock);

	/* uhid interrupts aren't enabled yet, so setup sc_q now */
	if (clalloc(&sc->sc_q, UHID_BSIZE, 0) == -1) {
		error = ENOMEM;
		goto fail0;
	}

	/* Allocate an output buffer if needed.  */
	if (sc->sc_osize > 0)
		sc->sc_obuf = kmem_alloc(sc->sc_osize, KM_SLEEP);
	else
		sc->sc_obuf = NULL;

	/* Paranoia: reset SIGIO before enabling interrupts.  */
	mutex_enter(&proc_lock);
	atomic_store_relaxed(&sc->sc_async, NULL);
	mutex_exit(&proc_lock);

	/* Open the hidev -- after this point we can get interrupts.  */
	error = hidev_open(sc->sc_hidev, &uhid_intr, sc);
	if (error)
		goto fail1;

	/* We are open for business.  */
	mutex_enter(&sc->sc_lock);
	sc->sc_open = UHID_OPEN;
	mutex_exit(&sc->sc_lock);

	return 0;

fail1:	selnotify(&sc->sc_rsel, POLLHUP, 0);
	mutex_enter(&proc_lock);
	atomic_store_relaxed(&sc->sc_async, NULL);
	mutex_exit(&proc_lock);
	if (sc->sc_osize > 0) {
		kmem_free(sc->sc_obuf, sc->sc_osize);
		sc->sc_obuf = NULL;
	}
	clfree(&sc->sc_q);
fail0:	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_open == UHID_OPENING);
	sc->sc_open = UHID_CLOSED;
	atomic_store_relaxed(&sc->sc_state, 0);
	mutex_exit(&sc->sc_lock);
	return error;
}

static int
uhidcancel(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));

	DPRINTF(("uhidcancel: sc=%p\n", sc));
	if (sc == NULL)
		return 0;

	/*
	 * Mark it closing, wake any waiters, and suspend output.
	 * After this point, no new xfers can be submitted.
	 */
	mutex_enter(&sc->sc_lock);
	sc->sc_closing = true;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	hidev_stop(sc->sc_hidev);

	return 0;
}

static int
uhidclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));

	DPRINTF(("uhidclose: sc=%p\n", sc));
	if (sc == NULL)
		return 0;

	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_closing);
	KASSERTMSG(sc->sc_open == UHID_OPEN || sc->sc_open == UHID_CLOSED,
	    "sc_open=%d", sc->sc_open);
	if (sc->sc_open == UHID_CLOSED)
		goto out;

	/* Release the lock while we free things.  */
	mutex_exit(&sc->sc_lock);

	/* Prevent further interrupts.  */
	hidev_close(sc->sc_hidev);

	/* Hang up all select/poll.  */
	selnotify(&sc->sc_rsel, POLLHUP, 0);

	/* Reset SIGIO.  */
	mutex_enter(&proc_lock);
	atomic_store_relaxed(&sc->sc_async, NULL);
	mutex_exit(&proc_lock);

	/* Free the buffer and queue.  */
	if (sc->sc_osize > 0) {
		kmem_free(sc->sc_obuf, sc->sc_osize);
		sc->sc_obuf = NULL;
	}
	clfree(&sc->sc_q);

	mutex_enter(&sc->sc_lock);

	/* All set.  We are now closed.  */
	sc->sc_open = UHID_CLOSED;
out:	KASSERT(sc->sc_open == UHID_CLOSED);
	sc->sc_closing = false;
	atomic_store_relaxed(&sc->sc_state, 0);
	mutex_exit(&sc->sc_lock);

	return 0;
}

static int
uhidread(dev_t dev, struct uio *uio, int flag)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));
	int error = 0;
	int extra;
	size_t length;
	u_char buffer[UHID_CHUNK];
	usbd_status err;

	DPRINTFN(1, ("uhidread\n"));
	if (atomic_load_relaxed(&sc->sc_state) & UHID_IMMED) {
		DPRINTFN(1, ("uhidread immed\n"));
		extra = sc->sc_report_id != 0;
		if (sc->sc_isize + extra > sizeof(buffer))
			return ENOBUFS;
		err = hidev_get_report(sc->sc_hidev, UHID_INPUT_REPORT,
				       buffer, sc->sc_isize + extra);
		if (err)
			return EIO;
		return uiomove(buffer+extra, sc->sc_isize, uio);
	}

	mutex_enter(&sc->sc_lock);
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			mutex_exit(&sc->sc_lock);
			return EWOULDBLOCK;
		}
		if (sc->sc_closing) {
			mutex_exit(&sc->sc_lock);
			return EIO;
		}
		DPRINTFN(5, ("uhidread: sleep on %p\n", &sc->sc_q));
		error = cv_wait_sig(&sc->sc_cv, &sc->sc_lock);
		DPRINTFN(5, ("uhidread: woke, error=%d\n", error));
		if (error) {
			break;
		}
	}

	/* Transfer as many chunks as possible. */
	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0 && !error) {
		length = uimin(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from the input queue. */
		(void) q_to_b(&sc->sc_q, buffer, length);
		DPRINTFN(5, ("uhidread: got %lu chars\n", (u_long)length));

		/* Copy the data to the user process. */
		mutex_exit(&sc->sc_lock);
		if ((error = uiomove(buffer, length, uio)) != 0)
			return error;
		mutex_enter(&sc->sc_lock);
	}

	mutex_exit(&sc->sc_lock);
	return error;
}

static int
uhidwrite(dev_t dev, struct uio *uio, int flag)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));
	int error;
	int size;
	usbd_status err;

	DPRINTFN(1, ("uhidwrite\n"));

	size = sc->sc_osize;
	if (uio->uio_resid != size || size == 0)
		return EINVAL;
	error = uiomove(sc->sc_obuf, size, uio);
#ifdef UHID_DEBUG
	if (uhiddebug > 5) {
		uint32_t i;

		DPRINTF(("%s: outdata[%d] =", device_xname(sc->sc_dev),
		    error));
		for (i = 0; i < size; i++)
			DPRINTF((" %02x", sc->sc_obuf[i]));
		DPRINTF(("\n"));
	}
#endif
	if (!error) {
		if (sc->sc_raw)
			err = hidev_write(sc->sc_hidev, sc->sc_obuf, size);
		else
			err = hidev_set_report(sc->sc_hidev,
			    UHID_OUTPUT_REPORT, sc->sc_obuf, size);
		if (err) {
			DPRINTF(("%s: err = %d\n",
			    device_xname(sc->sc_dev), err));
			error = EIO;
		}
	}

	return error;
}

static int
uhidioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));
	struct usb_ctl_report_desc *rd;
	struct usb_ctl_report *re;
	u_char buffer[UHID_CHUNK];
	int size, extra;
	usbd_status err;
	void *desc;

	DPRINTFN(2, ("uhidioctl: cmd=%lx\n", cmd));

	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIOASYNC:
		mutex_enter(&proc_lock);
		if (*(int *)addr) {
			if (sc->sc_async != NULL) {
				mutex_exit(&proc_lock);
				return EBUSY;
			}
			atomic_store_relaxed(&sc->sc_async, l->l_proc);
			DPRINTF(("uhid_do_ioctl: FIOASYNC %p\n", l->l_proc));
		} else
			atomic_store_relaxed(&sc->sc_async, NULL);
		mutex_exit(&proc_lock);
		break;

	/* XXX this is not the most general solution. */
	case TIOCSPGRP:
		mutex_enter(&proc_lock);
		if (sc->sc_async == NULL) {
			mutex_exit(&proc_lock);
			return EINVAL;
		}
		if (*(int *)addr != sc->sc_async->p_pgid) {
			mutex_exit(&proc_lock);
			return EPERM;
		}
		mutex_exit(&proc_lock);
		break;

	case FIOSETOWN:
		mutex_enter(&proc_lock);
		if (sc->sc_async == NULL) {
			mutex_exit(&proc_lock);
			return EINVAL;
		}
		if (-*(int *)addr != sc->sc_async->p_pgid
		    && *(int *)addr != sc->sc_async->p_pid) {
			mutex_exit(&proc_lock);
			return EPERM;
		}
		mutex_exit(&proc_lock);
		break;

	case USB_HID_GET_RAW:
		*(int *)addr = sc->sc_raw;
		break;

	case USB_HID_SET_RAW:
		sc->sc_raw = *(int *)addr;
		break;

	case USB_GET_REPORT_DESC:
		hidev_get_report_desc(sc->sc_hidev, &desc, &size);
		rd = (struct usb_ctl_report_desc *)addr;
		size = uimin(size, sizeof(rd->ucrd_data));
		rd->ucrd_size = size;
		memcpy(rd->ucrd_data, desc, size);
		break;

	case USB_SET_IMMED:
		if (*(int *)addr) {
			extra = sc->sc_report_id != 0;
			if (sc->sc_isize + extra > sizeof(buffer))
				return ENOBUFS;
			err = hidev_get_report(sc->sc_hidev, UHID_INPUT_REPORT,
					       buffer, sc->sc_isize + extra);
			if (err)
				return EOPNOTSUPP;

			atomic_or_32(&sc->sc_state, UHID_IMMED);
		} else
			atomic_and_32(&sc->sc_state, ~UHID_IMMED);
		break;

	case USB_GET_REPORT:
		re = (struct usb_ctl_report *)addr;
		switch (re->ucr_report) {
		case UHID_INPUT_REPORT:
			size = sc->sc_isize;
			break;
		case UHID_OUTPUT_REPORT:
			size = sc->sc_osize;
			break;
		case UHID_FEATURE_REPORT:
			size = sc->sc_fsize;
			break;
		default:
			return EINVAL;
		}
		extra = sc->sc_report_id != 0;
		if (size + extra > sizeof(re->ucr_data))
			return ENOBUFS;
		err = hidev_get_report(sc->sc_hidev, re->ucr_report,
		    re->ucr_data, size + extra);
		if (extra)
			memmove(re->ucr_data, re->ucr_data+1, size);
		if (err)
			return EIO;
		break;

	case USB_SET_REPORT:
		re = (struct usb_ctl_report *)addr;
		switch (re->ucr_report) {
		case UHID_INPUT_REPORT:
			size = sc->sc_isize;
			break;
		case UHID_OUTPUT_REPORT:
			size = sc->sc_osize;
			break;
		case UHID_FEATURE_REPORT:
			size = sc->sc_fsize;
			break;
		default:
			return EINVAL;
		}
		if (size > sizeof(re->ucr_data))
			return ENOBUFS;
		err = hidev_set_report(sc->sc_hidev, re->ucr_report,
		    re->ucr_data, size);
		if (err)
			return EIO;
		break;

	case USB_GET_REPORT_ID:
		*(int *)addr = sc->sc_report_id;
		break;

	default:
		if (sc->sc_ioctl != NULL) {
			return sc->sc_ioctl(sc, cmd, addr, flag, l);
		}
		return EINVAL;
	}
	return 0;
}

static int
uhidpoll(dev_t dev, int events, struct lwp *l)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));
	int revents = 0;

	mutex_enter(&sc->sc_lock);
	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_q.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}
	mutex_exit(&sc->sc_lock);

	return revents;
}

static void
filt_uhidrdetach(struct knote *kn)
{
	struct uhid_softc *sc = kn->kn_hook;

	mutex_enter(&sc->sc_lock);
	selremove_knote(&sc->sc_rsel, kn);
	mutex_exit(&sc->sc_lock);
}

static int
filt_uhidread(struct knote *kn, long hint)
{
	struct uhid_softc *sc = kn->kn_hook;

	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&sc->sc_lock));
	else
		mutex_enter(&sc->sc_lock);

	kn->kn_data = sc->sc_q.c_cc;

	if (hint == NOTE_SUBMIT)
		KASSERT(mutex_owned(&sc->sc_lock));
	else
		mutex_exit(&sc->sc_lock);

	return kn->kn_data > 0;
}

static const struct filterops uhidread_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_uhidrdetach,
	.f_event = filt_uhidread,
};

static int
uhidkqfilter(dev_t dev, struct knote *kn)
{
	struct uhid_softc *sc = device_lookup_private(&uhid_cd, UHIDUNIT(dev));

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &uhidread_filtops;
		kn->kn_hook = sc;
		mutex_enter(&sc->sc_lock);
		selrecord_knote(&sc->sc_rsel, kn);
		mutex_exit(&sc->sc_lock);
		return 0;

	case EVFILT_WRITE:
		kn->kn_fop = &seltrue_filtops;
		return 0;

	default:
		return EINVAL;
	}
}
