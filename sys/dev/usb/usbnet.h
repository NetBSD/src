/*	$NetBSD: usbnet.h,v 1.8 2019/08/09 02:14:35 mrg Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _DEV_USB_USBNET_H
#define _DEV_USB_USBNET_H

/*
 * Common code/data shared by all USB ethernet drivers (using these routines.)
 *
 * This framework provides the following features for USB ethernet drivers:
 *
 * - USB endpoint pipe handling
 * - rx and tx chain handling
 * - generic handlers or support for several struct ifnet callbacks
 * - MII bus locking
 * - interrupt handling
 * - partial autoconf handling
 *
 * Consumers of this interface need to:
 *
 * - replace most softc members with "struct usbnet" usage, in particular
 *   use usbnet pointer for ifp->if_softc, and device_private (real softc
 *   can be stored in un_sc member)
 * - use MII bus lock / access methods
 * - usbnet_attach() to initialise and allocate rx/tx chains
 * - usbnet_attach_ifp() to attach the interface, and either ether_ifattach()
 *   for ethernet devices, or if_alloc_sadl()/bpf_attach() pair otherwise.
 * - usbnet_detach() to clean them up
 * - usbnet_activate() for autoconf
 * - interface ioctl and start have direct frontends with callbacks for
 *   device specific handling:
 *   - ioctl can use either a device-specific override (useful for special
 *     cases), but provides a normal handler with callback to handle
 *     ENETRESET conditions that should be sufficient for most users
 *   - start uses usbnet send callback
 * - interface init and stop have helper functions
 *   - device specific init should use usbnet_init_rx_tx() to open pipes
 *     to the device and setup the rx/tx chains for use after any device
 *     specific setup
 *   - usbnet_stop() must be called with the un_lock held, and will
 *     call the device-specific usbnet stop callback, which enables the
 *     standard init calls stop idiom.
 * - interrupt handling:
 *   - for rx, usbnet_init_rx_tx() will enable the receive pipes and
 *     call the rx_loop callback to handle device specific processing of
 *     packets, which can use usbnet_enqueue() to provide data to the
 *     higher layers
 *   - for tx, usbnet_start (if_start) will pull entries out of the
 *     transmit queue and use the send callback for the given mbuf.
 *     the usb callback will use usbnet_txeof() for the transmit
 *     completion function (internal to usbnet)
 */

#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

/*
 * Per-transfer data, initialised in usbnet_[rt]x_list_init().
 *
 * Front-end must set uncd_tx_list_cnt and uncd_rx_list_cnt before calling
 * list init, which will allocate the chain arrays, and must be NULL to
 * indicate the first call.
 */
struct usbnet;
struct usbnet_chain {
	struct usbnet		*unc_un;
	struct usbd_xfer	*unc_xfer;
	uint8_t			*unc_buf;
};

struct usbnet_cdata {
	struct usbnet_chain	*uncd_tx_chain;
	struct usbnet_chain	*uncd_rx_chain;

	unsigned		uncd_rx_bufsz;
	unsigned		uncd_tx_bufsz;
	unsigned		uncd_rx_list_cnt;
	unsigned		uncd_tx_list_cnt;

	int			uncd_rx_xfer_flags;
	int			uncd_tx_xfer_flags;

	int			uncd_tx_prod;
	int			uncd_tx_cnt;
	int			uncd_rx_cnt;
};

/*
 * Extend this as necessary.  axe(4) claims to want 6 total, but
 * does not implement them.
 */
enum usbnet_ep {
	USBNET_ENDPT_RX,
	USBNET_ENDPT_TX,
	USBNET_ENDPT_INTR,
	USBNET_ENDPT_MAX,
};

/* Interface stop callback. */
typedef void (*usbnet_stop_cb)(struct ifnet *, int);
/* Interface ioctl callback. */
typedef int (*usbnet_ioctl_cb)(struct ifnet *, u_long, void *);
/* Initialise device callback. */
typedef int (*usbnet_init_cb)(struct ifnet *);

/* MII read register callback. */
typedef usbd_status (*usbnet_mii_read_reg_cb)(struct usbnet *, int reg,
					      int phy, uint16_t *val);
/* MII write register callback. */
typedef usbd_status (*usbnet_mii_write_reg_cb)(struct usbnet *, int reg,
					       int phy, uint16_t val);
/* MII status change callback. */
typedef void (*usbnet_mii_statchg_cb)(struct ifnet *);

/* Prepare packet to send callback, returns length. */
typedef unsigned (*usbnet_tx_prepare_cb)(struct usbnet *, struct mbuf *,
					 struct usbnet_chain *);
/* Receive some packets callback. */
typedef void (*usbnet_rx_loop_cb)(struct usbnet *, struct usbd_xfer *,
			          struct usbnet_chain *, uint32_t);
/* Interrupt pipe callback. */
typedef void (*usbnet_intr_cb)(struct usbnet *, usbd_status);

struct usbnet_ops {
	usbnet_stop_cb		uno_stop;
	usbnet_ioctl_cb		uno_ioctl;
	usbnet_ioctl_cb		uno_override_ioctl;
	usbnet_init_cb		uno_init;
	usbnet_mii_read_reg_cb	uno_read_reg;
	usbnet_mii_write_reg_cb uno_write_reg;
	usbnet_mii_statchg_cb	uno_statchg;
	usbnet_tx_prepare_cb	uno_tx_prepare;
	usbnet_rx_loop_cb	uno_rx_loop;
	usbnet_intr_cb		uno_intr;
};

/*
 * Generic USB ethernet structure.  Use this as ifp->if_softc and
 * set as device_private() in attach.
 *
 * Devices without MII should call usbnet_attach_ifp() with have_mii set
 * to true, and should ensure that the un_statchg_cb callback sets the
 * un_link member.  Devices without MII have this forced to true.
 */
struct usbnet {
	void			*un_sc;			/* real softc */
	device_t		un_dev;
	struct usbd_interface	*un_iface;
	struct usbd_device *	un_udev;

	krndsource_t		un_rndsrc;
	struct usbnet_ops	*un_ops;

	struct ethercom		un_ec;
	struct mii_data		un_mii;
	int			un_phyno;
	uint8_t			un_eaddr[ETHER_ADDR_LEN];

	enum usbnet_ep		un_ed[USBNET_ENDPT_MAX];
	struct usbd_pipe	*un_ep[USBNET_ENDPT_MAX];

	struct usbnet_cdata	un_cdata;
	struct callout		un_stat_ch;
	int			un_if_flags;

	/* This is for driver to use. */
	unsigned		un_flags;

	/*
	 * - un_lock protects most of the structure
	 * - un_miilock must be held to access this device's MII bus
	 * - un_rxlock protects the rx path and its data
	 * - un_txlock protects the tx path and its data
	 * - un_detachcv handles detach vs open references
	 */
	kmutex_t		un_lock;
	kmutex_t		un_miilock;
	kmutex_t		un_rxlock;
	kmutex_t		un_txlock;
	kcondvar_t		un_detachcv;

	struct usb_task		un_ticktask;

	int			un_refcnt;
	int			un_timer;

	bool			un_dying;
	bool			un_stopping;
	bool			un_link;
	bool			un_attached;

	struct timeval		un_rx_notice;
	struct timeval		un_tx_notice;
	struct timeval		un_intr_notice;

	/*
	 * if un_intr_buf is not NULL, use usbd_open_pipe_intr() not
	 * usbd_open_pipe() for USBNET_ENDPT_INTR, with this buffer,
	 * size, and interval.
	 */
	void			*un_intr_buf;
	unsigned		un_intr_bufsz;
	unsigned		un_intr_interval;
};

#define usbnet_ifp(un)		(&(un)->un_ec.ec_if)
#define usbnet_ec(un)		(&(un)->un_ec)
#define usbnet_rndsrc(un)	(&(un)->un_rndsrc)
#define usbnet_mii(un)		((un)->un_ec.ec_mii)
#define usbnet_softc(un)	((un)->un_sc)
#define usbnet_isdying(un)	((un)->un_dying)

/*
 * Endpoint / rx/tx chain management:
 *
 * usbnet_attach() allocates rx and tx chains
 * usbnet_init_rx_tx() open pipes, initialises the rx/tx chains for use
 * usbnet_stop() stops pipes, cleans (not frees) rx/tx chains, locked
 *                version assumes un_lock is held
 * usbnet_detach() frees the rx/tx chains
 *
 * Setup un_ed[] with valid end points before calling usbnet_init_rx_tx().
 * Will return with un_ep[] initialised upon success.
 */
int	usbnet_init_rx_tx(struct usbnet * const);

/* locking */
static __inline__ void
usbnet_lock(struct usbnet *un)
{
	mutex_enter(&un->un_lock);
}

static __inline__ void
usbnet_unlock(struct usbnet *un)
{
	mutex_exit(&un->un_lock);
}

static __inline__ void
usbnet_isowned(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_lock));
}

static __inline__ void
usbnet_lock_rx(struct usbnet *un)
{
	mutex_enter(&un->un_rxlock);
}

static __inline__ void
usbnet_unlock_rx(struct usbnet *un)
{
	mutex_exit(&un->un_rxlock);
}

static __inline__ void
usbnet_isowned_rx(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_rxlock));
}

static __inline__ void
usbnet_lock_tx(struct usbnet *un)
{
	mutex_enter(&un->un_txlock);
}

static __inline__ void
usbnet_unlock_tx(struct usbnet *un)
{
	mutex_exit(&un->un_txlock);
}

static __inline__ void
usbnet_isowned_tx(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_txlock));
}

/* mii */
void	usbnet_lock_mii(struct usbnet *);
void	usbnet_lock_mii_un_locked(struct usbnet *);
void	usbnet_unlock_mii(struct usbnet *);
void	usbnet_unlock_mii_un_locked(struct usbnet *);

static __inline__ void
usbnet_isowned_mii(struct usbnet *un)
{
	KASSERT(mutex_owned(&un->un_miilock));
}

int	usbnet_mii_readreg(device_t, int, int, uint16_t *);
int	usbnet_mii_writereg(device_t, int, int, uint16_t);
void	usbnet_mii_statchg(struct ifnet *);

/* interrupt handling */
void	usbnet_enqueue(struct usbnet * const, uint8_t *, size_t, int,
		       uint32_t, int);
void	usbnet_input(struct usbnet * const, uint8_t *, size_t);

/* autoconf */
void	usbnet_attach(struct usbnet *un, const char *, unsigned, unsigned,
		      unsigned, unsigned, unsigned, unsigned);
void	usbnet_attach_ifp(struct usbnet *, bool, unsigned, unsigned, int);
int	usbnet_detach(device_t, int);
int	usbnet_activate(device_t, devact_t);

/* stop backend */
void	usbnet_stop(struct usbnet *, struct ifnet *, int);

#endif /* _DEV_USB_USBNET_H */
