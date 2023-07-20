/*	$NetBSD: usbnet.c,v 1.114 2023/07/15 21:41:26 andvar Exp $	*/

/*
 * Copyright (c) 2019 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Common code shared between USB network drivers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usbnet.c,v 1.114 2023/07/15 21:41:26 andvar Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/atomic.h>

#include <dev/usb/usbnet.h>
#include <dev/usb/usbhist.h>

struct usbnet_cdata {
	struct usbnet_chain	*uncd_tx_chain;
	struct usbnet_chain	*uncd_rx_chain;

	int			uncd_tx_prod;
	int			uncd_tx_cnt;
};

struct usbnet_private {
	/*
	 * - unp_miilock protects the MII / media data and tick scheduling.
	 * - unp_rxlock protects the rx path and its data
	 * - unp_txlock protects the tx path and its data
	 *
	 * the lock ordering is:
	 *	ifnet lock -> unp_miilock
	 *		   -> unp_rxlock
	 *		   -> unp_txlock
	 *		   -> unp_mcastlock
	 */
	kmutex_t		unp_miilock;
	kmutex_t		unp_rxlock;
	kmutex_t		unp_txlock;

	kmutex_t		unp_mcastlock;
	bool			unp_mcastactive;

	struct usbnet_cdata	unp_cdata;

	struct ethercom		unp_ec;
	struct mii_data		unp_mii;
	struct usb_task		unp_ticktask;
	struct callout		unp_stat_ch;
	struct usbd_pipe	*unp_ep[USBNET_ENDPT_MAX];

	volatile bool		unp_dying;
	bool			unp_stopped;
	bool			unp_rxstopped;
	bool			unp_txstopped;
	bool			unp_attached;
	bool			unp_ifp_attached;
	bool			unp_link;

	int			unp_timer;
	unsigned short		unp_if_flags;
	unsigned		unp_number;

	krndsource_t		unp_rndsrc;

	struct timeval		unp_rx_notice;
	struct timeval		unp_tx_notice;
	struct timeval		unp_intr_notice;
};

#define un_cdata(un)	(&(un)->un_pri->unp_cdata)

volatile unsigned usbnet_number;

static void usbnet_isowned_rx(struct usbnet *);
static void usbnet_isowned_tx(struct usbnet *);

static inline void
usbnet_isowned_mii(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_pri->unp_miilock));
}

static int usbnet_modcmd(modcmd_t, void *);

#ifdef USB_DEBUG
#ifndef USBNET_DEBUG
#define usbnetdebug 0
#else
static int usbnetdebug = 0;

SYSCTL_SETUP(sysctl_hw_usbnet_setup, "sysctl hw.usbnet setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "usbnet",
	    SYSCTL_DESCR("usbnet global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &usbnetdebug, sizeof(usbnetdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* USBNET_DEBUG */
#endif /* USB_DEBUG */

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(usbnetdebug,1,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usbnetdebug,N,FMT,A,B,C,D)
#define USBNETHIST_FUNC()	USBHIST_FUNC()
#define USBNETHIST_CALLED(name)	USBHIST_CALLED(usbnetdebug)
#define USBNETHIST_CALLARGS(FMT,A,B,C,D) \
				USBHIST_CALLARGS(usbnetdebug,FMT,A,B,C,D)
#define USBNETHIST_CALLARGSN(N,FMT,A,B,C,D) \
				USBHIST_CALLARGSN(usbnetdebug,N,FMT,A,B,C,D)

/* Callback vectors. */

static void
uno_stop(struct usbnet *un, struct ifnet *ifp, int disable)
{
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	if (un->un_ops->uno_stop)
		(*un->un_ops->uno_stop)(ifp, disable);
}

static int
uno_ioctl(struct usbnet *un, struct ifnet *ifp, u_long cmd, void *data)
{

	KASSERTMSG(cmd != SIOCADDMULTI, "%s", ifp->if_xname);
	KASSERTMSG(cmd != SIOCDELMULTI, "%s", ifp->if_xname);
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	if (un->un_ops->uno_ioctl)
		return (*un->un_ops->uno_ioctl)(ifp, cmd, data);
	return 0;
}

static int
uno_override_ioctl(struct usbnet *un, struct ifnet *ifp, u_long cmd, void *data)
{

	switch (cmd) {
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;
	default:
		KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	}

	return (*un->un_ops->uno_override_ioctl)(ifp, cmd, data);
}

static int
uno_init(struct usbnet *un, struct ifnet *ifp)
{
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	return un->un_ops->uno_init ? (*un->un_ops->uno_init)(ifp) : 0;
}

static int
uno_read_reg(struct usbnet *un, int phy, int reg, uint16_t *val)
{
	usbnet_isowned_mii(un);
	return (*un->un_ops->uno_read_reg)(un, phy, reg, val);
}

static int
uno_write_reg(struct usbnet *un, int phy, int reg, uint16_t val)
{
	usbnet_isowned_mii(un);
	return (*un->un_ops->uno_write_reg)(un, phy, reg, val);
}

static void
uno_mii_statchg(struct usbnet *un, struct ifnet *ifp)
{
	usbnet_isowned_mii(un);
	(*un->un_ops->uno_statchg)(ifp);
}

static unsigned
uno_tx_prepare(struct usbnet *un, struct mbuf *m, struct usbnet_chain *c)
{
	usbnet_isowned_tx(un);
	return (*un->un_ops->uno_tx_prepare)(un, m, c);
}

static void
uno_rx_loop(struct usbnet *un, struct usbnet_chain *c, uint32_t total_len)
{
	usbnet_isowned_rx(un);
	(*un->un_ops->uno_rx_loop)(un, c, total_len);
}

static void
uno_tick(struct usbnet *un)
{
	if (un->un_ops->uno_tick)
		(*un->un_ops->uno_tick)(un);
}

static void
uno_intr(struct usbnet *un, usbd_status status)
{
	if (un->un_ops->uno_intr)
		(*un->un_ops->uno_intr)(un, status);
}

/* Interrupt handling. */

static struct mbuf *
usbnet_newbuf(size_t buflen)
{
	struct mbuf *m;

	if (buflen > MCLBYTES - ETHER_ALIGN)
		return NULL;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	if (buflen > MHLEN - ETHER_ALIGN) {
		MCLGET(m, M_DONTWAIT);
		if (!(m->m_flags & M_EXT)) {
			m_freem(m);
			return NULL;
		}
	}

	m->m_len = m->m_pkthdr.len = ETHER_ALIGN + buflen;
	m_adj(m, ETHER_ALIGN);

	return m;
}

/*
 * usbnet_rxeof() is designed to be the done callback for rx completion.
 * it provides generic setup and finalisation, calls a different usbnet
 * rx_loop callback in the middle, which can use usbnet_enqueue() to
 * enqueue a packet for higher levels (or usbnet_input() if previously
 * using if_input() path.)
 */
void
usbnet_enqueue(struct usbnet * const un, uint8_t *buf, size_t buflen,
	       int csum_flags, uint32_t csum_data, int mbuf_flags)
{
	USBNETHIST_FUNC();
	struct ifnet * const ifp = usbnet_ifp(un);
	struct usbnet_private * const unp __unused = un->un_pri;
	struct mbuf *m;

	USBNETHIST_CALLARGSN(5, "%jd: enter: len=%ju csf %#jx mbf %#jx",
	    unp->unp_number, buflen, csum_flags, mbuf_flags);

	usbnet_isowned_rx(un);

	m = usbnet_newbuf(buflen);
	if (m == NULL) {
		DPRINTF("%jd: no memory", unp->unp_number, 0, 0, 0);
		if_statinc(ifp, if_ierrors);
		return;
	}

	m_set_rcvif(m, ifp);
	m->m_pkthdr.csum_flags = csum_flags;
	m->m_pkthdr.csum_data = csum_data;
	m->m_flags |= mbuf_flags;
	memcpy(mtod(m, uint8_t *), buf, buflen);

	/* push the packet up */
	if_percpuq_enqueue(ifp->if_percpuq, m);
}

void
usbnet_input(struct usbnet * const un, uint8_t *buf, size_t buflen)
{
	USBNETHIST_FUNC();
	struct ifnet * const ifp = usbnet_ifp(un);
	struct usbnet_private * const unp __unused = un->un_pri;
	struct mbuf *m;

	USBNETHIST_CALLARGSN(5, "%jd: enter: buf %#jx len %ju",
	    unp->unp_number, (uintptr_t)buf, buflen, 0);

	usbnet_isowned_rx(un);

	m = usbnet_newbuf(buflen);
	if (m == NULL) {
		if_statinc(ifp, if_ierrors);
		return;
	}

	m_set_rcvif(m, ifp);
	memcpy(mtod(m, char *), buf, buflen);

	/* push the packet up */
	if_input(ifp, m);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
usbnet_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	USBNETHIST_FUNC();
	struct usbnet_chain * const c = priv;
	struct usbnet * const un = c->unc_un;
	struct usbnet_private * const unp = un->un_pri;
	uint32_t total_len;

	USBNETHIST_CALLARGSN(5, "%jd: enter: status %#jx xfer %#jx",
	    unp->unp_number, status, (uintptr_t)xfer, 0);

	mutex_enter(&unp->unp_rxlock);

	if (usbnet_isdying(un) || unp->unp_rxstopped ||
	    status == USBD_INVAL || status == USBD_NOT_STARTED ||
	    status == USBD_CANCELLED)
		goto out;

	if (status != USBD_NORMAL_COMPLETION) {
		if (usbd_ratecheck(&unp->unp_rx_notice))
			device_printf(un->un_dev, "usb errors on rx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(unp->unp_ep[USBNET_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	if (total_len > un->un_rx_bufsz) {
		aprint_error_dev(un->un_dev,
		    "rxeof: too large transfer (%u > %u)\n",
		    total_len, un->un_rx_bufsz);
		goto done;
	}

	uno_rx_loop(un, c, total_len);
	usbnet_isowned_rx(un);

done:
	if (usbnet_isdying(un) || unp->unp_rxstopped)
		goto out;

	mutex_exit(&unp->unp_rxlock);

	/* Setup new transfer. */
	usbd_setup_xfer(xfer, c, c->unc_buf, un->un_rx_bufsz,
	    un->un_rx_xfer_flags, USBD_NO_TIMEOUT, usbnet_rxeof);
	usbd_transfer(xfer);
	return;

out:
	mutex_exit(&unp->unp_rxlock);
}

static void
usbnet_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet_chain * const c = priv;
	struct usbnet * const un = c->unc_un;
	struct usbnet_cdata * const cd = un_cdata(un);
	struct usbnet_private * const unp = un->un_pri;
	struct ifnet * const ifp = usbnet_ifp(un);

	USBNETHIST_CALLARGSN(5, "%jd: enter: status %#jx xfer %#jx",
	    unp->unp_number, status, (uintptr_t)xfer, 0);

	mutex_enter(&unp->unp_txlock);
	if (unp->unp_txstopped || usbnet_isdying(un)) {
		mutex_exit(&unp->unp_txlock);
		return;
	}

	KASSERT(cd->uncd_tx_cnt > 0);
	cd->uncd_tx_cnt--;

	unp->unp_timer = 0;

	switch (status) {
	case USBD_NOT_STARTED:
	case USBD_CANCELLED:
		break;

	case USBD_NORMAL_COMPLETION:
		if_statinc(ifp, if_opackets);
		break;

	default:

		if_statinc(ifp, if_oerrors);
		if (usbd_ratecheck(&unp->unp_tx_notice))
			device_printf(un->un_dev, "usb error on tx: %s\n",
			    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(unp->unp_ep[USBNET_ENDPT_TX]);
		break;
	}

	mutex_exit(&unp->unp_txlock);

	if (status == USBD_NORMAL_COMPLETION && !IFQ_IS_EMPTY(&ifp->if_snd))
		(*ifp->if_start)(ifp);
}

static void
usbnet_pipe_intr(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = priv;
	struct usbnet_private * const unp = un->un_pri;
	struct usbnet_intr * const uni __unused = un->un_intr;

	if (usbnet_isdying(un) ||
	    status == USBD_INVAL || status == USBD_NOT_STARTED ||
	    status == USBD_CANCELLED) {
		USBNETHIST_CALLARGS("%jd: uni %#jx dying %#jx status %#jx",
		    unp->unp_number, (uintptr_t)uni,
		    usbnet_isdying(un), status);
		return;
	}

	if (status != USBD_NORMAL_COMPLETION) {
		if (usbd_ratecheck(&unp->unp_intr_notice)) {
			aprint_error_dev(un->un_dev, "usb error on intr: %s\n",
			    usbd_errstr(status));
		}
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall_async(unp->unp_ep[USBNET_ENDPT_INTR]);
		USBNETHIST_CALLARGS("%jd: not normal status %#jx",
		    unp->unp_number, status, 0, 0);
		return;
	}

	uno_intr(un, status);
}

static void
usbnet_start_locked(struct ifnet *ifp)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = ifp->if_softc;
	struct usbnet_cdata * const cd = un_cdata(un);
	struct usbnet_private * const unp = un->un_pri;
	struct mbuf *m;
	unsigned length;
	bool done_transmit = false;
	int idx, count;

	USBNETHIST_CALLARGS("%jd: tx_cnt %jd list_cnt %jd link %jd",
	    unp->unp_number, cd->uncd_tx_cnt, un->un_tx_list_cnt,
	    unp->unp_link);

	usbnet_isowned_tx(un);
	KASSERT(cd->uncd_tx_cnt <= un->un_tx_list_cnt);
	KASSERT(!unp->unp_txstopped);

	if (!unp->unp_link) {
		DPRINTF("start called no link (%jx)",
		    unp->unp_link, 0, 0, 0);
		return;
	}

	if (cd->uncd_tx_cnt == un->un_tx_list_cnt) {
		DPRINTF("start called, tx busy (%#jx == %#jx)",
		    cd->uncd_tx_cnt, un->un_tx_list_cnt, 0, 0);
		return;
	}

	idx = cd->uncd_tx_prod;
	count = 0;
	while (cd->uncd_tx_cnt < un->un_tx_list_cnt) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL) {
			DPRINTF("start called, queue empty", 0, 0, 0, 0);
			break;
		}
		KASSERT(m->m_pkthdr.len <= un->un_tx_bufsz);

		struct usbnet_chain *c = &cd->uncd_tx_chain[idx];

		length = uno_tx_prepare(un, m, c);
		if (length == 0) {
			DPRINTF("uno_tx_prepare gave zero length", 0, 0, 0, 0);
			if_statinc(ifp, if_oerrors);
			break;
		}

		if (__predict_false(c->unc_xfer == NULL)) {
			DPRINTF("unc_xfer is NULL", 0, 0, 0, 0);
			if_statinc(ifp, if_oerrors);
			break;
		}

		usbd_setup_xfer(c->unc_xfer, c, c->unc_buf, length,
		    un->un_tx_xfer_flags, 10000, usbnet_txeof);

		/* Transmit */
		usbd_status err = usbd_transfer(c->unc_xfer);
		if (err != USBD_IN_PROGRESS) {
			DPRINTF("usbd_transfer on %#jx for %ju bytes: %jd",
			    (uintptr_t)c->unc_buf, length, err, 0);
			if_statinc(ifp, if_oerrors);
			break;
		}
		done_transmit = true;

		IFQ_DEQUEUE(&ifp->if_snd, m);

		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		bpf_mtap(ifp, m, BPF_D_OUT);
		m_freem(m);

		idx = (idx + 1) % un->un_tx_list_cnt;
		cd->uncd_tx_cnt++;
		count++;
	}
	cd->uncd_tx_prod = idx;

	DPRINTF("finished with start; tx_cnt %jd list_cnt %jd link %jd",
	    cd->uncd_tx_cnt, un->un_tx_list_cnt, unp->unp_link, 0);

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	if (done_transmit)
		unp->unp_timer = 5;

	if (count != 0)
		rnd_add_uint32(&unp->unp_rndsrc, count);
}

static void
usbnet_if_start(struct ifnet *ifp)
{
	struct usbnet * const un = ifp->if_softc;
	struct usbnet_private * const unp = un->un_pri;

	USBNETHIST_FUNC();
	USBNETHIST_CALLARGS("%jd: txstopped %jd",
	    unp->unp_number, unp->unp_txstopped, 0, 0);

	mutex_enter(&unp->unp_txlock);
	if (!unp->unp_txstopped)
		usbnet_start_locked(ifp);
	mutex_exit(&unp->unp_txlock);
}

/*
 * Chain management.
 *
 * RX and TX are identical. Keep them that way.
 */

/* Start of common RX functions */

static size_t
usbnet_rx_list_size(struct usbnet_cdata * const cd, struct usbnet * const un)
{
	return sizeof(*cd->uncd_rx_chain) * un->un_rx_list_cnt;
}

static void
usbnet_rx_list_alloc(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	cd->uncd_rx_chain = kmem_zalloc(usbnet_rx_list_size(cd, un), KM_SLEEP);
}

static void
usbnet_rx_list_free(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	if (cd->uncd_rx_chain) {
		kmem_free(cd->uncd_rx_chain, usbnet_rx_list_size(cd, un));
		cd->uncd_rx_chain = NULL;
	}
}

static int
usbnet_rx_list_init(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);
	struct usbnet_private * const unp = un->un_pri;

	for (size_t i = 0; i < un->un_rx_list_cnt; i++) {
		struct usbnet_chain *c = &cd->uncd_rx_chain[i];

		c->unc_un = un;
		if (c->unc_xfer == NULL) {
			int err = usbd_create_xfer(unp->unp_ep[USBNET_ENDPT_RX],
			    un->un_rx_bufsz, un->un_rx_xfer_flags, 0,
			    &c->unc_xfer);
			if (err)
				return err;
			c->unc_buf = usbd_get_buffer(c->unc_xfer);
		}
	}

	return 0;
}

static void
usbnet_rx_list_fini(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	for (size_t i = 0; i < un->un_rx_list_cnt; i++) {
		struct usbnet_chain *c = &cd->uncd_rx_chain[i];

		if (c->unc_xfer != NULL) {
			usbd_destroy_xfer(c->unc_xfer);
			c->unc_xfer = NULL;
			c->unc_buf = NULL;
		}
	}
}

/* End of common RX functions */

static void
usbnet_rx_start_pipes(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);
	struct usbnet_private * const unp = un->un_pri;

	mutex_enter(&unp->unp_rxlock);
	KASSERT(unp->unp_rxstopped);
	unp->unp_rxstopped = false;

	for (size_t i = 0; i < un->un_rx_list_cnt; i++) {
		struct usbnet_chain *c = &cd->uncd_rx_chain[i];

		usbd_setup_xfer(c->unc_xfer, c, c->unc_buf, un->un_rx_bufsz,
		    un->un_rx_xfer_flags, USBD_NO_TIMEOUT, usbnet_rxeof);
		usbd_transfer(c->unc_xfer);
	}

	mutex_exit(&unp->unp_rxlock);
}

/* Start of common TX functions */

static size_t
usbnet_tx_list_size(struct usbnet_cdata * const cd, struct usbnet * const un)
{
	return sizeof(*cd->uncd_tx_chain) * un->un_tx_list_cnt;
}

static void
usbnet_tx_list_alloc(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	cd->uncd_tx_chain = kmem_zalloc(usbnet_tx_list_size(cd, un), KM_SLEEP);
}

static void
usbnet_tx_list_free(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	if (cd->uncd_tx_chain) {
		kmem_free(cd->uncd_tx_chain, usbnet_tx_list_size(cd, un));
		cd->uncd_tx_chain = NULL;
	}
}

static int
usbnet_tx_list_init(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);
	struct usbnet_private * const unp = un->un_pri;

	for (size_t i = 0; i < un->un_tx_list_cnt; i++) {
		struct usbnet_chain *c = &cd->uncd_tx_chain[i];

		c->unc_un = un;
		if (c->unc_xfer == NULL) {
			int err = usbd_create_xfer(unp->unp_ep[USBNET_ENDPT_TX],
			    un->un_tx_bufsz, un->un_tx_xfer_flags, 0,
			    &c->unc_xfer);
			if (err)
				return err;
			c->unc_buf = usbd_get_buffer(c->unc_xfer);
		}
	}

	return 0;
}

static void
usbnet_tx_list_fini(struct usbnet * const un)
{
	struct usbnet_cdata * const cd = un_cdata(un);

	for (size_t i = 0; i < un->un_tx_list_cnt; i++) {
		struct usbnet_chain *c = &cd->uncd_tx_chain[i];

		if (c->unc_xfer != NULL) {
			usbd_destroy_xfer(c->unc_xfer);
			c->unc_xfer = NULL;
			c->unc_buf = NULL;
		}
	}
	cd->uncd_tx_prod = cd->uncd_tx_cnt = 0;
}

/* End of common TX functions */

/* Endpoint pipe management. */

static void
usbnet_ep_close_pipes(struct usbnet * const un)
{
	struct usbnet_private * const unp = un->un_pri;

	for (size_t i = 0; i < __arraycount(unp->unp_ep); i++) {
		if (unp->unp_ep[i] == NULL)
			continue;
		usbd_close_pipe(unp->unp_ep[i]);
		unp->unp_ep[i] = NULL;
	}
}

static usbd_status
usbnet_ep_open_pipes(struct usbnet * const un)
{
	struct usbnet_intr * const uni = un->un_intr;
	struct usbnet_private * const unp = un->un_pri;

	for (size_t i = 0; i < __arraycount(unp->unp_ep); i++) {
		usbd_status err;

		if (un->un_ed[i] == 0)
			continue;

		if (i == USBNET_ENDPT_INTR && uni) {
			err = usbd_open_pipe_intr(un->un_iface, un->un_ed[i],
			    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &unp->unp_ep[i], un,
			    uni->uni_buf, uni->uni_bufsz, usbnet_pipe_intr,
			    uni->uni_interval);
		} else {
			err = usbd_open_pipe(un->un_iface, un->un_ed[i],
			    USBD_EXCLUSIVE_USE | USBD_MPSAFE, &unp->unp_ep[i]);
		}
		if (err) {
			usbnet_ep_close_pipes(un);
			return err;
		}
	}

	return USBD_NORMAL_COMPLETION;
}

static void
usbnet_ep_stop_pipes(struct usbnet * const un)
{
	struct usbnet_private * const unp = un->un_pri;

	for (size_t i = 0; i < __arraycount(unp->unp_ep); i++) {
		if (unp->unp_ep[i] == NULL)
			continue;
		usbd_abort_pipe(unp->unp_ep[i]);
	}
}

static int
usbnet_init_rx_tx(struct usbnet * const un)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet_private * const unp = un->un_pri;
	struct ifnet * const ifp = usbnet_ifp(un);
	usbd_status err;
	int error = 0;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	if (usbnet_isdying(un)) {
		return EIO;
	}

	/* Open RX and TX pipes. */
	err = usbnet_ep_open_pipes(un);
	if (err) {
		aprint_error_dev(un->un_dev, "open rx/tx pipes failed: %s\n",
		    usbd_errstr(err));
		error = EIO;
		goto out;
	}

	/* Init RX ring. */
	if (usbnet_rx_list_init(un)) {
		aprint_error_dev(un->un_dev, "rx list init failed\n");
		error = ENOBUFS;
		goto out;
	}

	/* Init TX ring. */
	if (usbnet_tx_list_init(un)) {
		aprint_error_dev(un->un_dev, "tx list init failed\n");
		error = ENOBUFS;
		goto out;
	}

	/* Indicate we are up and running. */
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	ifp->if_flags |= IFF_RUNNING;

	/*
	 * If the hardware has a multicast filter, program it and then
	 * allow updates to it while we're running.
	 */
	if (un->un_ops->uno_mcast) {
		mutex_enter(&unp->unp_mcastlock);
		(*un->un_ops->uno_mcast)(ifp);
		unp->unp_mcastactive = true;
		mutex_exit(&unp->unp_mcastlock);
	}

	/* Allow transmit.  */
	mutex_enter(&unp->unp_txlock);
	KASSERT(unp->unp_txstopped);
	unp->unp_txstopped = false;
	mutex_exit(&unp->unp_txlock);

	/* Start up the receive pipe(s). */
	usbnet_rx_start_pipes(un);

	/* Kick off the watchdog/stats/mii tick.  */
	mutex_enter(&unp->unp_miilock);
	unp->unp_stopped = false;
	callout_schedule(&unp->unp_stat_ch, hz);
	mutex_exit(&unp->unp_miilock);

out:
	if (error) {
		usbnet_rx_list_fini(un);
		usbnet_tx_list_fini(un);
		usbnet_ep_close_pipes(un);
	}

	/*
	 * For devices without any media autodetection, treat success
	 * here as an active link.
	 */
	if (un->un_ops->uno_statchg == NULL) {
		mutex_enter(&unp->unp_miilock);
		usbnet_set_link(un, error == 0);
		mutex_exit(&unp->unp_miilock);
	}

	return error;
}

/* MII management. */

static int
usbnet_mii_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = device_private(dev);
	int err;

	/* MII layer ensures miilock is held. */
	usbnet_isowned_mii(un);

	if (usbnet_isdying(un)) {
		return EIO;
	}

	err = uno_read_reg(un, phy, reg, val);
	if (err) {
		USBNETHIST_CALLARGS("%jd: read PHY failed: %jd",
		    un->un_pri->unp_number, err, 0, 0);
		return err;
	}

	return 0;
}

static int
usbnet_mii_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = device_private(dev);
	int err;

	/* MII layer ensures miilock is held. */
	usbnet_isowned_mii(un);

	if (usbnet_isdying(un)) {
		return EIO;
	}

	err = uno_write_reg(un, phy, reg, val);
	if (err) {
		USBNETHIST_CALLARGS("%jd: write PHY failed: %jd",
		    un->un_pri->unp_number, err, 0, 0);
		return err;
	}

	return 0;
}

static void
usbnet_mii_statchg(struct ifnet *ifp)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;

	/* MII layer ensures miilock is held. */
	usbnet_isowned_mii(un);

	uno_mii_statchg(un, ifp);
}

static int
usbnet_media_upd(struct ifnet *ifp)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	struct usbnet_private * const unp = un->un_pri;
	struct mii_data * const mii = usbnet_mii(un);

	/* ifmedia layer ensures miilock is held. */
	usbnet_isowned_mii(un);

	/* ifmedia changes only with IFNET_LOCK held.  */
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	if (usbnet_isdying(un))
		return EIO;

	unp->unp_link = false;

	if (mii->mii_instance) {
		struct mii_softc *miisc;

		LIST_FOREACH(miisc, &mii->mii_phys, mii_list)
			mii_phy_reset(miisc);
	}

	return ether_mediachange(ifp);
}

/* ioctl */

static int
usbnet_ifflags_cb(struct ethercom *ec)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct ifnet *ifp = &ec->ec_if;
	struct usbnet *un = ifp->if_softc;
	struct usbnet_private * const unp = un->un_pri;
	int rv = 0;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	const u_short changed = ifp->if_flags ^ unp->unp_if_flags;
	if ((changed & ~(IFF_CANTCHANGE | IFF_DEBUG)) == 0) {
		mutex_enter(&unp->unp_mcastlock);
		unp->unp_if_flags = ifp->if_flags;
		mutex_exit(&unp->unp_mcastlock);
		/*
		 * XXX Can we just do uno_mcast synchronously here
		 * instead of resetting the whole interface?
		 *
		 * Not yet, because some usbnet drivers (e.g., aue(4))
		 * initialize the hardware differently in uno_init
		 * depending on IFF_PROMISC.  But some (again, aue(4))
		 * _also_ need to know whether IFF_PROMISC is set in
		 * uno_mcast and do something different with it there.
		 * Maybe the logic can be unified, but it will require
		 * an audit and testing of all the usbnet drivers.
		 */
		if (changed & IFF_PROMISC)
			rv = ENETRESET;
	} else {
		rv = ENETRESET;
	}

	return rv;
}

bool
usbnet_ispromisc(struct usbnet *un)
{
	struct ifnet * const ifp = usbnet_ifp(un);
	struct usbnet_private * const unp = un->un_pri;

	KASSERTMSG(mutex_owned(&unp->unp_mcastlock) || IFNET_LOCKED(ifp),
	    "%s", ifp->if_xname);

	return unp->unp_if_flags & IFF_PROMISC;
}

static int
usbnet_if_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = ifp->if_softc;
	struct usbnet_private * const unp __unused = un->un_pri;
	int error;

	USBNETHIST_CALLARGSN(11, "%jd: enter %#jx data %#jx",
	    unp->unp_number, cmd, (uintptr_t)data, 0);

	if (un->un_ops->uno_override_ioctl)
		return uno_override_ioctl(un, ifp, cmd, data);

	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) {
		switch (cmd) {
		case SIOCADDMULTI:
		case SIOCDELMULTI:
			/*
			 * If there's a hardware multicast filter, and
			 * it has been programmed by usbnet_init_rx_tx
			 * and is active, update it now.  Otherwise,
			 * drop the update on the floor -- it will be
			 * observed by usbnet_init_rx_tx next time we
			 * bring the interface up.
			 */
			if (un->un_ops->uno_mcast) {
				mutex_enter(&unp->unp_mcastlock);
				if (unp->unp_mcastactive)
					(*un->un_ops->uno_mcast)(ifp);
				mutex_exit(&unp->unp_mcastlock);
			}
			error = 0;
			break;
		default:
			error = uno_ioctl(un, ifp, cmd, data);
		}
	}

	return error;
}

/*
 * Generic stop network function:
 *	- mark as stopping
 *	- call DD routine to stop the device
 *	- turn off running, timer, statchg callout, link
 *	- stop transfers
 *	- free RX and TX resources
 *	- close pipes
 *
 * usbnet_if_stop() is for the if_stop handler.
 */
static void
usbnet_stop(struct usbnet *un, struct ifnet *ifp, int disable)
{
	struct usbnet_private * const unp = un->un_pri;
	struct mii_data * const mii = usbnet_mii(un);

	USBNETHIST_FUNC(); USBNETHIST_CALLED();

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	/*
	 * For drivers with hardware multicast filter update callbacks:
	 * Prevent concurrent access to the hardware registers by
	 * multicast filter updates, which happens without IFNET_LOCK.
	 */
	if (un->un_ops->uno_mcast) {
		mutex_enter(&unp->unp_mcastlock);
		unp->unp_mcastactive = false;
		mutex_exit(&unp->unp_mcastlock);
	}

	/*
	 * Prevent new activity (rescheduling ticks, xfers, &c.) and
	 * clear the watchdog timer.
	 */
	mutex_enter(&unp->unp_miilock);
	unp->unp_stopped = true;
	mutex_exit(&unp->unp_miilock);

	mutex_enter(&unp->unp_rxlock);
	unp->unp_rxstopped = true;
	mutex_exit(&unp->unp_rxlock);

	mutex_enter(&unp->unp_txlock);
	unp->unp_txstopped = true;
	unp->unp_timer = 0;
	mutex_exit(&unp->unp_txlock);

	/*
	 * Stop the timer first, then the task -- if the timer was
	 * already firing, we stop the task or wait for it complete
	 * only after it last fired.  Setting unp_stopped prevents the
	 * timer task from being scheduled again.
	 */
	callout_halt(&unp->unp_stat_ch, NULL);
	usb_rem_task_wait(un->un_udev, &unp->unp_ticktask, USB_TASKQ_DRIVER,
	    NULL);

	/*
	 * Now that we have stopped calling mii_tick, bring the MII
	 * state machine down.
	 */
	if (mii) {
		mutex_enter(&unp->unp_miilock);
		mii_down(mii);
		mutex_exit(&unp->unp_miilock);
	}

	/* Stop transfers. */
	usbnet_ep_stop_pipes(un);

	/*
	 * Now that the software is quiescent, ask the driver to stop
	 * the hardware.  The driver's uno_stop routine now has
	 * exclusive access to any registers that might previously have
	 * been used by to ifmedia, mii, or ioctl callbacks.
	 *
	 * Don't bother if the device is being detached, though -- if
	 * it's been unplugged then there's no point in trying to touch
	 * the registers.
	 */
	if (!usbnet_isdying(un))
		uno_stop(un, ifp, disable);

	/* Free RX/TX resources. */
	usbnet_rx_list_fini(un);
	usbnet_tx_list_fini(un);

	/* Close pipes. */
	usbnet_ep_close_pipes(un);

	/* Everything is quesced now. */
	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);
	ifp->if_flags &= ~IFF_RUNNING;
}

static void
usbnet_if_stop(struct ifnet *ifp, int disable)
{
	struct usbnet * const un = ifp->if_softc;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	/*
	 * If we're already stopped, nothing to do.
	 *
	 * XXX This should be an assertion, but it may require some
	 * analysis -- and possibly some tweaking -- of sys/net to
	 * ensure.
	 */
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	usbnet_stop(un, ifp, disable);
}

/*
 * Generic tick task function.
 *
 * usbnet_tick() is triggered from a callout, and triggers a call to
 * usbnet_tick_task() from the usb_task subsystem.
 */
static void
usbnet_tick(void *arg)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = arg;
	struct usbnet_private * const unp = un->un_pri;

	USBNETHIST_CALLARGSN(10, "%jd: enter", unp->unp_number, 0, 0, 0);

	/* Perform periodic stuff in process context */
	usb_add_task(un->un_udev, &unp->unp_ticktask, USB_TASKQ_DRIVER);
}

static void
usbnet_watchdog(struct ifnet *ifp)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	struct usbnet_private * const unp = un->un_pri;
	struct usbnet_cdata * const cd = un_cdata(un);

	if_statinc(ifp, if_oerrors);
	device_printf(un->un_dev, "watchdog timeout\n");

	if (cd->uncd_tx_cnt > 0) {
		DPRINTF("uncd_tx_cnt=%ju non zero, aborting pipe", 0, 0, 0, 0);
		usbd_abort_pipe(unp->unp_ep[USBNET_ENDPT_TX]);
		if (cd->uncd_tx_cnt != 0)
			DPRINTF("uncd_tx_cnt now %ju", cd->uncd_tx_cnt, 0, 0, 0);
	}

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		(*ifp->if_start)(ifp);
}

static void
usbnet_tick_task(void *arg)
{
	USBNETHIST_FUNC();
	struct usbnet * const un = arg;
	struct usbnet_private * const unp = un->un_pri;
	struct ifnet * const ifp = usbnet_ifp(un);
	struct mii_data * const mii = usbnet_mii(un);

	USBNETHIST_CALLARGSN(8, "%jd: enter", unp->unp_number, 0, 0, 0);

	mutex_enter(&unp->unp_txlock);
	const bool timeout = unp->unp_timer != 0 && --unp->unp_timer == 0;
	mutex_exit(&unp->unp_txlock);
	if (timeout)
		usbnet_watchdog(ifp);

	/* Call driver if requested. */
	uno_tick(un);

	mutex_enter(&unp->unp_miilock);
	DPRINTFN(8, "mii %#jx ifp %#jx", (uintptr_t)mii, (uintptr_t)ifp, 0, 0);
	if (mii) {
		mii_tick(mii);
		if (!unp->unp_link)
			(*mii->mii_statchg)(ifp);
	}

	if (!unp->unp_stopped && !usbnet_isdying(un))
		callout_schedule(&unp->unp_stat_ch, hz);
	mutex_exit(&unp->unp_miilock);
}

static int
usbnet_if_init(struct ifnet *ifp)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = ifp->if_softc;
	int error;

	KASSERTMSG(IFNET_LOCKED(ifp), "%s", ifp->if_xname);

	/*
	 * Prevent anyone from bringing the interface back up once
	 * we're detaching.
	 */
	if (usbnet_isdying(un))
		return EIO;

	/*
	 * If we're already running, nothing to do.
	 *
	 * XXX This should be an assertion, but it may require some
	 * analysis -- and possibly some tweaking -- of sys/net to
	 * ensure.
	 */
	if (ifp->if_flags & IFF_RUNNING)
		return 0;

	error = uno_init(un, ifp);
	if (error)
		return error;
	error = usbnet_init_rx_tx(un);
	if (error)
		return error;

	return 0;
}


/* Various accessors. */

void
usbnet_set_link(struct usbnet *un, bool link)
{
	usbnet_isowned_mii(un);
	un->un_pri->unp_link = link;
}

struct ifnet *
usbnet_ifp(struct usbnet *un)
{
	return &un->un_pri->unp_ec.ec_if;
}

struct ethercom *
usbnet_ec(struct usbnet *un)
{
	return &un->un_pri->unp_ec;
}

struct mii_data *
usbnet_mii(struct usbnet *un)
{
	return un->un_pri->unp_ec.ec_mii;
}

krndsource_t *
usbnet_rndsrc(struct usbnet *un)
{
	return &un->un_pri->unp_rndsrc;
}

void *
usbnet_softc(struct usbnet *un)
{
	return un->un_sc;
}

bool
usbnet_havelink(struct usbnet *un)
{
	return un->un_pri->unp_link;
}

bool
usbnet_isdying(struct usbnet *un)
{
	return atomic_load_relaxed(&un->un_pri->unp_dying);
}


/* Locking. */

static void
usbnet_isowned_rx(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_pri->unp_rxlock));
}

static void
usbnet_isowned_tx(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_pri->unp_txlock));
}

/* Autoconf management. */

static bool
usbnet_empty_eaddr(struct usbnet * const un)
{
	return (un->un_eaddr[0] == 0 && un->un_eaddr[1] == 0 &&
		un->un_eaddr[2] == 0 && un->un_eaddr[3] == 0 &&
		un->un_eaddr[4] == 0 && un->un_eaddr[5] == 0);
}

/*
 * usbnet_attach() and usbnet_attach_ifp() perform setup of the relevant
 * 'usbnet'.  The first is enough to enable device access (eg, endpoints
 * are connected and commands can be sent), and the second connects the
 * device to the system networking.
 *
 * Always call usbnet_detach(), even if usbnet_attach_ifp() is skipped.
 * Also usable as driver detach directly.
 *
 * To skip ethernet configuration (eg, point-to-point), make sure that
 * the un_eaddr[] is fully zero.
 */

void
usbnet_attach(struct usbnet *un)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();

	/* Required inputs.  */
	KASSERT(un->un_ops->uno_tx_prepare);
	KASSERT(un->un_ops->uno_rx_loop);
	KASSERT(un->un_rx_bufsz);
	KASSERT(un->un_tx_bufsz);
	KASSERT(un->un_rx_list_cnt);
	KASSERT(un->un_tx_list_cnt);

	/* Unfortunate fact.  */
	KASSERT(un == device_private(un->un_dev));

	un->un_pri = kmem_zalloc(sizeof(*un->un_pri), KM_SLEEP);
	struct usbnet_private * const unp = un->un_pri;

	usb_init_task(&unp->unp_ticktask, usbnet_tick_task, un,
	    USB_TASKQ_MPSAFE);
	callout_init(&unp->unp_stat_ch, CALLOUT_MPSAFE);
	callout_setfunc(&unp->unp_stat_ch, usbnet_tick, un);

	mutex_init(&unp->unp_txlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&unp->unp_rxlock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&unp->unp_miilock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&unp->unp_mcastlock, MUTEX_DEFAULT, IPL_SOFTCLOCK);

	rnd_attach_source(&unp->unp_rndsrc, device_xname(un->un_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	usbnet_rx_list_alloc(un);
	usbnet_tx_list_alloc(un);

	unp->unp_number = atomic_inc_uint_nv(&usbnet_number);

	unp->unp_stopped = true;
	unp->unp_rxstopped = true;
	unp->unp_txstopped = true;
	unp->unp_attached = true;
}

static void
usbnet_attach_mii(struct usbnet *un, const struct usbnet_mii *unm)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet_private * const unp = un->un_pri;
	struct mii_data * const mii = &unp->unp_mii;
	struct ifnet * const ifp = usbnet_ifp(un);

	KASSERT(un->un_ops->uno_read_reg);
	KASSERT(un->un_ops->uno_write_reg);
	KASSERT(un->un_ops->uno_statchg);

	mii->mii_ifp = ifp;
	mii->mii_readreg = usbnet_mii_readreg;
	mii->mii_writereg = usbnet_mii_writereg;
	mii->mii_statchg = usbnet_mii_statchg;
	mii->mii_flags = MIIF_AUTOTSLEEP;

	usbnet_ec(un)->ec_mii = mii;
	ifmedia_init_with_lock(&mii->mii_media, 0,
	    usbnet_media_upd, ether_mediastatus, &unp->unp_miilock);
	mii_attach(un->un_dev, mii, unm->un_mii_capmask, unm->un_mii_phyloc,
	    unm->un_mii_offset, unm->un_mii_flags);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);
}

void
usbnet_attach_ifp(struct usbnet *un,
		  unsigned if_flags,		/* additional if_flags */
		  unsigned if_extflags,		/* additional if_extflags */
		  const struct usbnet_mii *unm)	/* additional mii_attach flags */
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet_private * const unp = un->un_pri;
	struct ifnet * const ifp = usbnet_ifp(un);

	KASSERT(unp->unp_attached);
	KASSERT(!unp->unp_ifp_attached);

	ifp->if_softc = un;
	strlcpy(ifp->if_xname, device_xname(un->un_dev), IFNAMSIZ);
	ifp->if_flags = if_flags;
	ifp->if_extflags = IFEF_MPSAFE | if_extflags;
	ifp->if_ioctl = usbnet_if_ioctl;
	ifp->if_start = usbnet_if_start;
	ifp->if_init = usbnet_if_init;
	ifp->if_stop = usbnet_if_stop;

	if (unm)
		usbnet_attach_mii(un, unm);
	else
		unp->unp_link = true;

	/* Attach the interface. */
	if_initialize(ifp);
	if (ifp->_if_input == NULL)
		ifp->if_percpuq = if_percpuq_create(ifp);
	if_register(ifp);
	unp->unp_ifp_attached = true;

	/*
	 * If ethernet address is all zero, skip ether_ifattach() and
	 * instead attach bpf here..
	 */
	if (!usbnet_empty_eaddr(un)) {
		ether_set_ifflags_cb(&unp->unp_ec, usbnet_ifflags_cb);
		aprint_normal_dev(un->un_dev, "Ethernet address %s\n",
		    ether_sprintf(un->un_eaddr));
		ether_ifattach(ifp, un->un_eaddr);
	} else {
		if_alloc_sadl(ifp);
		bpf_attach(ifp, DLT_RAW, 0);
	}

	/* Now ready, and attached. */
	IFQ_SET_READY(&ifp->if_snd);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, un->un_udev, un->un_dev);

	if (!pmf_device_register(un->un_dev, NULL, NULL))
		aprint_error_dev(un->un_dev, "couldn't establish power handler\n");
}

int
usbnet_detach(device_t self, int flags)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = device_private(self);
	struct usbnet_private * const unp = un->un_pri;

	/* Detached before attached finished, so just bail out. */
	if (unp == NULL || !unp->unp_attached)
		return 0;

	struct ifnet * const ifp = usbnet_ifp(un);
	struct mii_data * const mii = usbnet_mii(un);

	/*
	 * Prevent new activity.  After we stop the interface, it
	 * cannot be brought back up.
	 */
	atomic_store_relaxed(&unp->unp_dying, true);

	/*
	 * If we're still running on the network, stop and wait for all
	 * asynchronous activity to finish.
	 *
	 * If usbnet_attach_ifp never ran, IFNET_LOCK won't work, but
	 * no activity is possible, so just skip this part.
	 */
	if (unp->unp_ifp_attached) {
		IFNET_LOCK(ifp);
		if (ifp->if_flags & IFF_RUNNING) {
			usbnet_if_stop(ifp, 1);
		}
		IFNET_UNLOCK(ifp);
	}

	/*
	 * The callout and tick task can't be scheduled anew at this
	 * point, and usbnet_if_stop has waited for them to complete.
	 */
	KASSERT(!callout_pending(&unp->unp_stat_ch));
	KASSERT(!usb_task_pending(un->un_udev, &unp->unp_ticktask));

	if (mii) {
		mii_detach(mii, MII_PHY_ANY, MII_OFFSET_ANY);
		ifmedia_fini(&mii->mii_media);
	}
	if (unp->unp_ifp_attached) {
		if (!usbnet_empty_eaddr(un))
			ether_ifdetach(ifp);
		else
			bpf_detach(ifp);
		if_detach(ifp);
	}
	usbnet_ec(un)->ec_mii = NULL;

	usbnet_rx_list_free(un);
	usbnet_tx_list_free(un);

	rnd_detach_source(&unp->unp_rndsrc);

	mutex_destroy(&unp->unp_mcastlock);
	mutex_destroy(&unp->unp_miilock);
	mutex_destroy(&unp->unp_rxlock);
	mutex_destroy(&unp->unp_txlock);

	callout_destroy(&unp->unp_stat_ch);

	pmf_device_deregister(un->un_dev);

	/*
	 * Notify userland that we're going away, if we arrived in the
	 * first place.
	 */
	if (unp->unp_ifp_attached) {
		usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, un->un_udev,
		    un->un_dev);
	}

	kmem_free(unp, sizeof(*unp));
	un->un_pri = NULL;

	return 0;
}

int
usbnet_activate(device_t self, devact_t act)
{
	USBNETHIST_FUNC(); USBNETHIST_CALLED();
	struct usbnet * const un = device_private(self);
	struct usbnet_private * const unp = un->un_pri;
	struct ifnet * const ifp = usbnet_ifp(un);

	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(ifp);

		atomic_store_relaxed(&unp->unp_dying, true);

		mutex_enter(&unp->unp_miilock);
		unp->unp_stopped = true;
		mutex_exit(&unp->unp_miilock);

		mutex_enter(&unp->unp_rxlock);
		unp->unp_rxstopped = true;
		mutex_exit(&unp->unp_rxlock);

		mutex_enter(&unp->unp_txlock);
		unp->unp_txstopped = true;
		mutex_exit(&unp->unp_txlock);

		return 0;
	default:
		return EOPNOTSUPP;
	}
}

MODULE(MODULE_CLASS_MISC, usbnet, NULL);

static int
usbnet_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	case MODULE_CMD_STAT:
	case MODULE_CMD_AUTOUNLOAD:
	default:
		return ENOTTY;
	}
}
