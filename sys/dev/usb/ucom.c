/*	$NetBSD: ucom.c,v 1.108.2.24 2016/10/27 07:46:19 skrll Exp $	*/

/*
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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
 * This code is very heavily based on the 16550 driver, com.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ucom.c,v 1.108.2.24 2016/10/27 07:46:19 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/queue.h>
#include <sys/kauth.h>
#include <sys/sysctl.h>
#include <sys/timepps.h>
#include <sys/rndsource.h>

#include <dev/usb/usb.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/ucomvar.h>

#include "ucom.h"

#include "locators.h"

#if NUCOM > 0

#ifdef USB_DEBUG
#ifndef UCOM_DEBUG
#define ucomdebug 0
#else
int ucomdebug = 0;

SYSCTL_SETUP(sysctl_hw_ucom_setup, "sysctl hw.ucom setup")
{
        int err;
        const struct sysctlnode *rnode;
        const struct sysctlnode *cnode;

        err = sysctl_createv(clog, 0, NULL, &rnode,
            CTLFLAG_PERMANENT, CTLTYPE_NODE, "ucom",
            SYSCTL_DESCR("ucom global controls"),
            NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

        if (err)
                goto fail;

        /* control debugging printfs */
        err = sysctl_createv(clog, 0, &rnode, &cnode,
            CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
            "debug", SYSCTL_DESCR("Enable debugging output"),
            NULL, 0, &ucomdebug, sizeof(ucomdebug), CTL_CREATE, CTL_EOL);
        if (err)
                goto fail;

        return;
fail:
        aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* UCOM_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)    USBHIST_LOGN(ucomdebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D) USBHIST_LOGN(ucomdebug,N,FMT,A,B,C,D)
#define UCOMHIST_FUNC()         USBHIST_FUNC()
#define UCOMHIST_CALLED(name)   USBHIST_CALLED(ucomdebug)

#define	UCOMCALLUNIT_MASK	TTCALLUNIT_MASK
#define	UCOMUNIT_MASK		TTUNIT_MASK
#define	UCOMDIALOUT_MASK	TTDIALOUT_MASK

#define	UCOMCALLUNIT(x)		TTCALLUNIT(x)
#define	UCOMUNIT(x)		TTUNIT(x)
#define	UCOMDIALOUT(x)		TTDIALOUT(x)

/*
 * XXX: We can submit multiple input/output buffers to the usb stack
 * to improve throughput, but the usb stack is too lame to deal with this
 * in a number of places.
 */
#define	UCOM_IN_BUFFS	1
#define	UCOM_OUT_BUFFS	1

struct ucom_buffer {
	SIMPLEQ_ENTRY(ucom_buffer) ub_link;
	struct usbd_xfer *ub_xfer;
	u_char *ub_data;
	u_int ub_len;
	u_int ub_index;
};

struct ucom_softc {
	device_t		sc_dev;		/* base device */

	struct usbd_device	*sc_udev;	/* USB device */
	struct usbd_interface	*sc_iface;	/* data interface */

	int			sc_bulkin_no;	/* bulk in endpoint address */
	struct usbd_pipe	*sc_bulkin_pipe;	/* bulk in pipe */
	u_int			sc_ibufsize;	/* read buffer size */
	u_int			sc_ibufsizepad;	/* read buffer size padded */
	struct ucom_buffer	sc_ibuff[UCOM_IN_BUFFS];
	SIMPLEQ_HEAD(, ucom_buffer) sc_ibuff_empty;
	SIMPLEQ_HEAD(, ucom_buffer) sc_ibuff_full;

	int			sc_bulkout_no;	/* bulk out endpoint address */
	struct usbd_pipe	*sc_bulkout_pipe;/* bulk out pipe */
	u_int			sc_obufsize;	/* write buffer size */
	u_int			sc_opkthdrlen;	/* header length of */
	struct ucom_buffer	sc_obuff[UCOM_OUT_BUFFS];
	SIMPLEQ_HEAD(, ucom_buffer) sc_obuff_free;
	SIMPLEQ_HEAD(, ucom_buffer) sc_obuff_full;

	struct usbd_pipe	*sc_ipipe;
	struct usbd_xfer	*sc_ixfer;
	struct usbd_pipe	*sc_opipe;
	struct usbd_xfer	*sc_oxfer;

	void			*sc_si;

	const struct ucom_methods *sc_methods;
	void                    *sc_parent;
	int			sc_portno;

	struct tty		*sc_tty;	/* our tty */
	u_char			sc_lsr;
	u_char			sc_msr;
	u_char			sc_mcr;
	volatile u_char		sc_rx_stopped;
	u_char			sc_rx_unblock;
	u_char			sc_tx_stopped;
	int			sc_swflags;

	u_char			sc_opening;	/* lock during open */
	u_char			sc_closing;	/* lock during close */
	int			sc_refcnt;
	bool			sc_dying;	/* disconnecting */

	struct pps_state	sc_pps_state;	/* pps state */

	krndsource_t		sc_rndsource;	/* random source */

	kmutex_t		sc_lock;
	kcondvar_t		sc_opencv;
	kcondvar_t		sc_detachcv;
};

dev_type_open(ucomopen);
dev_type_close(ucomclose);
dev_type_read(ucomread);
dev_type_write(ucomwrite);
dev_type_ioctl(ucomioctl);
dev_type_stop(ucomstop);
dev_type_tty(ucomtty);
dev_type_poll(ucompoll);

const struct cdevsw ucom_cdevsw = {
	.d_open = ucomopen,
	.d_close = ucomclose,
	.d_read = ucomread,
	.d_write = ucomwrite,
	.d_ioctl = ucomioctl,
	.d_stop = ucomstop,
	.d_tty = ucomtty,
	.d_poll = ucompoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY | D_MPSAFE
};

static void	ucom_cleanup(struct ucom_softc *);
static int	ucomparam(struct tty *, struct termios *);
static int	ucomhwiflow(struct tty *, int);
static void	ucomstart(struct tty *);
static void	ucom_shutdown(struct ucom_softc *);
static int	ucom_do_ioctl(struct ucom_softc *, u_long, void *,
			      int, struct lwp *);
static void	ucom_dtr(struct ucom_softc *, int);
static void	ucom_rts(struct ucom_softc *, int);
static void	ucom_break(struct ucom_softc *, int);
static void	tiocm_to_ucom(struct ucom_softc *, u_long, int);
static int	ucom_to_tiocm(struct ucom_softc *);

static void	ucom_submit_write(struct ucom_softc *, struct ucom_buffer *);
static void	ucom_write_status(struct ucom_softc *, struct ucom_buffer *,
		    usbd_status);

static void	ucomwritecb(struct usbd_xfer *, void *, usbd_status);
static void	ucom_read_complete(struct ucom_softc *);
static usbd_status ucomsubmitread(struct ucom_softc *, struct ucom_buffer *);
static void	ucom_softintr(void *);

int ucom_match(device_t, cfdata_t, void *);
void ucom_attach(device_t, device_t, void *);
int ucom_detach(device_t, int);
int ucom_activate(device_t, enum devact);
extern struct cfdriver ucom_cd;
CFATTACH_DECL_NEW(ucom, sizeof(struct ucom_softc), ucom_match, ucom_attach,
    ucom_detach, ucom_activate);

int
ucom_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

void
ucom_attach(device_t parent, device_t self, void *aux)
{
	struct ucom_softc *sc = device_private(self);
	struct ucom_attach_args *ucaa = aux;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	if (ucaa->ucaa_info != NULL)
		aprint_normal(": %s", ucaa->ucaa_info);
	aprint_normal("\n");

	prop_dictionary_set_int32(device_properties(self), "port",
	    ucaa->ucaa_portno);

	//KASSERT(ucaa->ucaa_bulkin != -1 || (ucaa->ucaa_ipipe && ucaa->ucaa_ixfer));
	//KASSERT(ucaa->ucaa_bulkout != -1 || (ucaa->ucaa_opipe && ucaa->ucaa_oxfer));

	sc->sc_dev = self;
	sc->sc_udev = ucaa->ucaa_device;
	sc->sc_iface = ucaa->ucaa_iface;
	sc->sc_bulkout_no = ucaa->ucaa_bulkout;
	sc->sc_bulkin_no = ucaa->ucaa_bulkin;
	sc->sc_ibufsize = ucaa->ucaa_ibufsize;
	sc->sc_ibufsizepad = ucaa->ucaa_ibufsizepad;
	sc->sc_obufsize = ucaa->ucaa_obufsize;
	sc->sc_opkthdrlen = ucaa->ucaa_opkthdrlen;
	sc->sc_methods = ucaa->ucaa_methods;
	sc->sc_parent = ucaa->ucaa_arg;
	sc->sc_portno = ucaa->ucaa_portno;

	sc->sc_lsr = 0;
	sc->sc_msr = 0;
	sc->sc_mcr = 0;
	sc->sc_tx_stopped = 0;
	sc->sc_swflags = 0;
	sc->sc_opening = 0;
	sc->sc_closing = 0;
	sc->sc_refcnt = 0;
	sc->sc_dying = false;

	sc->sc_si = softint_establish(SOFTINT_USB, ucom_softintr, sc);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	cv_init(&sc->sc_opencv, "ucomopen");
	cv_init(&sc->sc_detachcv, "ucomdtch");

	SIMPLEQ_INIT(&sc->sc_ibuff_empty);
	SIMPLEQ_INIT(&sc->sc_ibuff_full);
	SIMPLEQ_INIT(&sc->sc_obuff_free);
	SIMPLEQ_INIT(&sc->sc_obuff_full);

	memset(sc->sc_ibuff, 0, sizeof(sc->sc_ibuff));
	memset(sc->sc_obuff, 0, sizeof(sc->sc_obuff));

	DPRINTF("open pipes in=%d out=%d", sc->sc_bulkin_no, sc->sc_bulkout_no,
	    0, 0);

	struct ucom_buffer *ub;
	usbd_status err;
	int error;

	if (sc->sc_bulkin_no != -1) {
		/* Open the bulk pipes */
		err = usbd_open_pipe(sc->sc_iface, sc->sc_bulkin_no,
		    USBD_EXCLUSIVE_USE, &sc->sc_bulkin_pipe);
		if (err) {
			DPRINTF("open bulk in error (addr %d), err=%d",
			    sc->sc_bulkin_no, err, 0, 0);
			error = EIO;
			goto fail_0;
		}
		/* Allocate input buffers */
		for (ub = &sc->sc_ibuff[0]; ub != &sc->sc_ibuff[UCOM_IN_BUFFS];
		    ub++) {
			error = usbd_create_xfer(sc->sc_bulkin_pipe,
			    sc->sc_ibufsizepad, USBD_SHORT_XFER_OK, 0,
			    &ub->ub_xfer);
			if (error)
				goto fail_1;
			ub->ub_data = usbd_get_buffer(ub->ub_xfer);
		}

	}

	if (sc->sc_bulkout_no != -1) {
		err = usbd_open_pipe(sc->sc_iface, sc->sc_bulkout_no,
		    USBD_EXCLUSIVE_USE, &sc->sc_bulkout_pipe);
		if (err) {
			DPRINTF("open bulk out error (addr %d), err=%d",
			    sc->sc_bulkout_no, err, 0, 0);
			error = EIO;
			goto fail_1;
		}
		for (ub = &sc->sc_obuff[0]; ub != &sc->sc_obuff[UCOM_OUT_BUFFS];
		    ub++) {
			error = usbd_create_xfer(sc->sc_bulkout_pipe,
			    sc->sc_obufsize, 0, 0, &ub->ub_xfer);
			if (error)
				goto fail_2;
			ub->ub_data = usbd_get_buffer(ub->ub_xfer);
			SIMPLEQ_INSERT_TAIL(&sc->sc_obuff_free, ub, ub_link);
		}
	}

	if (sc->sc_ipipe && sc->sc_ixfer) {
		ub = &sc->sc_ibuff[0];
		ub->ub_data = usbd_get_buffer(sc->sc_ixfer);
		SIMPLEQ_INSERT_TAIL(&sc->sc_ibuff_empty, ub, ub_link);
	}
	if (sc->sc_opipe && sc->sc_oxfer) {
		ub = &sc->sc_obuff[0];
		ub->ub_data = usbd_get_buffer(sc->sc_oxfer);
		SIMPLEQ_INSERT_TAIL(&sc->sc_obuff_free, ub, ub_link);
	}

	struct tty *tp = tty_alloc();
	tp->t_oproc = ucomstart;
	tp->t_param = ucomparam;
	tp->t_hwiflow = ucomhwiflow;
	sc->sc_tty = tp;

	DPRINTF("tty_attach %p", tp, 0, 0, 0);
	tty_attach(tp);

	rnd_attach_source(&sc->sc_rndsource, device_xname(sc->sc_dev),
			  RND_TYPE_TTY, RND_FLAG_DEFAULT);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	return;

fail_2:
	for (ub = &sc->sc_obuff[0]; ub != &sc->sc_obuff[UCOM_OUT_BUFFS];
	    ub++) {
		if (ub->ub_xfer)
			usbd_destroy_xfer(ub->ub_xfer);
	}

	usbd_close_pipe(sc->sc_bulkout_pipe);

fail_1:
	for (ub = &sc->sc_ibuff[0]; ub != &sc->sc_ibuff[UCOM_IN_BUFFS];
	    ub++) {
		if (ub->ub_xfer)
			usbd_destroy_xfer(ub->ub_xfer);
	}
	usbd_close_pipe(sc->sc_bulkin_pipe);

fail_0:
	aprint_error_dev(self, "attach failed, error=%d\n", error);

	return;
}

int
ucom_detach(device_t self, int flags)
{
	struct ucom_softc *sc = device_private(self);
	struct tty *tp = sc->sc_tty;
	int maj, mn;
	int i;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("sc=%p flags=%d tp=%p", sc, flags, tp, 0);
	DPRINTF("... pipe=%d,%d", sc->sc_bulkin_no, sc->sc_bulkout_no, 0, 0);

	mutex_enter(&sc->sc_lock);
	sc->sc_dying = true;
	mutex_exit(&sc->sc_lock);

	pmf_device_deregister(self);

	if (sc->sc_bulkin_pipe != NULL)
		usbd_abort_pipe(sc->sc_bulkin_pipe);
	if (sc->sc_bulkout_pipe != NULL)
		usbd_abort_pipe(sc->sc_bulkout_pipe);

	mutex_enter(&sc->sc_lock);
	while (sc->sc_refcnt > 0) {
		/* Wake up anyone waiting */
		if (tp != NULL) {
			mutex_spin_enter(&tty_lock);
			CLR(tp->t_state, TS_CARR_ON);
			CLR(tp->t_cflag, CLOCAL | MDMBUF);
			ttyflush(tp, FREAD|FWRITE);
			mutex_spin_exit(&tty_lock);
		}
		/* Wait for processes to go away. */
		if (cv_timedwait(&sc->sc_detachcv, &sc->sc_lock, hz * 60)) {
			printf("%s: %s didn't detach\n", __func__,
			    device_xname(sc->sc_dev));
		}
	}

	softint_disestablish(sc->sc_si);
	mutex_exit(&sc->sc_lock);

	/* locate the major number */
	maj = cdevsw_lookup_major(&ucom_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	DPRINTF("maj=%d mn=%d", maj, mn, 0, 0);
	vdevgone(maj, mn, mn, VCHR);
	vdevgone(maj, mn | UCOMDIALOUT_MASK, mn | UCOMDIALOUT_MASK, VCHR);
	vdevgone(maj, mn | UCOMCALLUNIT_MASK, mn | UCOMCALLUNIT_MASK, VCHR);

	/* Detach and free the tty. */
	if (tp != NULL) {
		tty_detach(tp);
		tty_free(tp);
		sc->sc_tty = NULL;
	}

	for (i = 0; i < UCOM_IN_BUFFS; i++) {
		if (sc->sc_ibuff[i].ub_xfer != NULL)
			usbd_destroy_xfer(sc->sc_ibuff[i].ub_xfer);
	}

	for (i = 0; i < UCOM_OUT_BUFFS; i++) {
		if (sc->sc_obuff[i].ub_xfer != NULL)
			usbd_destroy_xfer(sc->sc_obuff[i].ub_xfer);
	}

	if (sc->sc_bulkin_pipe != NULL) {
		usbd_close_pipe(sc->sc_bulkin_pipe);
		sc->sc_bulkin_pipe = NULL;
	}

	if (sc->sc_bulkout_pipe != NULL) {
		usbd_close_pipe(sc->sc_bulkout_pipe);
		sc->sc_bulkout_pipe = NULL;
	}

	/* Detach the random source */
	rnd_detach_source(&sc->sc_rndsource);

	mutex_destroy(&sc->sc_lock);
	cv_destroy(&sc->sc_opencv);
	cv_destroy(&sc->sc_detachcv);

	return 0;
}

int
ucom_activate(device_t self, enum devact act)
{
	struct ucom_softc *sc = device_private(self);

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTFN(5, "%d", act, 0, 0, 0);

	switch (act) {
	case DVACT_DEACTIVATE:
		mutex_enter(&sc->sc_lock);
		sc->sc_dying = true;
		mutex_exit(&sc->sc_lock);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
ucom_shutdown(struct ucom_softc *sc)
{
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		ucom_dtr(sc, 0);
		/* XXX will only timeout */
		(void) kpause(ttclos, false, hz, &sc->sc_lock);
	}
}

int
ucomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = UCOMUNIT(dev);
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, unit);
	struct ucom_buffer *ub;
	int error;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return EIO;
	}

	if (!device_is_active(sc->sc_dev)) {
		mutex_exit(&sc->sc_lock);
		return ENXIO;
	}

	struct tty *tp = sc->sc_tty;

	DPRINTF("unit=%d, tp=%p", unit, tp, 0, 0);

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)) {
		mutex_exit(&sc->sc_lock);
		return EBUSY;
	}

	/*
	 * Wait while the device is initialized by the
	 * first opener or cleaned up by the last closer.
	 */
	while (sc->sc_opening || sc->sc_closing) {
		error = cv_wait_sig(&sc->sc_opencv, &sc->sc_lock);

		if (error) {
			mutex_exit(&sc->sc_lock);
			return error;
		}
	}

	sc->sc_opening = 1;

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		if (sc->sc_methods->ucom_open != NULL) {
			error = sc->sc_methods->ucom_open(sc->sc_parent,
							  sc->sc_portno);
			if (error) {
				ucom_cleanup(sc);
				sc->sc_opening = 0;
				cv_signal(&sc->sc_opencv);
				mutex_exit(&sc->sc_lock);
				return error;
			}
		}

		ucom_status_change(sc);

		/* Clear PPS capture state on first open. */
		mutex_spin_enter(&timecounter_lock);
		memset(&sc->sc_pps_state, 0, sizeof(sc->sc_pps_state));
		sc->sc_pps_state.ppscap = PPS_CAPTUREASSERT | PPS_CAPTURECLEAR;
		pps_init(&sc->sc_pps_state);
		mutex_spin_exit(&timecounter_lock);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		t.c_ospeed = TTYDEF_SPEED;
		t.c_cflag = TTYDEF_CFLAG;
		if (ISSET(sc->sc_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		/* Make sure ucomparam() will do something. */
		tp->t_ospeed = 0;
		(void) ucomparam(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.  Ditto RTS.
		 */
		ucom_dtr(sc, 1);
		ucom_rts(sc, 1);

		sc->sc_rx_unblock = 0;
		sc->sc_rx_stopped = 0;
		sc->sc_tx_stopped = 0;

		/*
		 * ucomsubmitread handles bulk vs other
		 */
		for (ub = &sc->sc_ibuff[0]; ub != &sc->sc_ibuff[UCOM_IN_BUFFS];
		    ub++) {
			if (ucomsubmitread(sc, ub) != USBD_NORMAL_COMPLETION) {
				error = EIO;
				goto fail_2;
			}
		}
	}
	sc->sc_opening = 0;
	cv_signal(&sc->sc_opencv);
	mutex_exit(&sc->sc_lock);

	error = ttyopen(tp, UCOMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return 0;

fail_2:
	if (sc->sc_bulkin_no != -1)
		usbd_abort_pipe(sc->sc_bulkin_pipe);
	if (sc->sc_bulkout_no != -1)
		usbd_abort_pipe(sc->sc_bulkout_pipe);

	mutex_enter(&sc->sc_lock);
	sc->sc_opening = 0;
	cv_signal(&sc->sc_opencv);
	mutex_exit(&sc->sc_lock);

	return error;

bad:
	mutex_spin_enter(&tty_lock);
	CLR(tp->t_state, TS_BUSY);
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		ucom_cleanup(sc);
	}
	mutex_spin_exit(&tty_lock);

	return error;
}

int
ucomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("unit=%d", UCOMUNIT(dev), 0, 0, 0);

	if (sc == NULL)
		return 0;

	mutex_enter(&sc->sc_lock);
	struct tty *tp = sc->sc_tty;

	while (sc->sc_closing)
		cv_wait(&sc->sc_opencv, &sc->sc_lock);
	sc->sc_closing = 1;

	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		goto out;
	}

	sc->sc_refcnt++;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		ucom_cleanup(sc);
	}

	if (sc->sc_methods->ucom_close != NULL)
		sc->sc_methods->ucom_close(sc->sc_parent, sc->sc_portno);

	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detachcv);

out:
	sc->sc_closing = 0;
	cv_signal(&sc->sc_opencv);

	mutex_exit(&sc->sc_lock);

	return 0;
}

int
ucomread(dev_t dev, struct uio *uio, int flag)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));
	int error;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	if (sc == NULL)
		return EIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return EIO;
	}

	struct tty *tp = sc->sc_tty;

	sc->sc_refcnt++;
	mutex_exit(&sc->sc_lock);

	error = ((*tp->t_linesw->l_read)(tp, uio, flag));

	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detachcv);
	mutex_exit(&sc->sc_lock);

	return error;
}

int
ucomwrite(dev_t dev, struct uio *uio, int flag)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));
	int error;

	if (sc == NULL)
		return EIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return EIO;
	}

	struct tty *tp = sc->sc_tty;

	sc->sc_refcnt++;
	mutex_exit(&sc->sc_lock);

	error = ((*tp->t_linesw->l_write)(tp, uio, flag));

	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detachcv);
	mutex_exit(&sc->sc_lock);

	return error;
}

int
ucompoll(dev_t dev, int events, struct lwp *l)
{
	struct ucom_softc *sc;
	int revents;

	sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));
	if (sc == NULL)
		return POLLHUP;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return POLLHUP;
	}
	struct tty *tp = sc->sc_tty;

	sc->sc_refcnt++;
	mutex_exit(&sc->sc_lock);

	revents = ((*tp->t_linesw->l_poll)(tp, events, l));

	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detachcv);
	mutex_exit(&sc->sc_lock);

	return revents;
}

struct tty *
ucomtty(dev_t dev)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));

	return sc != NULL ? sc->sc_tty : NULL;
}

int
ucomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, UCOMUNIT(dev));
	int error;

	if (sc == NULL)
		return EIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_lock);
		return EIO;
	}

	sc->sc_refcnt++;
	mutex_exit(&sc->sc_lock);

	error = ucom_do_ioctl(sc, cmd, data, flag, l);

	mutex_enter(&sc->sc_lock);
	if (--sc->sc_refcnt < 0)
		cv_broadcast(&sc->sc_detachcv);
	mutex_exit(&sc->sc_lock);

	return error;
}

static int
ucom_do_ioctl(struct ucom_softc *sc, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct tty *tp = sc->sc_tty;
	int error;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("cmd=0x%08lx", cmd, 0, 0, 0);

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	if (sc->sc_methods->ucom_ioctl != NULL) {
		error = sc->sc_methods->ucom_ioctl(sc->sc_parent,
		    sc->sc_portno, cmd, data, flag, l->l_proc);
		if (error != EPASSTHROUGH)
			return error;
	}

	error = 0;

	DPRINTF("our cmd=0x%08lx", cmd, 0, 0, 0);
	//mutex_enter(&tty_lock);

	switch (cmd) {
	case TIOCSBRK:
		ucom_break(sc, 1);
		break;

	case TIOCCBRK:
		ucom_break(sc, 0);
		break;

	case TIOCSDTR:
		ucom_dtr(sc, 1);
		break;

	case TIOCCDTR:
		ucom_dtr(sc, 0);
		break;

	case TIOCGFLAGS:
		mutex_enter(&sc->sc_lock);
		*(int *)data = sc->sc_swflags;
		mutex_exit(&sc->sc_lock);
		break;

	case TIOCSFLAGS:
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
		if (error)
			break;
		mutex_enter(&sc->sc_lock);
		sc->sc_swflags = *(int *)data;
		mutex_exit(&sc->sc_lock);
		break;

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
		tiocm_to_ucom(sc, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = ucom_to_tiocm(sc);
		break;

	case PPS_IOC_CREATE:
	case PPS_IOC_DESTROY:
	case PPS_IOC_GETPARAMS:
	case PPS_IOC_SETPARAMS:
	case PPS_IOC_GETCAP:
	case PPS_IOC_FETCH:
#ifdef PPS_SYNC
	case PPS_IOC_KCBIND:
#endif
		mutex_spin_enter(&timecounter_lock);
		error = pps_ioctl(cmd, data, &sc->sc_pps_state);
		mutex_spin_exit(&timecounter_lock);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	//mutex_exit(&tty_lock);

	return error;
}

static void
tiocm_to_ucom(struct ucom_softc *sc, u_long how, int ttybits)
{
	u_char combits;

	combits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(combits, UMCR_DTR);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(combits, UMCR_RTS);

	mutex_enter(&sc->sc_lock);
	switch (how) {
	case TIOCMBIC:
		CLR(sc->sc_mcr, combits);
		break;

	case TIOCMBIS:
		SET(sc->sc_mcr, combits);
		break;

	case TIOCMSET:
		CLR(sc->sc_mcr, UMCR_DTR | UMCR_RTS);
		SET(sc->sc_mcr, combits);
		break;
	}
	mutex_exit(&sc->sc_lock);

	if (how == TIOCMSET || ISSET(combits, UMCR_DTR))
		ucom_dtr(sc, (sc->sc_mcr & UMCR_DTR) != 0);
	if (how == TIOCMSET || ISSET(combits, UMCR_RTS))
		ucom_rts(sc, (sc->sc_mcr & UMCR_RTS) != 0);
}

static int
ucom_to_tiocm(struct ucom_softc *sc)
{
	u_char combits;
	int ttybits = 0;

	mutex_enter(&sc->sc_lock);
	combits = sc->sc_mcr;
	if (ISSET(combits, UMCR_DTR))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(combits, UMCR_RTS))
		SET(ttybits, TIOCM_RTS);

	combits = sc->sc_msr;
	if (ISSET(combits, UMSR_DCD))
		SET(ttybits, TIOCM_CD);
	if (ISSET(combits, UMSR_CTS))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(combits, UMSR_DSR))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(combits, UMSR_RI | UMSR_TERI))
		SET(ttybits, TIOCM_RI);

#if 0
XXX;
	if (sc->sc_ier != 0)
		SET(ttybits, TIOCM_LE);
#endif
	mutex_exit(&sc->sc_lock);

	return ttybits;
}

static void
ucom_break(struct ucom_softc *sc, int onoff)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("onoff=%d", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_BREAK, onoff);
}

static void
ucom_dtr(struct ucom_softc *sc, int onoff)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("onoff=%d", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_DTR, onoff);
}

static void
ucom_rts(struct ucom_softc *sc, int onoff)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("onoff=%d", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_RTS, onoff);
}

void
ucom_status_change(struct ucom_softc *sc)
{
	struct tty *tp = sc->sc_tty;
	u_char old_msr;

	if (sc->sc_methods->ucom_get_status != NULL) {
		old_msr = sc->sc_msr;
		sc->sc_methods->ucom_get_status(sc->sc_parent, sc->sc_portno,
		    &sc->sc_lsr, &sc->sc_msr);
		if (ISSET((sc->sc_msr ^ old_msr), UMSR_DCD)) {
			mutex_spin_enter(&timecounter_lock);
			pps_capture(&sc->sc_pps_state);
			pps_event(&sc->sc_pps_state,
			    (sc->sc_msr & UMSR_DCD) ?
			    PPS_CAPTUREASSERT :
			    PPS_CAPTURECLEAR);
			mutex_spin_exit(&timecounter_lock);

			(*tp->t_linesw->l_modem)(tp,
			    ISSET(sc->sc_msr, UMSR_DCD));
		}
	} else {
		sc->sc_lsr = 0;
		/* Assume DCD is present, if we have no chance to check it. */
		sc->sc_msr = UMSR_DCD;
	}
}

static int
ucomparam(struct tty *tp, struct termios *t)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd,
	    UCOMUNIT(tp->t_dev));
	int error;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	if (sc == NULL || sc->sc_dying)
		return EIO;

	/* Check requested parameters. */
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return EINVAL;

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(sc->sc_swflags, TIOCFLAG_SOFTCAR)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return 0;

	/* XXX lcr = ISSET(sc->sc_lcr, LCR_SBREAK) | cflag2lcr(t->c_cflag); */

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (sc->sc_methods->ucom_param != NULL) {
		error = sc->sc_methods->ucom_param(sc->sc_parent, sc->sc_portno,
			    t);
		if (error)
			return error;
	}

	/* XXX worry about CHWFLOW */

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	DPRINTF("l_modem", 0, 0, 0, 0);
	(void) (*tp->t_linesw->l_modem)(tp, ISSET(sc->sc_msr, UMSR_DCD));

#if 0
XXX what if the hardware is not open
	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (sc->sc_tx_stopped) {
			sc->sc_tx_stopped = 0;
			ucomstart(tp);
		}
	}
#endif

	return 0;
}

static int
ucomhwiflow(struct tty *tp, int block)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd,
	    UCOMUNIT(tp->t_dev));
	int old;

	if (sc == NULL)
		return 0;

	mutex_enter(&sc->sc_lock);
	old = sc->sc_rx_stopped;
	sc->sc_rx_stopped = (u_char)block;

	if (old && !block) {
		sc->sc_rx_unblock = 1;
		softint_schedule(sc->sc_si);
	}
	mutex_exit(&sc->sc_lock);

	return 1;
}

static void
ucomstart(struct tty *tp)
{
	struct ucom_softc *sc = device_lookup_private(&ucom_cd,
	    UCOMUNIT(tp->t_dev));
	struct ucom_buffer *ub;
	u_char *data;
	int cnt;

	if (sc == NULL)
		return;

	KASSERT(&sc->sc_lock);
	KASSERT(mutex_owned(&tty_lock));
	if (sc->sc_dying) {
		return;
	}

	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (sc->sc_tx_stopped)
		goto out;

	if (!ttypull(tp))
		goto out;

	/* Grab the first contiguous region of buffer space. */
	data = tp->t_outq.c_cf;
	cnt = ndqb(&tp->t_outq, 0);

	if (cnt == 0)
		goto out;

	ub = SIMPLEQ_FIRST(&sc->sc_obuff_free);
	if (ub == NULL) {
		SET(tp->t_state, TS_BUSY);
		goto out;
	}

	SIMPLEQ_REMOVE_HEAD(&sc->sc_obuff_free, ub_link);

	if (SIMPLEQ_FIRST(&sc->sc_obuff_free) == NULL)
		SET(tp->t_state, TS_BUSY);

	if (cnt > sc->sc_obufsize)
		cnt = sc->sc_obufsize;

	if (sc->sc_methods->ucom_write != NULL)
		sc->sc_methods->ucom_write(sc->sc_parent, sc->sc_portno,
					   ub->ub_data, data, &cnt);
	else
		memcpy(ub->ub_data, data, cnt);

	ub->ub_len = cnt;
	ub->ub_index = 0;

	SIMPLEQ_INSERT_TAIL(&sc->sc_obuff_full, ub, ub_link);

	softint_schedule(sc->sc_si);

 out:
	return;
}

void
ucomstop(struct tty *tp, int flag)
{
#if 0
	struct ucom_softc *sc =
	    device_lookup_private(&ucom_cd, UCOMUNIT(tp->t_dev));

	mutex_enter(&sc->sc_lock);
	mutex_spin_enter(&tty_lock);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* obuff_full -> obuff_free? */
		/* sc->sc_tx_stopped = 1; */
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	mutex_spin_exit(&tty_lock);
	mutex_exit(&sc->sc_lock);
#endif
}

static void
ucom_write_status(struct ucom_softc *sc, struct ucom_buffer *ub,
    usbd_status err)
{
	struct tty *tp = sc->sc_tty;
	uint32_t cc = ub->ub_len;

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (err) {
	case USBD_IN_PROGRESS:
		ub->ub_index = ub->ub_len;
		break;
	case USBD_STALLED:
		ub->ub_index = 0;
		softint_schedule(sc->sc_si);
		break;
	case USBD_NORMAL_COMPLETION:
		usbd_get_xfer_status(ub->ub_xfer, NULL, NULL, &cc, NULL);
		rnd_add_uint32(&sc->sc_rndsource, cc);
		/*FALLTHROUGH*/
	default:
		SIMPLEQ_REMOVE_HEAD(&sc->sc_obuff_full, ub_link);
		SIMPLEQ_INSERT_TAIL(&sc->sc_obuff_free, ub, ub_link);
		cc -= sc->sc_opkthdrlen;

		mutex_spin_enter(&tty_lock);
		CLR(tp->t_state, TS_BUSY);
		if (ISSET(tp->t_state, TS_FLUSH))
			CLR(tp->t_state, TS_FLUSH);
		else
			ndflush(&tp->t_outq, cc);
		mutex_spin_exit(&tty_lock);

		if (err != USBD_CANCELLED && err != USBD_IOERROR &&
		    !sc->sc_dying) {
			if ((ub = SIMPLEQ_FIRST(&sc->sc_obuff_full)) != NULL)
				ucom_submit_write(sc, ub);

			mutex_spin_enter(&tty_lock);
			(*tp->t_linesw->l_start)(tp);
			mutex_spin_exit(&tty_lock);
		}
		break;
	}
}

static void
ucom_submit_write(struct ucom_softc *sc, struct ucom_buffer *ub)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	usbd_setup_xfer(ub->ub_xfer, sc, ub->ub_data, ub->ub_len,
	    0, USBD_NO_TIMEOUT, ucomwritecb);

	ucom_write_status(sc, ub, usbd_transfer(ub->ub_xfer));
}

static void
ucomwritecb(struct usbd_xfer *xfer, void *p, usbd_status status)
{
	struct ucom_softc *sc = (struct ucom_softc *)p;

	mutex_enter(&sc->sc_lock);
	ucom_write_status(sc, SIMPLEQ_FIRST(&sc->sc_obuff_full), status);
	mutex_exit(&sc->sc_lock);

}

static void
ucom_softintr(void *arg)
{
	struct ucom_softc *sc = arg;

	mutex_enter(&sc->sc_lock);
	struct tty *tp = sc->sc_tty;
	mutex_enter(&tty_lock);
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		mutex_exit(&tty_lock);
		mutex_exit(&sc->sc_lock);
		return;
	}
	mutex_exit(&tty_lock);

	struct ucom_buffer *ub = SIMPLEQ_FIRST(&sc->sc_obuff_full);

	if (ub != NULL && ub->ub_index == 0)
		ucom_submit_write(sc, ub);

	if (sc->sc_rx_unblock)
		ucom_read_complete(sc);

	mutex_exit(&sc->sc_lock);
}

static void
ucom_read_complete(struct ucom_softc *sc)
{
	int (*rint)(int, struct tty *);
	struct ucom_buffer *ub;
	struct tty *tp;

	KASSERT(mutex_owned(&sc->sc_lock));

	tp = sc->sc_tty;
	rint = tp->t_linesw->l_rint;
	ub = SIMPLEQ_FIRST(&sc->sc_ibuff_full);

	while (ub != NULL && !sc->sc_rx_stopped) {

		/* XXX ttyinput takes tty_lock */
		while (ub->ub_index < ub->ub_len && !sc->sc_rx_stopped) {
			/* Give characters to tty layer. */
			if ((*rint)(ub->ub_data[ub->ub_index], tp) == -1) {
				/* Overflow: drop remainder */
				ub->ub_index = ub->ub_len;
			} else
				ub->ub_index++;
		}

		if (ub->ub_index == ub->ub_len) {
			SIMPLEQ_REMOVE_HEAD(&sc->sc_ibuff_full, ub_link);

			ucomsubmitread(sc, ub);

			ub = SIMPLEQ_FIRST(&sc->sc_ibuff_full);
		}
	}

	sc->sc_rx_unblock = (ub != NULL);
}

static usbd_status
ucomsubmitread(struct ucom_softc *sc, struct ucom_buffer *ub)
{
	usbd_status err;

	if (sc->sc_bulkin_no != -1) {
		usbd_setup_xfer(ub->ub_xfer, sc, ub->ub_data, sc->sc_ibufsize,
		    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, ucomreadcb);

		if ((err = usbd_transfer(ub->ub_xfer)) != USBD_IN_PROGRESS) {
			/* XXX: Recover from this, please! */
			printf("%s: err=%s\n", __func__, usbd_errstr(err));
			return err;
		}
	}

	SIMPLEQ_INSERT_TAIL(&sc->sc_ibuff_empty, ub, ub_link);

	return USBD_NORMAL_COMPLETION;
}

void
ucomreadcb(struct usbd_xfer *xfer, void *p, usbd_status status)
{
	struct ucom_softc *sc = (struct ucom_softc *)p;
	struct tty *tp = sc->sc_tty;
	struct ucom_buffer *ub;
	uint32_t cc;
	u_char *cp;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	if (status == USBD_CANCELLED)
		return;

	mutex_enter(&sc->sc_lock);
	if (status == USBD_IOERROR ||
	    sc->sc_dying) {
		DPRINTF("dying", 0, 0, 0, 0);
		/* Send something to wake upper layer */
		(tp->t_linesw->l_rint)('\n', tp);
		mutex_spin_enter(&tty_lock);	/* XXX */
		ttwakeup(tp);
		mutex_spin_exit(&tty_lock);	/* XXX */
		mutex_exit(&sc->sc_lock);
		return;
	}

	ub = SIMPLEQ_FIRST(&sc->sc_ibuff_empty);
	SIMPLEQ_REMOVE_HEAD(&sc->sc_ibuff_empty, ub_link);

	if (status == USBD_STALLED) {
		if (sc->sc_bulkin_no != -1)
			usbd_clear_endpoint_stall_async(sc->sc_bulkin_pipe);
		if (sc->sc_ipipe)
			usbd_clear_endpoint_stall_async(sc->sc_ipipe);
		ucomsubmitread(sc, ub);
		mutex_exit(&sc->sc_lock);
		return;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		printf("ucomreadcb: wonky status=%s\n", usbd_errstr(status));
		mutex_exit(&sc->sc_lock);
		return;
	}

	usbd_get_xfer_status(xfer, NULL, (void *)&cp, &cc, NULL);

#ifdef UCOM_DEBUG
	/* This is triggered by uslsa(4) occasionally. */
	if ((ucomdebug > 0) && (cc == 0)) {
		device_printf(sc->sc_dev, "ucomreadcb: zero length xfer!\n");
	}
#endif

	KDASSERT(cp == ub->ub_data);

	rnd_add_uint32(&sc->sc_rndsource, cc);

	if (sc->sc_opening) {
		ucomsubmitread(sc, ub);
		mutex_exit(&sc->sc_lock);
		return;
	}

	ub->ub_data = usbd_get_buffer(xfer);
	if (sc->sc_methods->ucom_read != NULL) {
		sc->sc_methods->ucom_read(sc->sc_parent, sc->sc_portno,
		    &cp, &cc);
		ub->ub_index = (u_int)(cp - ub->ub_data);
	} else
		ub->ub_index = 0;

	ub->ub_len = cc;

	SIMPLEQ_INSERT_TAIL(&sc->sc_ibuff_full, ub, ub_link);

	ucom_read_complete(sc);
	mutex_exit(&sc->sc_lock);
}

static void
ucom_cleanup(struct ucom_softc *sc)
{

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("aborting pipes", 0, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	ucom_shutdown(sc);
	if (sc->sc_bulkin_pipe != NULL) {
		usbd_abort_pipe(sc->sc_bulkin_pipe);
	}
	if (sc->sc_bulkout_pipe != NULL) {
		usbd_abort_pipe(sc->sc_bulkout_pipe);
	}
}

#endif /* NUCOM > 0 */

int
ucomprint(void *aux, const char *pnp)
{
	struct ucom_attach_args *ucaa = aux;

	if (pnp)
		aprint_normal("ucom at %s", pnp);
	if (ucaa->ucaa_portno != UCOM_UNK_PORTNO)
		aprint_normal(" portno %d", ucaa->ucaa_portno);
	return UNCONF;
}

int
ucomsubmatch(device_t parent, cfdata_t cf,
	     const int *ldesc, void *aux)
{
	struct ucom_attach_args *ucaa = aux;

	if (ucaa->ucaa_portno != UCOM_UNK_PORTNO &&
	    cf->cf_loc[UCOMBUSCF_PORTNO] != UCOMBUSCF_PORTNO_DEFAULT &&
	    cf->cf_loc[UCOMBUSCF_PORTNO] != ucaa->ucaa_portno)
		return 0;
	return config_match(parent, cf, aux);
}
