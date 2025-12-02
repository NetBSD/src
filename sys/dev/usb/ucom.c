/*	$NetBSD: ucom.c,v 1.152 2025/12/02 01:16:41 manu Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ucom.c,v 1.152 2025/12/02 01:16:41 manu Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_ntp.h"
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/rndsource.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/timepps.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <dev/cons.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbhist.h>

#include <dev/usb/ucomvar.h>

#include <ddb/db_active.h>

#include "ucom.h"
#include "locators.h"
#include "ioconf.h"

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

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(ucomdebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(ucomdebug,N,FMT,A,B,C,D)
#define UCOMHIST_FUNC()		USBHIST_FUNC()
#define UCOMHIST_CALLED(name)	USBHIST_CALLED(ucomdebug)

#define	UCOMCALLUNIT_MASK	TTCALLUNIT_MASK
#define	UCOMUNIT_MASK		TTUNIT_MASK
#define	UCOMDIALOUT_MASK	TTDIALOUT_MASK

#define	UCOMCALLUNIT(x)		TTCALLUNIT(x)
#define	UCOMUNIT(x)		TTUNIT(x)
#define	UCOMDIALOUT(x)		TTDIALOUT(x)
#define	ucom_unit		tty_unit

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

	struct usbd_device *	sc_udev;	/* USB device */
	struct usbd_interface *	sc_iface;	/* data interface */

	int			sc_bulkin_no;	/* bulk in endpoint address */
	struct usbd_pipe *	sc_bulkin_pipe;/* bulk in pipe */
	u_int			sc_ibufsize;	/* read buffer size */
	u_int			sc_ibufsizepad;	/* read buffer size padded */
	struct ucom_buffer	sc_ibuff[UCOM_IN_BUFFS];
	SIMPLEQ_HEAD(, ucom_buffer) sc_ibuff_empty;
	SIMPLEQ_HEAD(, ucom_buffer) sc_ibuff_full;

	int			sc_bulkout_no;	/* bulk out endpoint address */
	struct usbd_pipe *	sc_bulkout_pipe;/* bulk out pipe */
	u_int			sc_obufsize;	/* write buffer size */
	u_int			sc_opkthdrlen;	/* header length of */
	struct ucom_buffer	sc_obuff[UCOM_OUT_BUFFS];
	SIMPLEQ_HEAD(, ucom_buffer) sc_obuff_free;
	SIMPLEQ_HEAD(, ucom_buffer) sc_obuff_full;

	void			*sc_si;

	const struct ucom_methods *sc_methods;
	void			*sc_parent;
	int			sc_portno;

	struct tty		*sc_tty;	/* our tty */
	u_char			sc_lsr;
	u_char			sc_msr;
	u_char			sc_mcr;
	volatile u_char		sc_rx_stopped;
	u_char			sc_rx_unblock;
	u_char			sc_tx_stopped;
	int			sc_swflags;

	enum ucom_state {
	    UCOM_DEAD,
	    UCOM_ATTACHED,
	    UCOM_OPENING,
	    UCOM_OPEN
	}			sc_state;
	bool			sc_closing;	/* software is closing */
	bool			sc_dying;	/* hardware is gone */
	struct ucom_cnstate	*sc_cn;		/* console state */

	struct pps_state	sc_pps_state;	/* pps state */

	krndsource_t		sc_rndsource;	/* random source */

	kmutex_t		sc_lock;
	kcondvar_t		sc_statecv;

	struct timeval		sc_hup_time;
};

dev_type_open(ucomopen);
dev_type_cancel(ucomcancel);
dev_type_close(ucomclose);
dev_type_read(ucomread);
dev_type_write(ucomwrite);
dev_type_ioctl(ucomioctl);
dev_type_stop(ucomstop);
dev_type_tty(ucomtty);
dev_type_poll(ucompoll);

const struct cdevsw ucom_cdevsw = {
	.d_open = ucomopen,
	.d_cancel = ucomcancel,
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
	.d_cfdriver = &ucom_cd,
	.d_devtounit = ucom_unit,
	.d_flag = D_TTY | D_MPSAFE
};

static void	ucom_cleanup(struct ucom_softc *, int);
static int	ucomparam(struct tty *, struct termios *);
static int	ucomhwiflow(struct tty *, int);
static void	ucomstart(struct tty *);
static void	ucom_shutdown(struct ucom_softc *);
static void	ucom_dtr(struct ucom_softc *, int);
static void	ucom_rts(struct ucom_softc *, int);
static void	ucom_break(struct ucom_softc *, int);
static void	tiocm_to_ucom(struct ucom_softc *, u_long, int);
static int	ucom_to_tiocm(struct ucom_softc *);

static void	ucomreadcb(struct usbd_xfer *, void *, usbd_status);
static void	ucom_submit_write(struct ucom_softc *, struct ucom_buffer *);
static void	ucom_write_status(struct ucom_softc *, struct ucom_buffer *,
		    usbd_status);

static void	ucomwritecb(struct usbd_xfer *, void *, usbd_status);
static void	ucom_read_complete(struct ucom_softc *);
static int	ucomsubmitread(struct ucom_softc *, struct ucom_buffer *);
static void	ucom_softintr(void *);

static bool	ucom_polling(struct ucom_softc *);
static void	ucom_mutex_enter(struct ucom_softc *);
static void	ucom_mutex_exit(struct ucom_softc *);
static bool	ucom_xfer_done(struct usbd_xfer *);

static void	ucom_cninit(struct ucom_softc *);
static void	ucom_cnread_complete(struct ucom_softc *);
static int	ucom_cnput_bytes(struct ucom_softc *);
static void	ucom_cnput_complete(struct usbd_xfer *, void *, usbd_status);
static void	ucom_cnput_callout(void *);
static void	ucom_cnput_softintr(void *);
static int	ucom_cngetc(dev_t);
static void	ucom_cnputc(dev_t, int);
static void	ucom_cnpollc(dev_t, int);
static void	ucom_cnfree(struct ucom_softc *);
static void	ucom_cnhalt(dev_t);
#ifdef DDB
static void	ucom_cncheckmagic(struct ucom_softc *, struct ucom_buffer *);
#endif

/*
 * Following are all routines needed for COM to act as console
 */
static struct consdev ucom_cnops = {
	.cn_getc = ucom_cngetc,
	.cn_putc = ucom_cnputc,
	.cn_pollc = ucom_cnpollc,
	.cn_halt = ucom_cnhalt,
	.cn_dev = NODEV,
	.cn_pri = CN_NORMAL
};
static int ucom_cnspeed = B115200;

struct ucom_cnstate {
	dev_t			cn_dev;
	u_char			*cn_buf_in;
	u_int			cn_prod_in;
	u_int			cn_cons_in;

	uint32_t		cn_sending;
	struct usbd_xfer	*cn_xfer_out;
	u_char			*cn_buf_out;
	size_t			cn_buf_out_size;
	u_char			*cn_linear_buf_out;
	u_int			cn_prod_out;
	u_int			cn_cons_out;
	void			*cn_si;
	callout_t		cn_callout;
	struct cnm_state	cn_magic_state;
	bool			cn_magic_req;
	bool			cn_polling;
};

int ucom_match(device_t, cfdata_t, void *);
void ucom_attach(device_t, device_t, void *);
int ucom_detach(device_t, int);

CFATTACH_DECL_NEW(ucom, sizeof(struct ucom_softc), ucom_match, ucom_attach,
    ucom_detach, NULL);

int
ucom_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

/*
 * Output a chunk of data.
 *
 * This is the consumer side of the output ring buffer.
 */
static void
ucom_cnput_complete(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct ucom_softc * const sc = priv;
	struct ucom_cnstate * const cn = sc->sc_cn;
	bool polling = ucom_polling(sc);
	usbd_status err;
	uint32_t cnt;

	KASSERT(ucom_xfer_done(cn->cn_xfer_out));

	usbd_get_xfer_status(cn->cn_xfer_out, NULL, NULL, &cnt, &err);

	u_int cnconsout = atomic_load_relaxed(&cn->cn_cons_out);

	switch (err) {
	case USBD_STALLED:
		if (!polling)
			usbd_clear_endpoint_stall_async(sc->sc_bulkout_pipe);

		/*
		 * The device can't keep up so give it a rest and retry
		 * again in 10 ticks.
		 * Do not move cn->cn_cons_out so that this output is
		 * retried later.
		 * XXXNH Maybe a more scientific retry timer calculation.
		 * When DDB is active, ucom_cnputc() loops on
		 * ucom_cnput_bytes(), this will perform the retry.
		 */
		if (!db_active)
			callout_schedule(&cn->cn_callout, 10);
		return;

	case USBD_NORMAL_COMPLETION:
		cnconsout += cnt;
		atomic_store_release(&cn->cn_cons_out, cnconsout);
		break;

	case USBD_CANCELLED:
		/*
		 * Device close causes pipe abort, and all in-progress
		 * transfers get USBD_CANCELLED. We will retry the
		 * output on next ucom_cnput_bytes() call.
		 */
		break;

	default:
		/*
		 * Catch unusual errors when DIAGNOSTIC, otherwise
		 * retry on next com_cnput_bytes() call.
		 */
#ifdef DIAGNOSTIC
		printf("%s: error %d", __func__, err);
#endif
		break;
	}

	KASSERT(db_active || atomic_load_relaxed(&cn->cn_sending) == 1);

	/*
	 * Under DDB rule, just return, ucom_cnput_bytes() should
	 * poll for completion.
	 */
	if (db_active)
		return;

	(void)ucom_cnput_bytes(sc);
}

static int
ucom_cnput_bytes(struct ucom_softc *sc)
{
	struct ucom_cnstate * const cn = sc->sc_cn;
	u_char *ub_out = usbd_get_buffer(cn->cn_xfer_out);
	u_char *out = NULL;
	usbd_status err;
	u_int prodout = atomic_load_acquire(&cn->cn_prod_out);
	u_int consout = atomic_load_relaxed(&cn->cn_cons_out);
	u_int consout_idx = consout % cn->cn_buf_out_size;
	u_int cnt = prodout - consout;

	if (cnt > sc->sc_obufsize)
		cnt = sc->sc_obufsize;

	KASSERT(db_active || atomic_load_relaxed(&cn->cn_sending) == 1);

	/* Nothing to do so success. */
	if (cnt == 0) {
		if (!db_active)
			atomic_store_relaxed(&cn->cn_sending, 0);
		return 0;
	}

	u_int cntidx = (consout_idx + cnt) % cn->cn_buf_out_size;
	if (cntidx > consout_idx || cntidx == 0) {
		out = cn->cn_buf_out + consout_idx;
	} else {
		/*
		 * Producer wrapped around the ring buffer,
		 * data to send is split in two chenks that
		 * we copy to the linear buffer.
		 * - from consout_idx to end of ring buffer
		 * - from start of ring buffer up to cnt total.
		 */
		u_int top_size = cn->cn_buf_out_size - consout_idx;

		memcpy(cn->cn_linear_buf_out,
		    cn->cn_buf_out + consout_idx, top_size);

		memcpy(cn->cn_linear_buf_out + top_size,
		    cn->cn_buf_out, cntidx);

		out = cn->cn_linear_buf_out;
	}

	if (sc->sc_methods->ucom_write != NULL)
		sc->sc_methods->ucom_write(sc->sc_parent, sc->sc_portno,
		    ub_out, out, &cnt);
	else
		memcpy(ub_out, out, cnt);

	KASSERT(ucom_xfer_done(cn->cn_xfer_out));

	/*
	 * cn->cn_cons_out is increased after output succeeds,
	 * in callback ucom_cnput_complete().
	 */
	usbd_setup_xfer(cn->cn_xfer_out, sc, ub_out, cnt,
	    0, USBD_NO_TIMEOUT, ucom_cnput_complete);
	err = usbd_transfer(cn->cn_xfer_out);

	switch (err) {
	case USBD_NOT_STARTED:
	case USBD_IN_PROGRESS:
		/*
		 * Poll for completion if DDB is active.
		 * Otherwise rely on cn->cn_sending to serialize transfers
		 */
		while (db_active && ucom_polling(sc) &&
		    !ucom_xfer_done(cn->cn_xfer_out))
			usbd_dopoll(sc->sc_iface);
		break;
	case USBD_CANCELLED:
	case USBD_NORMAL_COMPLETION:
		break;
	default:
#ifdef DIAGNOSTIC
		printf("%s: error %d", __func__, err);
#endif
		break;
	}

	return cnt;
}

static void
ucom_cnput_softintr(void *priv)
{
	struct ucom_softc * const sc = priv;
	struct ucom_cnstate * const cn = sc->sc_cn;

	/*
	 * atomic_cas_32() returns the original value,
	 * hence we only proceed if cn->cn_sending was 0.
	 * This makes sure ucom_cnput_bytes() does not
	 * reuse cn->cn_xfer_out before it completes the
	 * transfer.
	 */
	if (atomic_cas_32(&cn->cn_sending, 0, 1) == 0)
		(void)ucom_cnput_bytes(sc);
}

static void
ucom_cnput_callout(void *priv)
{
	struct ucom_softc * const sc = priv;
	struct ucom_cnstate * const cn = sc->sc_cn;

	kpreempt_disable();
	softint_schedule(cn->cn_si);
	kpreempt_enable();
}

static bool
ucom_polling(struct ucom_softc *sc)
{
	struct ucom_cnstate * const cn = sc->sc_cn;

	if (cn == NULL)
		return false;

	return cn->cn_polling;
}

static void
ucom_mutex_enter(struct ucom_softc *sc)
{
	if (!ucom_polling(sc))
		mutex_enter(&sc->sc_lock);
}

static void
ucom_mutex_exit(struct ucom_softc *sc)
{
	if (!ucom_polling(sc))
		mutex_exit(&sc->sc_lock);
}


static void
ucom_cninit(struct ucom_softc *sc)
{
	int major = cdevsw_lookup_major(&ucom_cdevsw);
	int unit = device_unit(sc->sc_dev);
	struct tty *tp = sc->sc_tty;
	struct termios t;
	struct ucom_buffer *ub;
	int error = -1;

	if (makedev(major, unit) != ucom_cnops.cn_dev)
		return;

	/*
	 * TIOCFLAG_SOFTCAR sets CLOCAL in ucomopen().
	 */
	SET(sc->sc_swflags, TIOCFLAG_SOFTCAR);

	memset(&t, 0, sizeof(t));
	t.c_ispeed = ucom_cnspeed;
	t.c_ospeed = ucom_cnspeed;
	error = ucomparam(tp, &t);
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "ucomparam fail, error = %d", error);

	sc->sc_cn = kmem_zalloc(sizeof(*sc->sc_cn), KM_SLEEP);
	struct ucom_cnstate * const cn = sc->sc_cn;

	cn->cn_dev = ucom_cnops.cn_dev;

	cn->cn_si = softint_establish(SOFTINT_USB, ucom_cnput_softintr, sc);

	callout_init(&cn->cn_callout, CALLOUT_MPSAFE);
	callout_setfunc(&cn->cn_callout, ucom_cnput_callout, sc);

	cn->cn_buf_in = kmem_alloc(sc->sc_ibufsize, KM_SLEEP);
	cn->cn_prod_in = 0;
	cn->cn_cons_in = 0;

	/*
	 * Kernel can easily write to the output buffer faster
	 * that the serial interface can consume. We need a
	 * buffer large enough to avoid overflown abd
	 * sc->sc_obufsize, may be too small for that.
	 */
	cn->cn_buf_out_size = MAX(sc->sc_obufsize, PAGE_SIZE);
	cn->cn_buf_out = kmem_alloc(cn->cn_buf_out_size, KM_SLEEP);
	cn->cn_prod_out = 0;
	cn->cn_cons_out = 0;
	if (usbd_create_xfer(sc->sc_bulkout_pipe,
	    sc->sc_obufsize, 0, 0, &cn->cn_xfer_out) != 0) {
		aprint_error_dev(sc->sc_dev, "fail to create cn_xfer_out");
		goto ucom_cnfail;
	}

	cn->cn_linear_buf_out = kmem_alloc(sc->sc_obufsize, KM_SLEEP);

	cn_tab = &ucom_cnops;
	cn_init_magic(&cn->cn_magic_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */

	aprint_normal_dev(sc->sc_dev, "console\n");

	mutex_enter(&sc->sc_lock);
	for (size_t i = 0; i < UCOM_IN_BUFFS; i++) {
		ub = &sc->sc_ibuff[i];
		error = ucomsubmitread(sc, ub);
		if (error) {
			mutex_exit(&sc->sc_lock);
			goto ucom_cnfail;
		}
	}
	mutex_exit(&sc->sc_lock);

	return;

ucom_cnfail:
	if (error)
		ucom_cnfree(sc);
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
	sc->sc_closing = false;
	sc->sc_dying = false;
	sc->sc_cn = NULL;
	sc->sc_state = UCOM_DEAD;

	sc->sc_si = softint_establish(SOFTINT_USB, ucom_softintr, sc);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	cv_init(&sc->sc_statecv, "ucomstate");

	rnd_attach_source(&sc->sc_rndsource, device_xname(sc->sc_dev),
	    RND_TYPE_TTY, RND_FLAG_DEFAULT);

	SIMPLEQ_INIT(&sc->sc_ibuff_empty);
	SIMPLEQ_INIT(&sc->sc_ibuff_full);
	SIMPLEQ_INIT(&sc->sc_obuff_free);
	SIMPLEQ_INIT(&sc->sc_obuff_full);

	memset(sc->sc_ibuff, 0, sizeof(sc->sc_ibuff));
	memset(sc->sc_obuff, 0, sizeof(sc->sc_obuff));

	DPRINTF("open pipes in=%jd out=%jd",
	    sc->sc_bulkin_no, sc->sc_bulkout_no, 0, 0);

	struct ucom_buffer *ub;
	usbd_status err;
	int error;

	/* Open the bulk pipes */
	err = usbd_open_pipe(sc->sc_iface, sc->sc_bulkin_no,
	    USBD_EXCLUSIVE_USE, &sc->sc_bulkin_pipe);
	if (err) {
		DPRINTF("open bulk in error (addr %jd), err=%jd",
		    sc->sc_bulkin_no, err, 0, 0);
		error = EIO;
		goto fail_0;
	}
	/* Allocate input buffers */
	for (ub = &sc->sc_ibuff[0]; ub != &sc->sc_ibuff[UCOM_IN_BUFFS];
	    ub++) {
		error = usbd_create_xfer(sc->sc_bulkin_pipe,
		    sc->sc_ibufsizepad, 0, 0,
		    &ub->ub_xfer);
		if (error)
			goto fail_1;
		ub->ub_data = usbd_get_buffer(ub->ub_xfer);
	}

	err = usbd_open_pipe(sc->sc_iface, sc->sc_bulkout_no,
	    USBD_EXCLUSIVE_USE, &sc->sc_bulkout_pipe);
	if (err) {
		DPRINTF("open bulk out error (addr %jd), err=%jd",
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

	struct tty *tp = tty_alloc();
	tp->t_oproc = ucomstart;
	tp->t_param = ucomparam;
	tp->t_hwiflow = ucomhwiflow;
	tp->t_softc = sc;

	sc->sc_tty = tp;

	DPRINTF("tty_attach %#jx", (uintptr_t)tp, 0, 0, 0);
	tty_attach(tp);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_state = UCOM_ATTACHED;

	/*
	 * Attach ucom console
	 */
	ucom_cninit(sc);

	return;

fail_2:
	for (ub = &sc->sc_obuff[0]; ub != &sc->sc_obuff[UCOM_OUT_BUFFS];
	    ub++) {
		if (ub->ub_xfer)
			usbd_destroy_xfer(ub->ub_xfer);
	}
	usbd_close_pipe(sc->sc_bulkout_pipe);
	sc->sc_bulkout_pipe = NULL;

fail_1:
	for (ub = &sc->sc_ibuff[0]; ub != &sc->sc_ibuff[UCOM_IN_BUFFS];
	    ub++) {
		if (ub->ub_xfer)
			usbd_destroy_xfer(ub->ub_xfer);
	}
	usbd_close_pipe(sc->sc_bulkin_pipe);
	sc->sc_bulkin_pipe = NULL;

fail_0:
	aprint_error_dev(self, "attach failed, error=%d\n", error);
}

int
ucom_cnattach(int unit, int speed)
{
	int major = cdevsw_lookup_major(&ucom_cdevsw);

	/*
	 * This is called from consinit(), but at that time the
	 * usb devices are not yet attached, hence we must defer
	 * ucom console attachement until ucom_attach(). We set
	 * ucomcn.cn_dev to signal ucom_attach() which ucom
	 * unit should attach the console.
	 */
	ucom_cnops.cn_dev = makedev(major, unit);
	ucom_cnspeed = speed;

	return 0;
}


int
ucom_detach(device_t self, int flags)
{
	struct ucom_softc *sc = device_private(self);
	struct tty *tp = sc->sc_tty;
	int maj, mn;
	int i;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("sc=%#jx flags=%jd tp=%#jx", (uintptr_t)sc, flags,
	    (uintptr_t)tp, 0);
	DPRINTF("... pipe=%jd,%jd", sc->sc_bulkin_no, sc->sc_bulkout_no, 0, 0);

	/* Prevent new opens from hanging.  */
	mutex_enter(&sc->sc_lock);
	sc->sc_dying = true;
	mutex_exit(&sc->sc_lock);

	/*
	 * If console is attached, we still have input pipe used
	 * after device is closed, so that we can catch cnmagic
	 * and run DDB. It is time to abort the pipe now.
	 */
	if (sc->sc_cn)
		usbd_abort_pipe(sc->sc_bulkin_pipe);

	pmf_device_deregister(self);

	/* tty is now off.  */
	if (tp != NULL) {
		ttylock(tp);
		CLR(tp->t_state, TS_CARR_ON);
		CLR(tp->t_cflag, CLOCAL | MDMBUF);
		ttyunlock(tp);
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&ucom_cdevsw);

	/* Nuke the vnodes for any open instances. */
	mn = device_unit(self);
	DPRINTF("maj=%jd mn=%jd", maj, mn, 0, 0);
	vdevgone(maj, mn, mn, VCHR);
	vdevgone(maj, mn | UCOMDIALOUT_MASK, mn | UCOMDIALOUT_MASK, VCHR);
	vdevgone(maj, mn | UCOMCALLUNIT_MASK, mn | UCOMCALLUNIT_MASK, VCHR);

	softint_disestablish(sc->sc_si);

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

	/*
	 * Detach console after pipes are closed, so that
	 * cn->cn_xfer_out is properly aborted.
	 */
	if (sc->sc_cn) {
		ucom_cnhalt(sc->sc_cn->cn_dev);
		sc->sc_cn = NULL;
	}

	/* Detach the random source */
	rnd_detach_source(&sc->sc_rndsource);

	mutex_destroy(&sc->sc_lock);
	cv_destroy(&sc->sc_statecv);

	return 0;
}

void
ucom_shutdown(struct ucom_softc *sc)
{
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		ucom_dtr(sc, 0);
		mutex_enter(&sc->sc_lock);
		microuptime(&sc->sc_hup_time);
		sc->sc_hup_time.tv_sec++;
		mutex_exit(&sc->sc_lock);
	}
}

/*
 * ucomopen(dev, flag, mode, l)
 *
 *	Called when anyone tries to open /dev/ttyU? for an existing
 *	ucom? instance that has completed attach.  The attach may have
 *	failed, though, or there may be concurrent detach or close in
 *	progress, so fail if attach failed (no sc_tty) or detach has
 *	begun (sc_dying), or wait if there's a concurrent close in
 *	progress before reopening.
 */
int
ucomopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	int error = 0;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	mutex_enter(&sc->sc_lock);
	if (sc->sc_dying) {
		DPRINTF("... dying", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);
		return ENXIO;
	}

	if (!device_is_active(sc->sc_dev)) {
		mutex_exit(&sc->sc_lock);
		return ENXIO;
	}

	struct tty *tp = sc->sc_tty;
	if (tp == NULL) {
		DPRINTF("... not attached", 0, 0, 0, 0);
		mutex_exit(&sc->sc_lock);
		return ENXIO;
	}

	DPRINTF("unit=%jd, tp=%#jx", unit, (uintptr_t)tp, 0, 0);

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)) {
		mutex_exit(&sc->sc_lock);
		return EBUSY;
	}

	/*
	 * If the previous use had set DTR on close, wait up to one
	 * second for the other side to notice we hung up.  After
	 * sleeping, the tty may have been revoked, so restart the
	 * whole operation.
	 *
	 * XXX The wchan is not ttclose but maybe should be.
	 */
	if (timerisset(&sc->sc_hup_time)) {
		struct timeval now, delta;
		int ms, ticks;

		microuptime(&now);
		if (timercmp(&now, &sc->sc_hup_time, <)) {
			timersub(&sc->sc_hup_time, &now, &delta);
			ms = MIN(INT_MAX - 1000, delta.tv_sec*1000);
			ms += howmany(delta.tv_usec, 1000);
			ticks = MAX(1, MIN(INT_MAX, mstohz(ms)));
			(void)cv_timedwait(&sc->sc_statecv, &sc->sc_lock,
			    ticks);
			mutex_exit(&sc->sc_lock);
			return ERESTART;
		}
		timerclear(&sc->sc_hup_time);
	}

	/*
	 * Wait while the device is initialized by the
	 * first opener or cleaned up by the last closer.
	 */
	enum ucom_state state = sc->sc_state;
	if (state == UCOM_OPENING || sc->sc_closing) {
		if (flag & FNONBLOCK)
			error = EWOULDBLOCK;
		else
			error = cv_wait_sig(&sc->sc_statecv, &sc->sc_lock);
		mutex_exit(&sc->sc_lock);
		return error ? error : ERESTART;
	}
	KASSERTMSG(state == UCOM_OPEN || state == UCOM_ATTACHED,
	    "state is %d", state);

	/*
	 * If this is the first open, then make sure the pipes are
	 * running and perform any initialization needed.
	 */
	bool firstopen = (state == UCOM_ATTACHED);
	if (firstopen) {
		KASSERT(!ISSET(tp->t_state, TS_ISOPEN));
		KASSERT(tp->t_wopen == 0);

		tp->t_dev = dev;
		sc->sc_state = UCOM_OPENING;
		mutex_exit(&sc->sc_lock);

		if (sc->sc_methods->ucom_open != NULL) {
			error = sc->sc_methods->ucom_open(sc->sc_parent,
			    sc->sc_portno);
			if (error)
				goto bad;
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
		struct termios t;

		if (cn_isconsole(dev)) {
			t.c_ispeed = ucom_cnspeed;
			t.c_ospeed = ucom_cnspeed;
		} else {
			t.c_ispeed = 0;
			t.c_ospeed = TTYDEF_SPEED;
		}
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

		mutex_enter(&sc->sc_lock);
		sc->sc_rx_unblock = 0;
		sc->sc_rx_stopped = 0;
		sc->sc_tx_stopped = 0;

		/*
		 * If device is console, input xfers are scheduled
		 * from attach to detach time. Otherwise from open
		 * here to close.
		 */
		if (!cn_isconsole(dev)) {
			/*
			 * Either after ucom was attached, or after
			 * ucomclose(), this list must be empty,
			 * otherwise ucomsubmitread() will corrupt
			 * it by queueing an ucom_buffer that is already
			 *  queued.
			 */
			KASSERT(SIMPLEQ_EMPTY(&sc->sc_ibuff_empty));

			for (size_t i = 0; i < UCOM_IN_BUFFS; i++) {
				struct ucom_buffer *ub = &sc->sc_ibuff[i];
				error = ucomsubmitread(sc, ub);
				if (error) {
					mutex_exit(&sc->sc_lock);
					goto bad;
				}
			}
		}
	}
	mutex_exit(&sc->sc_lock);

	DPRINTF("unit=%jd, tp=%#jx dialout %jd nonblock %jd", unit,
	    (uintptr_t)tp, !!UCOMDIALOUT(dev), !!ISSET(flag, O_NONBLOCK));
	error = ttyopen(tp, UCOMDIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	/*
	 * Success!  If this was the first open, notify waiters that
	 * the tty is open for business.
	 */
	if (firstopen) {
		mutex_enter(&sc->sc_lock);
		KASSERT(sc->sc_state == UCOM_OPENING);
		sc->sc_state = UCOM_OPEN;
		cv_broadcast(&sc->sc_statecv);
		mutex_exit(&sc->sc_lock);
	}
	return 0;

bad:
	/*
	 * Failure!  If this was the first open, hang up, abort pipes,
	 * and notify waiters that we're not opening after all.
	 */
	if (firstopen) {
		ucom_cleanup(sc, flag);

		mutex_enter(&sc->sc_lock);
		KASSERT(sc->sc_state == UCOM_OPENING);
		sc->sc_state = UCOM_ATTACHED;
		cv_broadcast(&sc->sc_statecv);
		mutex_exit(&sc->sc_lock);
	}
	return error;
}

/*
 * ucomcancel(dev, flag, mode, l)
 *
 *	Called on revoke or last close.  Must interrupt any pending I/O
 *	operations and make them fail promptly; once they have all
 *	finished (except possibly new opens), ucomclose will be called.
 *	We set sc_closing to block new opens until ucomclose runs.
 */
int
ucomcancel(dev_t dev, int flag, int mode, struct lwp *l)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, unit);

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("unit=%jd", UCOMUNIT(dev), 0, 0, 0);

	/*
	 * This can run at any time before ucomclose on any device
	 * node, even if never attached or if attach failed, so we may
	 * not have a softc or a tty.
	 */
	if (sc == NULL)
		return 0;
	struct tty *tp = sc->sc_tty;
	if (tp == NULL)
		return 0;

	/*
	 * Mark the device closing so opens block until we're done
	 * closing.  Wake them up so they start over at the top -- if
	 * we're closing because we're detaching, they need to wake up
	 * and notice it's time to fail.
	 */
	mutex_enter(&sc->sc_lock);
	sc->sc_closing = true;
	cv_broadcast(&sc->sc_statecv);
	mutex_exit(&sc->sc_lock);

	/*
	 * Cancel any pending tty I/O operations, causing them to wake
	 * up and fail promptly, and preventing any new ones from
	 * starting to wait until we have finished closing.
	 */
	ttycancel(tp);

	return 0;
}

/*
 * ucomclose(dev, flag, mode, l)
 *
 *	Called after ucomcancel, when all prior operations on the /dev
 *	node have completed.  Only new opens may be in progress at this
 *	point, but they will block until sc_closing is set to false.
 */
int
ucomclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc *sc = device_lookup_private(&ucom_cd, unit);
	int error = 0;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("unit=%jd", UCOMUNIT(dev), 0, 0, 0);

	/*
	 * This can run at any time after ucomcancel on any device
	 * node, even if never attached or if attach failed, so we may
	 * not have a softc or a tty.
	 */
	if (sc == NULL)
		return 0;
	struct tty *tp = sc->sc_tty;
	if (tp == NULL)
		return 0;

	/*
	 * Close the tty, causing anyone waiting for it to wake, and
	 * hang up the phone.
	 */
	ucom_cleanup(sc, flag);

	/*
	 * ttyclose should have cleared TS_ISOPEN and interrupted all
	 * pending opens, which should have completed by now.
	 */
	ttylock(tp);
	KASSERT(!ISSET(tp->t_state, TS_ISOPEN));
	KASSERT(tp->t_wopen == 0);
	ttyunlock(tp);

	/*
	 * Close any device-specific state.
	 */
	if (sc->sc_methods->ucom_close != NULL) {
		sc->sc_methods->ucom_close(sc->sc_parent,
		    sc->sc_portno);
	}

	/*
	 * We're now closed.  Can reopen after this point, so resume
	 * transfers, mark us no longer closing, and notify anyone
	 * waiting in open.  The state may be OPEN or ATTACHED at this
	 * point -- OPEN if the device was already open when we closed
	 * it, ATTACHED if we interrupted it in the process of opening.
	 */
	mutex_enter(&sc->sc_lock);
	KASSERTMSG(sc->sc_state == UCOM_ATTACHED || sc->sc_state == UCOM_OPEN,
	    "%s sc=%p state=%d", device_xname(sc->sc_dev), sc, sc->sc_state);
	KASSERT(sc->sc_closing);
	sc->sc_state = UCOM_ATTACHED;
	sc->sc_closing = false;
	cv_broadcast(&sc->sc_statecv);
	mutex_exit(&sc->sc_lock);

	return error;
}

int
ucomread(dev_t dev, struct uio *uio, int flag)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
ucomwrite(dev_t dev, struct uio *uio, int flag)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

int
ucompoll(dev_t dev, int events, struct lwp *l)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	return (*tp->t_linesw->l_poll)(tp, events, l);
}

struct tty *
ucomtty(dev_t dev)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);

	return sc->sc_tty;
}

int
ucomioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct tty *tp = sc->sc_tty;
	int error;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("cmd=0x%08jx", cmd, 0, 0, 0);

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

	DPRINTF("our cmd=0x%08jx", cmd, 0, 0, 0);

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

		int swflags = *(int *)data;
		/* Force local if device is console */
		if (cn_isconsole(dev))
			swflags |= TIOCFLAG_SOFTCAR;

		mutex_enter(&sc->sc_lock);
		sc->sc_swflags = swflags;
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
	u_char mcr = sc->sc_mcr;
	mutex_exit(&sc->sc_lock);

	if (how == TIOCMSET || ISSET(combits, UMCR_DTR))
		ucom_dtr(sc, (mcr & UMCR_DTR) != 0);
	if (how == TIOCMSET || ISSET(combits, UMCR_RTS))
		ucom_rts(sc, (mcr & UMCR_RTS) != 0);
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

	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_BREAK, onoff);
}

static void
ucom_dtr(struct ucom_softc *sc, int onoff)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_DTR, onoff);
}

static void
ucom_rts(struct ucom_softc *sc, int onoff)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("onoff=%jd", onoff, 0, 0, 0);

	if (sc->sc_methods->ucom_set != NULL)
		sc->sc_methods->ucom_set(sc->sc_parent, sc->sc_portno,
		    UCOM_SET_RTS, onoff);
}

void
ucom_status_change(struct ucom_softc *sc)
{
	/* We get here from sc->sc_methods->ucom_read */
	if (ucom_polling(sc))
		return;

	if (sc->sc_methods->ucom_get_status != NULL) {
		struct tty *tp = sc->sc_tty;
		u_char msr, lsr;

		sc->sc_methods->ucom_get_status(sc->sc_parent, sc->sc_portno,
		    &lsr, &msr);
		mutex_enter(&sc->sc_lock);
		u_char old_msr = sc->sc_msr;

		sc->sc_lsr = lsr;
		sc->sc_msr = msr;
		mutex_exit(&sc->sc_lock);

		if (ISSET((msr ^ old_msr), UMSR_DCD)) {
			mutex_spin_enter(&timecounter_lock);
			pps_capture(&sc->sc_pps_state);
			pps_event(&sc->sc_pps_state,
			    (sc->sc_msr & UMSR_DCD) ?
			    PPS_CAPTUREASSERT :
			    PPS_CAPTURECLEAR);
			mutex_spin_exit(&timecounter_lock);

			(*tp->t_linesw->l_modem)(tp, ISSET(msr, UMSR_DCD));
		}
	} else {
		mutex_enter(&sc->sc_lock);
		sc->sc_lsr = 0;
		/* Assume DCD is present, if we have no chance to check it. */
		sc->sc_msr = UMSR_DCD;
		mutex_exit(&sc->sc_lock);
	}
}

static int
ucomparam(struct tty *tp, struct termios *t)
{
	struct ucom_softc * const sc = tp->t_softc;
	int error = 0;

	/* XXX should take tty lock around touching tp */

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	/* Check requested parameters. */
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed) {
		error = EINVAL;
		goto out;
	}

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
	    tp->t_cflag == t->c_cflag) {
		goto out;
	}

	/* XXX lcr = ISSET(sc->sc_lcr, LCR_SBREAK) | cflag2lcr(t->c_cflag); */

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (sc->sc_methods->ucom_param != NULL) {
		error = sc->sc_methods->ucom_param(sc->sc_parent, sc->sc_portno,
		    t);
		if (error)
			goto out;
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
out:
	return error;
}

static int
ucomhwiflow(struct tty *tp, int block)
{
	struct ucom_softc * const sc = tp->t_softc;
	int old;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	KASSERT(&sc->sc_lock);
	KASSERT(ttylocked(tp));

	old = sc->sc_rx_stopped;
	sc->sc_rx_stopped = (u_char)block;

	if (old && !block) {
		sc->sc_rx_unblock = 1;
		kpreempt_disable();
		softint_schedule(sc->sc_si);
		kpreempt_enable();
	}

	return 1;
}

static void
ucomstart(struct tty *tp)
{
	struct ucom_softc * const sc = tp->t_softc;
	struct ucom_buffer *ub;
	u_char *data;
	int cnt;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

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

	kpreempt_disable();
	softint_schedule(sc->sc_si);
	kpreempt_enable();

 out:
	DPRINTF("... done", 0, 0, 0, 0);
}

void
ucomstop(struct tty *tp, int flag)
{
#if 0
	struct ucom_softc * const sc = tp->t_softc;

	mutex_enter(&sc->sc_lock);
	ttylock(tp);
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* obuff_full -> obuff_free? */
		/* sc->sc_tx_stopped = 1; */
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	ttyunlock(tp);
	mutex_exit(&sc->sc_lock);
#endif
}

static void
ucom_write_status(struct ucom_softc *sc, struct ucom_buffer *ub,
    usbd_status err)
{
	struct tty *tp = sc->sc_tty;
	uint32_t cc = ub->ub_len;
	bool polling = ucom_polling(sc);

	KASSERT(polling || mutex_owned(&sc->sc_lock));

	switch (err) {
	case USBD_IN_PROGRESS:
		ub->ub_index = ub->ub_len;
		break;
	case USBD_STALLED:
		ub->ub_index = 0;
		kpreempt_disable();
		softint_schedule(sc->sc_si);
		kpreempt_enable();
		break;
	case USBD_NORMAL_COMPLETION:
		usbd_get_xfer_status(ub->ub_xfer, NULL, NULL, &cc, NULL);
		rnd_add_uint32(&sc->sc_rndsource, cc);
		/*FALLTHROUGH*/
	default:
		SIMPLEQ_REMOVE_HEAD(&sc->sc_obuff_full, ub_link);
		SIMPLEQ_INSERT_TAIL(&sc->sc_obuff_free, ub, ub_link);
		cc -= sc->sc_opkthdrlen;

		ttylock(tp);
		CLR(tp->t_state, TS_BUSY);
		if (ISSET(tp->t_state, TS_FLUSH))
			CLR(tp->t_state, TS_FLUSH);
		else
			ndflush(&tp->t_outq, cc);
		ttyunlock(tp);

		if (err != USBD_CANCELLED && err != USBD_IOERROR &&
		    !sc->sc_closing) {
			if ((ub = SIMPLEQ_FIRST(&sc->sc_obuff_full)) != NULL)
				ucom_submit_write(sc, ub);

			ttylock(tp);
			(*tp->t_linesw->l_start)(tp);
			ttyunlock(tp);
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

	ucom_mutex_enter(sc);
	ucom_write_status(sc, SIMPLEQ_FIRST(&sc->sc_obuff_full), status);
	ucom_mutex_exit(sc);

}

static void
ucom_softintr(void *arg)
{
	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	struct ucom_softc *sc = arg;
	struct tty *tp = sc->sc_tty;
	struct ucom_buffer *ub;

	if (ucom_polling(sc))
		return;

	mutex_enter(&sc->sc_lock);

#ifdef DDB
	struct ucom_cnstate *cn = sc->sc_cn;

	if (cn && cn->cn_magic_req) {
		if (!SIMPLEQ_EMPTY(&sc->sc_ibuff_empty)) {
			cn->cn_magic_req = false;
			cn_trap();
		} else {
			/*
			 * Very unlikely case where ucom_softintr()
			 * raced with ucomreadcb(). Just retry later.
			 */
			kpreempt_disable();
			softint_schedule(sc->sc_si);
			kpreempt_enable();
		}
	}
#endif

	ttylock(tp);
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		ttyunlock(tp);
		mutex_exit(&sc->sc_lock);
		return;
	}
	ttyunlock(tp);

	ub = SIMPLEQ_FIRST(&sc->sc_obuff_full);

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

		/* XXX ttyinput takes ttylock */
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

static int
ucomsubmitread(struct ucom_softc *sc, struct ucom_buffer *ub)
{
	usbd_status err;

	KASSERT(ucom_polling(sc) || mutex_owned(&sc->sc_lock));

	usbd_setup_xfer(ub->ub_xfer, sc, ub->ub_data, sc->sc_ibufsize,
	    USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT, ucomreadcb);

	if ((err = usbd_transfer(ub->ub_xfer)) != USBD_IN_PROGRESS) {
		/* XXX: Recover from this, please! */
		device_printf(sc->sc_dev, "%s: err=%s\n",
		    __func__, usbd_errstr(err));
		return EIO;
	}

	SIMPLEQ_INSERT_TAIL(&sc->sc_ibuff_empty, ub, ub_link);

	return 0;
}

static void
ucom_cnread_complete(struct ucom_softc *sc)
{
	struct ucom_buffer *ub;
	struct ucom_cnstate * const cn = sc->sc_cn;
	u_int prodin = cn->cn_prod_in;
	u_int consin = cn->cn_cons_in;
	u_int prodin_idx = prodin % sc->sc_ibufsize;
	u_int consin_idx = consin % sc->sc_ibufsize;
	uint32_t cc, cnt;
	u_char *cp;

	ub = SIMPLEQ_FIRST(&sc->sc_ibuff_full);

	while (ub != NULL) {
		KASSERT(ucom_xfer_done(ub->ub_xfer));

		cp = ub->ub_data + ub->ub_index;
		cc = ub->ub_len - ub->ub_index;

		if (cc == 0)
			goto next;

		if (consin_idx > prodin_idx) {
			if (cc > consin_idx - prodin_idx)
				cnt = consin_idx - prodin_idx;
			else
				cnt = cc;

			KASSERT(prodin_idx + cnt <= sc->sc_ibufsize);

			memcpy(cn->cn_buf_in + prodin_idx, cp, cnt);
			cn->cn_prod_in += cnt;
			cp += cnt;
			cc -= cnt;
		} else { /* consin_idx <= prodin_idx */
			/*
			 * First chunk, from prodin_idx to end of buffer
			 */
			if (cc > sc->sc_ibufsize - prodin_idx)
				cnt = sc->sc_ibufsize - prodin_idx;
			else
				cnt = cc;

			KASSERT(prodin_idx + cnt <= sc->sc_ibufsize);

			memcpy(cn->cn_buf_in + prodin_idx, cp, cnt);
			cn->cn_prod_in += cnt;
			cp += cnt;
			cc -= cnt;

			if (cc == 0)
				goto next;

			/*
			 * Second chunk, from start of buffer to cosin_idx
			 */
			if (cc > consin_idx)
				cnt = consin_idx;
			else
				cnt = cc;

			KASSERT(cn->cn_prod_in + cnt <= sc->sc_ibufsize);

			memcpy(cn->cn_buf_in + prodin_idx, cp, cnt);
			cn->cn_prod_in += cnt;
			cp += cnt;
			cc -= cnt;
		}
next:
		/* If cc > 0 we discard some input */

		SIMPLEQ_REMOVE_HEAD(&sc->sc_ibuff_full, ub_link);
		ucomsubmitread(sc, ub);
		ub = SIMPLEQ_FIRST(&sc->sc_ibuff_full);
	}
}

static void
ucomreadcb(struct usbd_xfer *xfer, void *p, usbd_status status)
{
	struct ucom_softc *sc = (struct ucom_softc *)p;
	struct ucom_buffer *ub;
	uint32_t cc;
	u_char *cp;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	ucom_mutex_enter(sc);

	/*
	 * Always remove the ucom_buffer from the sc_ibuff_empty list
	 * regardless of transfer status. Leaving it on the list on
	 * USBD_CANCELLED or USBD_IOERROR would cause corruption
	 * next time the device is opened.
	 *
	 * USBD_CANCELLED is a reported when the device is closed
	 * via ucomclose() -> usbd_abort_pipe() -> usbd_xfer_abort()
	 */
	ub = SIMPLEQ_FIRST(&sc->sc_ibuff_empty);
	SIMPLEQ_REMOVE_HEAD(&sc->sc_ibuff_empty, ub_link);
	KASSERT(xfer == ub->ub_xfer);

	if (status == USBD_CANCELLED || status == USBD_IOERROR ||
	    sc->sc_closing) {
		DPRINTF("... done (status %jd closing %jd)",
		    status, sc->sc_closing, 0, 0);

		/*
		 * If console is attached, keep reading after
		 * close so that we can catch cnmagic.
		 */
		if (sc->sc_cn)
			ucomsubmitread(sc, ub);

		ucom_mutex_exit(sc);

		return;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_STALLED) {
			usbd_clear_endpoint_stall_async(sc->sc_bulkin_pipe);
		} else {
			device_printf(sc->sc_dev,
			    "ucomreadcb: wonky status=%s\n",
			    usbd_errstr(status));
		}

		DPRINTF("... done (status %jd)", status, 0, 0, 0);
		/* re-adds ub to sc_ibuff_empty */
		ucomsubmitread(sc, ub);
		ucom_mutex_exit(sc);
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

	ucom_mutex_exit(sc);
	if (sc->sc_methods->ucom_read != NULL) {
		sc->sc_methods->ucom_read(sc->sc_parent, sc->sc_portno,
		    &cp, &cc);
		ub->ub_index = (u_int)(cp - ub->ub_data);
	} else
		ub->ub_index = 0;

	ub->ub_len = cc;

	rnd_add_uint32(&sc->sc_rndsource, cc);

#ifdef DDB
	ucom_cncheckmagic(sc, ub);
#endif
	ucom_mutex_enter(sc);
	/*
	 * Quickly discard input if device is not open,
	 * except when console is polling.
	 */
	if (sc->sc_state != UCOM_OPEN && !ucom_polling(sc)) {
		/* Go around again - we're not quite ready */
		/* re-adds ub to sc_ibuff_empty */
		ucomsubmitread(sc, ub);
		ucom_mutex_exit(sc);
		DPRINTF("... done (not open)", 0, 0, 0, 0);
		return;
	}

	SIMPLEQ_INSERT_TAIL(&sc->sc_ibuff_full, ub, ub_link);

	if (ucom_polling(sc))
		ucom_cnread_complete(sc);
	else
		ucom_read_complete(sc);

	ucom_mutex_exit(sc);

	DPRINTF("... done", 0, 0, 0, 0);
}

static void
ucom_cleanup(struct ucom_softc *sc, int flag)
{
	struct tty *tp = sc->sc_tty;

	UCOMHIST_FUNC(); UCOMHIST_CALLED();

	DPRINTF("closing pipes", 0, 0, 0, 0);

	/*
	 * Close the tty and interrupt any pending opens waiting for
	 * carrier so they restart or give up.  This may flush data.
	 */
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	/*
	 * Interrupt any pending xfers and cause them to fail promptly.
	 * New xfers will only be submitted under the lock after
	 * sc_closing is cleared.
	 * If console is attached, we keep the input pipe running
	 * so that we can catch cnmagic and run DDB.
	 */
	if (sc->sc_cn == NULL)
		usbd_abort_pipe(sc->sc_bulkin_pipe);
	usbd_abort_pipe(sc->sc_bulkout_pipe);

	/*
	 * Hang up the phone and start the timer before we can make a
	 * call again, if necessary.
	 */
	ucom_shutdown(sc);
}

#endif /* NUCOM > 0 */

static bool
ucom_xfer_done(struct usbd_xfer *xfer)
{
	usbd_status err;

	usbd_get_xfer_status(xfer, NULL, NULL, NULL, &err);

	return (err != USBD_NOT_STARTED && err != USBD_IN_PROGRESS);
}

/*
 * Kernel console input, used when no user process is involved:
 * used by userconf prompt (boot -c), root device prompt (boot -a)
 * or DDB. This code path is not used in single user mode or by getty.
 *
 * ucom console specificities: userconf prompt (boot -c) cannot work
 * as USB devices are not yet attached.
 */
static int
ucom_cngetc(dev_t dev)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct ucom_cnstate * const cn = sc->sc_cn;
	int c = -1; /* If -1 we are called again in a loop at splhigh */

	if (cn->cn_cons_in == cn->cn_prod_in) {
		struct ucom_buffer *ub;

		if ((ub = SIMPLEQ_FIRST(&sc->sc_ibuff_empty)) != NULL) {
			if (!ucom_xfer_done(ub->ub_xfer))
				usbd_dopoll(sc->sc_iface);
		} else if ((ub = SIMPLEQ_FIRST(&sc->sc_ibuff_full)) != NULL) {
			/*
			 * This may happen if we entered DDB while
			 * executing ucomreadcb(). We can start an
			 * input transfer for DDB, but if we get there
			 * because of a breakpoint, we are not likely
			 * survive exitting DDB.
			 */
			SIMPLEQ_REMOVE_HEAD(&sc->sc_ibuff_full, ub_link);
			ucomsubmitread(sc, ub);
		} else {
			panic("no ibuff anywhere");
		}
	}

	if (cn->cn_cons_in != cn->cn_prod_in) {
		c = cn->cn_buf_in[cn->cn_cons_in % sc->sc_ibufsize];
		cn->cn_cons_in++;
	}

	return c;
}

/*
 * Console kernel output, used when no userland process is
 * involved. This is the "green" output on wscons default setup.
 * This code path is not used in single user mode, during
 * userland boot sequence, or by getty.
 *
 * When ucom_cnputc() is called from kprintf(), kprintf_lock is held. Take
 * advantage of this to serialiase the "producer" side of the ucom console
 * ring buffer.
 *
 * Add to the ring buffer if there is space and trigger a soft interrupt
 * handler at IPL_SOFTUSB to initiate USB transfers with the device.
 */
static void
ucom_cnputc(dev_t dev, int out)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct ucom_cnstate * const cn = sc->sc_cn;

	u_int cnprodout = cn->cn_prod_out;
	u_int cnconsout = atomic_load_relaxed(&cn->cn_cons_out);
	size_t bufsz = cn->cn_buf_out_size;

	/*
	 * Check for cn->cn_buf_out rini buffer overflow.
	 * New character is discared in that case.
	 */
	if (cnprodout - cnconsout < bufsz) {
		cn->cn_buf_out[cnprodout % bufsz] = (u_char)(out & 0xff);

		/*
		 * ensure the output character write completes before
		 * incrementing cn->cn_prod_out
		 */
		atomic_store_release(&cn->cn_prod_out, cnprodout + 1);
	}

	if (db_active) {
		int res;

		/*
		 * When DDB takes over, output is done synchronously
		 * by ucom_cnput_bytes().
		 */
		do {
			res = ucom_cnput_bytes(sc);
		} while (res != 0);
	} else {
		/*
		 * ucom_cnput_bytes() call is deferred to soft
		 * interrupt context in case current context does not
		 * allow USB transfer to proceed.
		 *
		 * This must be done even if we detected overflow, because
		 * in that case we still need the output to be flushed.
		 */
		kpreempt_disable();
		softint_schedule(cn->cn_si);
		kpreempt_enable();
	}
}

/*
 * Toggle USB polling, where USB interrupt handler (hard and soft)
 * are called from ucom_cngetc() through usbd_dopoll().
 * This is called by console driver before calling ucom_cngetc().
 */
static void
ucom_cnpollc(dev_t dev, int on)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct ucom_cnstate * const cn = sc->sc_cn;
	struct usbd_device *udev = sc->sc_udev;

	if (on)
		cn->cn_polling = true;

	usbd_set_polling(udev, on);

	if (on) {
		/*
		 * Poll for regular output completion
		 */
		while(!db_active && atomic_load_relaxed(&cn->cn_sending) == 1)
			usbd_dopoll(sc->sc_iface);
	} else {
		cn->cn_polling = false;
	}
}

static void
ucom_cnfree(struct ucom_softc *sc)
{
	struct ucom_cnstate * const cn = sc->sc_cn;

	usbd_destroy_xfer(cn->cn_xfer_out);

	callout_halt(&cn->cn_callout, NULL);
	callout_destroy(&cn->cn_callout);

	softint_disestablish(cn->cn_si);

	if (cn->cn_buf_in)
		kmem_free(cn->cn_buf_in, sc->sc_ibufsize);

	if (cn->cn_buf_out)
		kmem_free(cn->cn_buf_out, cn->cn_buf_out_size);

	if (cn->cn_linear_buf_out)
		kmem_free(cn->cn_linear_buf_out, sc->sc_obufsize);

	kmem_free(cn, sizeof(*cn));

	sc->sc_cn = NULL;
}

static void
ucom_cnhalt(dev_t dev)
{
	const int unit = UCOMUNIT(dev);
	struct ucom_softc * const sc = device_lookup_private(&ucom_cd, unit);
	struct ucom_cnstate * const cn = sc->sc_cn;

	if (cn_tab != &ucom_cnops)
		return;

	/*
	 * Make sure we do not get any new output, and wait for
	 * pending ones to complete
	 */
	cn_tab = NULL;
	while (atomic_load_relaxed(&cn->cn_sending) == 1)
		kpause("ucomhalt", true, mstohz(1), NULL);

	cn_destroy_magic(&cn->cn_magic_state);

	ucom_cnfree(sc);
}

#ifdef DDB
/*
 * This is almost cn_check_magic() but the call to cn_trap() is
 * deferred so that it happens in ucom_softintr().
 */
static void
ucom_cncheckmagic(struct ucom_softc *sc, struct ucom_buffer *ub)
{
	struct ucom_cnstate *cn = sc->sc_cn;
	struct cnm_state *s = &cn->cn_magic_state;

	if (cn == NULL || db_active)
		return;

	for (size_t i = ub->ub_index; i < ub->ub_len; i++) {
		int _v = s->cnm_magic[s->cnm_state];
		if (ub->ub_data[i] == CNS_MAGIC_VAL(_v)) {
			s->cnm_state = CNS_MAGIC_NEXT(_v);
			if (s->cnm_state == CNS_TERM) {
				cn->cn_magic_req = true;
				s->cnm_state = 0;

				kpreempt_disable();
				softint_schedule(sc->sc_si);
				kpreempt_enable();
				break;
			}
		} else {
			s->cnm_state = 0;
		}
	}
}
#endif /* DDB */

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
