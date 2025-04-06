/*	$NetBSD: if_gscan.c,v 1.2 2025/04/06 03:33:51 riastradh Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
__KERNEL_RCSID(0, "$NetBSD: if_gscan.c,v 1.2 2025/04/06 03:33:51 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_net_mpsafe.h"
#include "opt_can.h"
#endif

#include <sys/param.h>

#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>
#include <sys/module.h>
#include <sys/syslog.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_types.h>

#include <netcan/can.h>
#include <netcan/can_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#include <dev/usb/usbhist.h>
#include <dev/usb/if_gscanreg.h>

#ifdef USB_DEBUG
#ifndef GSCAN_DEBUG
#define gscandebug 0
#else
static int gscandebug = 0;
SYSCTL_SETUP(sysctl_hw_gscan_setup, "sysctl hw.gscan setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "gscan",
	    SYSCTL_DESCR("gscan global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;
	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &gscandebug, sizeof(gscandebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* GSCAN_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(gscandebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(gscandebug,N,FMT,A,B,C,D)
#define GSCANHIST_FUNC()	USBHIST_FUNC()
#define GSCANHIST_CALLED(name)	USBHIST_CALLED(gscandebug)
#define GSCANHIST_CALLARGS(FMT,A,B,C,D) \
		USBHIST_CALLARGS(gscandebug,FMT,A,B,C,D)

struct gscan_softc {
	struct canif_softc sc_cansc;
	struct usbd_interface *sc_iface;
	struct usbd_device    *sc_udev;
	uByte sc_ed_tx;
	uByte sc_ed_rx;
	struct usbd_pipe *sc_tx_pipe;
	struct usbd_pipe *sc_rx_pipe;
	struct usbd_xfer *sc_tx_xfer;
	struct usbd_xfer *sc_rx_xfer;
	struct gscan_frame *sc_tx_frame;
	struct gscan_frame *sc_rx_frame;
	kmutex_t sc_txlock;
	kmutex_t sc_rxlock;
	bool sc_txstopped;
	bool sc_rxstopped;
	int sc_rx_nerr;
	struct ifnet *sc_ifp;
	struct if_percpuq *sc_ipq;
	volatile bool sc_dying;
	krndsource_t sc_rnd_source;
	struct mbuf *sc_m_transmit; /* mbuf being transmitted */
};

#define sc_dev	  sc_cansc.csc_dev
#define sc_timecaps     sc_cansc.csc_timecaps
#define sc_timings      sc_cansc.csc_timings
#define sc_linkmodes    sc_cansc.csc_linkmodes

static bool
gscan_isdying(struct gscan_softc *sc)
{
	return atomic_load_relaxed(&sc->sc_dying);
}

static int
gscan_write_device(struct gscan_softc *sc, int breq, void *v, int len)
{
	usb_device_request_t req;
	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest =  breq;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return usbd_do_request(sc->sc_udev, &req, v);
}

static int
gscan_read_device(struct gscan_softc *sc, int breq, void *v, int len)
{
	usb_device_request_t req;
	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest =  breq;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return usbd_do_request(sc->sc_udev, &req, v);
}

static int	gscan_match(device_t, cfdata_t, void *);
static void	gscan_attach(device_t, device_t, void *);
static int      gscan_detach(device_t, int);
static int	gscan_activate(device_t, devact_t);

static void	gscan_ifstart(struct ifnet *);
static int	gscan_ifioctl(struct ifnet *, u_long, void *);
static void	gscan_ifwatchdog(struct ifnet *);

static int 	gscan_ifup(struct gscan_softc * const);
static void	gscan_stop(struct gscan_softc * const, struct ifnet *, int);
static void	gscan_startrx(struct gscan_softc * const);

CFATTACH_DECL_NEW(gscan, sizeof(struct gscan_softc),
	gscan_match, gscan_attach, gscan_detach, gscan_activate);

static void
gscan_rx(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	GSCANHIST_FUNC();
	struct gscan_softc *sc = priv;
	struct gscan_frame *gsframe;
	struct can_frame *cf;
	uint32_t len,  dlc, can_id;
	int32_t echo_id;
	struct ifnet *ifp = sc->sc_ifp;
	struct mbuf *m;

	GSCANHIST_CALLARGS("status: %d", status, 0, 0, 0);

	mutex_enter(&sc->sc_rxlock);
	if (sc->sc_rxstopped || gscan_isdying(sc) ||
	    status == USBD_NOT_STARTED || status == USBD_CANCELLED ||
	    status == USBD_INVAL) {
		mutex_exit(&sc->sc_rxlock);
		return;
	}
	if (status != USBD_NORMAL_COMPLETION) {
		DPRINTF("rx error: %s\n", usbd_errstr(status), 0, 0, 0);
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_rx_pipe);
		if (++sc->sc_rx_nerr > 100) {
			log(LOG_ERR, "%s: too many rx errors, disabling\n",
			    device_xname(sc->sc_dev));
			gscan_activate(sc->sc_dev, DVACT_DEACTIVATE);
		}
		goto out;
	}
	sc->sc_rx_nerr = 0;
	usbd_get_xfer_status(xfer, NULL, (void **)&gsframe, &len, NULL);
	if (len < sizeof(struct gscan_frame) - 8) {
		if_statinc(ifp, if_ierrors);
		goto out;
	}
	if (gsframe->gsframe_flags & GSFRAME_FLAG_OVER) {
		if_statinc(ifp, if_ierrors);
		goto out;
	}
	dlc = le32toh(gsframe->gsframe_can_dlc);
	if (dlc > CAN_MAX_DLC) {
		if_statinc(ifp, if_ierrors);
		goto out;
	}
	echo_id = le32toh(gsframe->gsframe_echo_id);
	if (echo_id != -1) {
		/* echo of a frame we sent */
		goto out;
	}
	can_id = le32toh(gsframe->gsframe_can_id);
	/* for now ignore error frames */
	if (can_id & CAN_ERR_FLAG) {
		goto out;
	}
	m = m_gethdr(M_NOWAIT, MT_HEADER);
	if (m == NULL) {
		if_statinc(ifp, if_ierrors);
		goto out;
	}
	cf = mtod(m, struct can_frame *);
	memset(cf, 0, sizeof(struct can_frame));
	cf->can_id = can_id;
	cf->can_dlc = dlc;
	memcpy(&cf->data[0], &gsframe->gsframe_can_data[0], 8);
	/* done with the buffer, get next frame */
	mutex_exit(&sc->sc_rxlock);
	gscan_startrx(sc);

	m->m_len = m->m_pkthdr.len = CAN_MTU;
	m_set_rcvif(m, ifp);
	if_statadd(ifp, if_ibytes, m->m_len);
	can_bpf_mtap(ifp, m, 1);
	can_input(ifp, m);
	return;

out:
	mutex_exit(&sc->sc_rxlock);
	gscan_startrx(sc);
}

static void
gscan_startrx(struct gscan_softc * const sc)
{
	usbd_setup_xfer(sc->sc_rx_xfer, sc, sc->sc_rx_frame,
	    sizeof(struct gscan_frame), USBD_SHORT_XFER_OK,
	    USBD_NO_TIMEOUT, gscan_rx);
	usbd_transfer(sc->sc_rx_xfer);
}

static void
gscan_tx(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	GSCANHIST_FUNC();
	struct gscan_softc *sc = priv;
	struct ifnet *ifp = sc->sc_ifp;
	struct mbuf *m;

	GSCANHIST_CALLARGS("status: %d", status, 0, 0, 0);
	mutex_enter(&sc->sc_txlock);
	if (sc->sc_txstopped || gscan_isdying(sc)) {
		mutex_exit(&sc->sc_txlock);
		return;
	}
	ifp->if_timer = 0;
	m = sc->sc_m_transmit;
	sc->sc_m_transmit = NULL;
	if (m != NULL) {
		if (status == USBD_NORMAL_COMPLETION)
			if_statadd2(ifp, if_obytes, m->m_len, if_opackets, 1);
		can_mbuf_tag_clean(m);
		m_set_rcvif(m, ifp);
		can_input(ifp, m); /* loopback */
	}
	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			mutex_exit(&sc->sc_txlock);
			return;
		}
		DPRINTF("rx error: %s\n", usbd_errstr(status), 0, 0, 0);
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_rx_pipe);
	}
	if_schedule_deferred_start(ifp);
	mutex_exit(&sc->sc_txlock);
}

static void
gscan_ifstart(struct ifnet *ifp)
{
	GSCANHIST_FUNC();
	struct gscan_softc * const sc = ifp->if_softc;
	struct mbuf *m;
	struct can_frame *cf;
	int err;
	GSCANHIST_CALLED();

	mutex_enter(&sc->sc_txlock);
	if (sc->sc_txstopped || gscan_isdying(sc))
		goto out;

	if (sc->sc_m_transmit != NULL)
		goto out;
	IF_DEQUEUE(&ifp->if_snd, m);
	if (m == NULL)
		goto out;

	MCLAIM(m, ifp->if_mowner);

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT(m->m_len == m->m_pkthdr.len);

	cf = mtod(m, struct can_frame *);
	memset(sc->sc_tx_frame, 0, sizeof(struct gscan_frame));
	sc->sc_tx_frame->gsframe_echo_id = 0;
	sc->sc_tx_frame->gsframe_can_id = htole32(cf->can_id);
	sc->sc_tx_frame->gsframe_can_dlc = htole32(cf->can_dlc);
	memcpy(&sc->sc_tx_frame->gsframe_can_data[0], &cf->data[0], 8);

	usbd_setup_xfer(sc->sc_tx_xfer, sc, sc->sc_tx_frame,
	    sizeof(struct gscan_frame), 0, 10000, gscan_tx);
        err = usbd_transfer(sc->sc_tx_xfer);
	if (err != USBD_IN_PROGRESS) {
		DPRINTF("start tx error: %s\n", usbd_errstr(err), 0, 0, 0);
		if_statadd(ifp, if_oerrors, 1);
	} else {
		sc->sc_m_transmit = m;
		ifp->if_timer = 5;
	}
	can_bpf_mtap(ifp, m, 0);
out:
	mutex_exit(&sc->sc_txlock);
}

static int
gscan_ifup(struct gscan_softc * const sc)
{
	struct gscan_bt gscan_bt;
	struct gscan_set_mode gscan_set_mode;
	int err;
	struct ifnet * const ifp = sc->sc_ifp;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	gscan_set_mode.mode_mode = MODE_START;
	gscan_set_mode.mode_flags = 0;

	if (sc->sc_linkmodes & CAN_LINKMODE_LISTENONLY) {
		if ((sc->sc_timecaps.cltc_linkmode_caps & CAN_LINKMODE_LISTENONLY) == 0)
			return EINVAL;
		gscan_set_mode.mode_flags |= FLAGS_LISTEN_ONLY;
	}
	if (sc->sc_linkmodes & CAN_LINKMODE_LOOPBACK) {
		if ((sc->sc_timecaps.cltc_linkmode_caps & CAN_LINKMODE_LOOPBACK) == 0)
			return EINVAL;
		gscan_set_mode.mode_flags |= FLAGS_LOOPBACK;
	}
	if (sc->sc_linkmodes & CAN_LINKMODE_3SAMPLES) {
		if ((sc->sc_timecaps.cltc_linkmode_caps & CAN_LINKMODE_3SAMPLES) == 0)
			return EINVAL;
		gscan_set_mode.mode_flags |= FLAGS_TRIPLE_SAMPLE;
	}
	if (sc->sc_timings.clt_prop != 0)
		return EINVAL;
	gscan_bt.bt_prop_seg = 0;

	if (sc->sc_timings.clt_brp > sc->sc_timecaps.cltc_brp_max ||
	   sc->sc_timings.clt_brp < sc->sc_timecaps.cltc_brp_min)
		return EINVAL;
	gscan_bt.bt_brp = sc->sc_timings.clt_brp;

	if (sc->sc_timings.clt_ps1 > sc->sc_timecaps.cltc_ps1_max ||
	   sc->sc_timings.clt_ps1 < sc->sc_timecaps.cltc_ps1_min)
		return EINVAL;
	gscan_bt.bt_phase_seg1 = sc->sc_timings.clt_ps1;
	if (sc->sc_timings.clt_ps2 > sc->sc_timecaps.cltc_ps2_max ||
	   sc->sc_timings.clt_ps2 < sc->sc_timecaps.cltc_ps2_min)
		return EINVAL;
	gscan_bt.bt_phase_seg2 = sc->sc_timings.clt_ps2;
	if (sc->sc_timings.clt_sjw > sc->sc_timecaps.cltc_sjw_max ||
	    sc->sc_timings.clt_sjw < 1)
		return EINVAL;
	gscan_bt.bt_swj = sc->sc_timings.clt_sjw;

	err = gscan_write_device(sc, GSCAN_SET_BITTIMING,
	     &gscan_bt, sizeof(gscan_bt));
	if (err) {
		aprint_error_dev(sc->sc_dev, "SET_BITTIMING: %s\n",
		    usbd_errstr(err));
		return EIO;
	}
	err = gscan_write_device(sc, GSCAN_SET_MODE,
	     &gscan_set_mode, sizeof(gscan_set_mode));
	if (err) {
		aprint_error_dev(sc->sc_dev, "SET_MODE start: %s\n",
		    usbd_errstr(err));
		return EIO;
	}

	if ((err = usbd_open_pipe(sc->sc_iface, sc->sc_ed_rx,
	    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &sc->sc_rx_pipe)) != 0) {
		aprint_error_dev(sc->sc_dev, "open rx pipe: %s\n",
		    usbd_errstr(err));
		goto fail;
	}
	if ((err = usbd_open_pipe(sc->sc_iface, sc->sc_ed_tx,
	    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &sc->sc_tx_pipe)) != 0) {
		aprint_error_dev(sc->sc_dev, "open tx pipe: %s\n",
		    usbd_errstr(err));
		goto fail;
	}

	if ((err = usbd_create_xfer(sc->sc_rx_pipe, sizeof(struct gscan_frame),
	    0, 0, &sc->sc_rx_xfer)) != 0) {
		aprint_error_dev(sc->sc_dev, "create rx xfer: %s\n",
		    usbd_errstr(err));
		goto fail;
	}
	if ((err = usbd_create_xfer(sc->sc_tx_pipe, sizeof(struct gscan_frame),
	    0, 0, &sc->sc_tx_xfer)) != 0) {
		aprint_error_dev(sc->sc_dev, "create tx xfer: %s\n",
		    usbd_errstr(err));
		goto fail;
	}

	sc->sc_rx_frame = usbd_get_buffer(sc->sc_rx_xfer);
	sc->sc_tx_frame = usbd_get_buffer(sc->sc_tx_xfer);
	sc->sc_ifp->if_flags |= IFF_RUNNING;

	mutex_enter(&sc->sc_rxlock);
	sc->sc_rxstopped = false;
	sc->sc_rx_nerr = 0;
	mutex_exit(&sc->sc_rxlock);
	gscan_startrx(sc);
	mutex_enter(&sc->sc_txlock);
	sc->sc_txstopped = false;
	mutex_exit(&sc->sc_txlock);
	return 0;

fail:
	gscan_stop(sc, ifp, 1);
	return EIO;
}

static void
gscan_stop(struct gscan_softc * const sc, struct ifnet *ifp, int disable)
{
	struct gscan_set_mode gscan_set_mode;
	int err;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	mutex_enter(&sc->sc_txlock);
	sc->sc_txstopped = true;
	ifp->if_timer = 0;
	if (sc->sc_m_transmit != NULL) {
		m_freem(sc->sc_m_transmit);
		sc->sc_m_transmit = NULL;
	}
	mutex_exit(&sc->sc_txlock);
	mutex_enter(&sc->sc_rxlock);
	sc->sc_rxstopped = true;
	mutex_exit(&sc->sc_rxlock);
	if (ifp->if_flags & IFF_RUNNING) {
		if (sc->sc_tx_pipe)
			usbd_abort_pipe(sc->sc_tx_pipe);
		if (sc->sc_rx_pipe)
			usbd_abort_pipe(sc->sc_rx_pipe);
	}
	if (sc->sc_rx_pipe) {
		usbd_close_pipe(sc->sc_rx_pipe);
		sc->sc_rx_pipe = NULL;
	}
	if (sc->sc_tx_pipe) {
		usbd_close_pipe(sc->sc_tx_pipe);
		sc->sc_tx_pipe = NULL;
	}
	if (sc->sc_rx_xfer != NULL) {
		usbd_destroy_xfer(sc->sc_rx_xfer);
		sc->sc_rx_xfer = NULL;
		sc->sc_rx_pipe = NULL;
	}
	if (sc->sc_tx_xfer != NULL) {
		usbd_destroy_xfer(sc->sc_tx_xfer);
		sc->sc_tx_xfer = NULL;
		sc->sc_tx_pipe = NULL;
	}

	gscan_set_mode.mode_mode = MODE_RESET;
	gscan_set_mode.mode_flags = 0;
	err = gscan_write_device(sc, GSCAN_SET_MODE,
	     &gscan_set_mode, sizeof(gscan_set_mode));
	if (err != 0 && err  != USBD_CANCELLED) {
		aprint_error_dev(sc->sc_dev, "SET_MODE stop: %s\n",
		    usbd_errstr(err));
	}
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	ifp->if_flags &= ~IFF_RUNNING;
}

static void
gscan_ifstop(struct ifnet *ifp, int disable)
{
	struct gscan_softc * const sc = ifp->if_softc;
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	gscan_stop(sc, ifp, disable);
}


static int
gscan_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct gscan_softc * const sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error = 0;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	if (gscan_isdying(sc))
		return EIO;

	switch (cmd) {
	case SIOCINITIFADDR:
		error = EAFNOSUPPORT;
		break;
	case SIOCSIFMTU:
		if ((unsigned)ifr->ifr_mtu != sizeof(struct can_frame))
			error = EINVAL;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = EAFNOSUPPORT;
		break;
	default:
		error = ifioctl_common(ifp, cmd, data);
		if (error == 0) {
			if ((ifp->if_flags & IFF_UP) != 0 &&
			    (ifp->if_flags & IFF_RUNNING) == 0) {
				error = gscan_ifup(sc);
				if (error) {
					ifp->if_flags &= ~IFF_UP;
				}
			} else if ((ifp->if_flags & IFF_UP) == 0 &&
			    (ifp->if_flags & IFF_RUNNING) != 0) {
				gscan_stop(sc, sc->sc_ifp, 1);
			}
		}
		break;
	}
	return error;
}

static void
gscan_ifwatchdog(struct ifnet *ifp)
{
	struct gscan_softc * const sc = ifp->if_softc;
	printf("%s: watchdog timeout\n", device_xname(sc->sc_dev));
#if 0
	/* if there is a transmit in progress abort */
	if (gscan_tx_abort(sc)) {
		if_statinc(ifp, if_oerrors);
	}
#endif
}

static const struct usb_devno gscan_devs[] = {
    {USB_VENDOR_FUTUREBITS, USB_PRODUCT_FUTUREBITS_CDL_CAN},
    {USB_VENDOR_INTERBIO, USB_PRODUCT_INTERBIO_CDL_CAN},
};

static int
gscan_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return
	    (usb_lookup(gscan_devs, uaa->uaa_vendor, uaa->uaa_product) != NULL ?
	     UMATCH_VENDOR_PRODUCT : UMATCH_NONE);
}

static void
gscan_attach(device_t parent, device_t self, void *aux)
{
	GSCANHIST_FUNC(); GSCANHIST_CALLED();
	struct gscan_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	struct usbd_device *dev = uaa->uaa_device;
	usbd_status err;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	uint32_t val32;
	struct gscan_config gscan_config;
	struct gscan_bt_const gscan_bt_const;
	struct ifnet *ifp;

	aprint_naive("\n");
	aprint_normal("\n");
	devinfop = usbd_devinfo_alloc(dev, 0);
	aprint_normal_dev(self, "%s\n", devinfop);
	usbd_devinfo_free(devinfop);

	sc->sc_dev = self;
	sc->sc_udev = dev;

	err = usbd_set_config_no(dev, 1, 0);
	if (err) {
		aprint_error_dev(self, "failed to set configuration"
		    ", err=%s\n", usbd_errstr(err));
		return;
	}

	err = usbd_device2interface_handle(dev, 0, &sc->sc_iface);
	if (err) {
		aprint_error_dev(self, "getting interface handle failed\n");
		return;
	}

	id = usbd_get_interface_descriptor(sc->sc_iface);
	if (id->bNumEndpoints < 2) {
		aprint_error_dev(self, "%d endpoints < 2\n", id->bNumEndpoints);
		return;
	}

	val32 = htole32(0x0000beef);
	err = gscan_write_device(sc, GSCAN_SET_HOST_FORMAT,
	    &val32, sizeof(val32));

	if (err) {
		aprint_error_dev(self, "SET_HOST_FORMAT: %s\n",
		    usbd_errstr(err));
		return;
	}

	err = gscan_read_device(sc, GSCAN_GET_DEVICE_CONFIG,
	    &gscan_config, sizeof(struct gscan_config));
	if (err) {
		aprint_error_dev(self, "GET_DEVICE_CONFIG: %s\n",
		    usbd_errstr(err));
		return;
	}
	aprint_normal_dev(self, "%d port%s, sw version %d, hw version %d\n",
	    gscan_config.conf_count + 1, gscan_config.conf_count ? "s" : "",
	    le32toh(gscan_config.sw_version), le32toh(gscan_config.hw_version));

	err = gscan_read_device(sc, GSCAN_GET_BT_CONST,
	    &gscan_bt_const, sizeof(struct gscan_bt_const));
	if (err) {
		aprint_error_dev(self, "GET_BT_CONST: %s\n",
		    usbd_errstr(err));
		return;
	}
	aprint_debug_dev(self, "feat 0x%x clk %dHz tseg1 %d -> %d tseg2 %d -> %d max swj %d brp %d -> %d/%d\n",
	    le32toh(gscan_bt_const.btc_features),
	    le32toh(gscan_bt_const.btc_fclk),
	    le32toh(gscan_bt_const.btc_tseg1_min),
	    le32toh(gscan_bt_const.btc_tseg1_max),
	    le32toh(gscan_bt_const.btc_tseg2_min),
	    le32toh(gscan_bt_const.btc_tseg2_max),
	    le32toh(gscan_bt_const.btc_swj_max),
	    le32toh(gscan_bt_const.btc_brp_min),
	    le32toh(gscan_bt_const.btc_brp_max),
	    le32toh(gscan_bt_const.btc_brp_inc));

	sc->sc_timecaps.cltc_prop_min = 0;
	sc->sc_timecaps.cltc_prop_max = 0;
	sc->sc_timecaps.cltc_ps1_min = le32toh(gscan_bt_const.btc_tseg1_min);
	sc->sc_timecaps.cltc_ps1_max = le32toh(gscan_bt_const.btc_tseg1_max);
	sc->sc_timecaps.cltc_ps2_min = le32toh(gscan_bt_const.btc_tseg2_min);
	sc->sc_timecaps.cltc_ps2_max = le32toh(gscan_bt_const.btc_tseg2_max);
	sc->sc_timecaps.cltc_sjw_max = le32toh(gscan_bt_const.btc_swj_max);
	sc->sc_timecaps.cltc_brp_min = le32toh(gscan_bt_const.btc_brp_min);
	sc->sc_timecaps.cltc_brp_max = le32toh(gscan_bt_const.btc_brp_max);
	sc->sc_timecaps.cltc_brp_inc = le32toh(gscan_bt_const.btc_brp_inc);
	sc->sc_timecaps.cltc_clock_freq = le32toh(gscan_bt_const.btc_fclk);
	sc->sc_timecaps.cltc_linkmode_caps = 0;
	if (le32toh(gscan_bt_const.btc_features) & FEAT_LISTEN_ONLY)
		sc->sc_timecaps.cltc_linkmode_caps |= CAN_LINKMODE_LISTENONLY;
	if (le32toh(gscan_bt_const.btc_features) & FEAT_LOOPBACK)
		sc->sc_timecaps.cltc_linkmode_caps |= CAN_LINKMODE_LOOPBACK;
	if (le32toh(gscan_bt_const.btc_features) & FEAT_TRIPLE_SAMPLE)
		sc->sc_timecaps.cltc_linkmode_caps |= CAN_LINKMODE_3SAMPLES;

	can_ifinit_timings(&sc->sc_cansc);
	sc->sc_timings.clt_prop = 0;
	sc->sc_timings.clt_sjw = 1;

	/* Find endpoints. */
	ed = usbd_interface2endpoint_descriptor(sc->sc_iface, 0);
	if (ed == NULL) {
		aprint_error_dev(self, "couldn't get ep 1\n");
		return;
	}
	const uint8_t xt1 = UE_GET_XFERTYPE(ed->bmAttributes);
	const uint8_t dir1 = UE_GET_DIR(ed->bEndpointAddress);

	if (dir1 != UE_DIR_IN || xt1 != UE_BULK) {
		aprint_error_dev(self,
		    "ep 1 wrong dir %d or xt %d\n", dir1, xt1);
		return;
	}
	sc->sc_ed_rx = ed->bEndpointAddress;

	ed = usbd_interface2endpoint_descriptor(sc->sc_iface, 1);
	if (ed == NULL) {
		aprint_error_dev(self, "couldn't get ep 2\n");
		return;
	}
	const uint8_t xt2 = UE_GET_XFERTYPE(ed->bmAttributes);
	const uint8_t dir2 = UE_GET_DIR(ed->bEndpointAddress);

	if (dir2 != UE_DIR_OUT || xt2 != UE_BULK) {
		aprint_error_dev(self,
		    "ep 2 wrong dir %d or xt %d\n", dir2, xt2);
		return;
	}
	sc->sc_ed_tx = ed->bEndpointAddress;

	mutex_init(&sc->sc_txlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_rxlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	sc->sc_rxstopped = true;
	sc->sc_txstopped = true;


	ifp = if_alloc(IFT_OTHER);
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_capabilities = 0;
	ifp->if_flags = 0;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_start = gscan_ifstart;
	ifp->if_ioctl = gscan_ifioctl;
	ifp->if_stop = gscan_ifstop;
	ifp->if_watchdog = gscan_ifwatchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);

	sc->sc_ifp = ifp;
	can_ifattach(ifp);
	if_deferred_start_init(ifp, NULL);
	bpf_mtap_softint_init(ifp);
	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);
#ifdef MBUFTRACE
	ifp->if_mowner = kmem_zalloc(sizeof(*ifp->if_mowner), KM_SLEEP);
	strlcpy(ifp->if_mowner->mo_name, ifp->if_xname,
		sizeof(ifp->if_mowner->mo_name));
	MOWNER_ATTACH(ifp->if_mowner);
#endif
	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev, sc->sc_dev);
};

static int
gscan_detach(device_t self, int flags)
{
	GSCANHIST_FUNC(); GSCANHIST_CALLED();
	struct gscan_softc * const sc = device_private(self);

	struct ifnet * const ifp = sc->sc_ifp;
	/*
	 * Prevent new activity.  After we stop the interface, it
	 * cannot be brought back up.
	 */
	atomic_store_relaxed(&sc->sc_dying, true);

	/*
	 * If we're still running on the network, stop and wait for all
	 * asynchronous activity to finish.
	 *
	 * If _attach_ifp never ran, IFNET_LOCK won't work, but
	 * no activity is possible, so just skip this part.
	 */
	if (ifp != NULL) {
		IFNET_LOCK(ifp);
		if (ifp->if_flags & IFF_RUNNING) {
			gscan_stop(sc, ifp, 1);
		}
		IFNET_UNLOCK(ifp);
		bpf_detach(ifp);
		if_detach(ifp);
	}
	rnd_detach_source(&sc->sc_rnd_source);
	mutex_destroy(&sc->sc_txlock);
	mutex_destroy(&sc->sc_rxlock);

	pmf_device_deregister(sc->sc_dev);
	if (ifp != NULL) {
		usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
		    sc->sc_dev);
	}
	return 0;
}

static int
gscan_activate(device_t self, devact_t act)
{
	GSCANHIST_FUNC(); GSCANHIST_CALLED();
	struct gscan_softc * const sc = device_private(self);
	struct ifnet * const ifp = sc->sc_ifp;

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(ifp);
		atomic_store_relaxed(&sc->sc_dying, true);
		mutex_enter(&sc->sc_txlock);
		sc->sc_txstopped = true;
		if (sc->sc_m_transmit != NULL) {
			m_freem(sc->sc_m_transmit);
			sc->sc_m_transmit = NULL;
		}
		mutex_exit(&sc->sc_txlock);
		mutex_enter(&sc->sc_rxlock);
		sc->sc_rxstopped = true;
		mutex_exit(&sc->sc_rxlock);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

#ifdef _MODULE
#include "ioconf.c"
#endif

MODULE(MODULE_CLASS_DRIVER, gscan, NULL);

static int
gscan_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_gscan,
		    cfattach_ioconf_gscan, cfdata_ioconf_gscan);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_gscan,
		    cfattach_ioconf_gscan, cfdata_ioconf_gscan);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
