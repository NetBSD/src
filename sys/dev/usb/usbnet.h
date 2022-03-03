/*	$NetBSD: usbnet.h,v 1.26 2022/03/03 05:52:20 riastradh Exp $	*/

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
 *   - start uses usbnet transmit prepare callback (uno_tx_prepare)
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
 *     transmit queue and use the transmit prepare callback (uno_tx_prepare)
 *     for the given mbuf.  the usb callback will use usbnet_txeof() for
 *     the transmit completion function (internal to usbnet)
 *   - there is special interrupt pipe handling
 * - timer/tick:
 *   - the uno_tick callback will be called once a second if present.
 */

#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>
#include <sys/module.h>

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
 * Per-transfer data.
 *
 * Front-end must set un_rx_list_cnt and un_tx_list_cnt before
 * calling usbnet_attach(), and then call usbnet_rx_tx_init()
 * which will allocate the chain arrays, and must be NULL to
 * indicate the first call.
 */
struct usbnet;
struct usbnet_chain {
	struct usbnet		*unc_un;
	struct usbd_xfer	*unc_xfer;
	uint8_t			*unc_buf;
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
/* Reprogram multicast filters callback. */
typedef void (*usbnet_mcast_cb)(struct ifnet *);
/* Initialise device callback. */
typedef int (*usbnet_init_cb)(struct ifnet *);

/* MII read register callback. */
typedef int (*usbnet_mii_read_reg_cb)(struct usbnet *, int reg,
				      int phy, uint16_t *val);
/* MII write register callback. */
typedef int (*usbnet_mii_write_reg_cb)(struct usbnet *, int reg,
				       int phy, uint16_t val);
/* MII status change callback. */
typedef void (*usbnet_mii_statchg_cb)(struct ifnet *);

/* Prepare packet to send callback, returns length. */
typedef unsigned (*usbnet_tx_prepare_cb)(struct usbnet *, struct mbuf *,
					 struct usbnet_chain *);
/* Receive some packets callback. */
typedef void (*usbnet_rx_loop_cb)(struct usbnet *, struct usbnet_chain *,
				  uint32_t);
/* Tick callback. */
typedef void (*usbnet_tick_cb)(struct usbnet *);
/* Interrupt pipe callback. */
typedef void (*usbnet_intr_cb)(struct usbnet *, usbd_status);

/*
 * LOCKING
 * =======
 *
 * The following annotations indicate which locks are held when
 * usbnet_ops functions are invoked:
 *
 * I -> IFNET_LOCK (if_ioctl_lock)
 * C -> CORE_LOCK (usbnet core_lock)
 * T -> TX_LOCK (usbnet tx_lock)
 * R -> RX_LOCK (usbnet rx_lock)
 * n -> no locks held
 *
 * Note that when CORE_LOCK is held, IFNET_LOCK may or may not also
 * be held.
 *
 * Note that the IFNET_LOCK **may not be held** for the ioctl commands
 * SIOCADDMULTI/SIOCDELMULTI.  These commands are only passed
 * explicitly to uno_override_ioctl; for all other devices, they are
 * handled inside usbnet by scheduling a task to asynchronously call
 * uno_mcast with IFNET_LOCK held.
 */
struct usbnet_ops {
	usbnet_stop_cb		uno_stop;		/* C */
	usbnet_ioctl_cb		uno_ioctl;		/* I */
	usbnet_ioctl_cb		uno_override_ioctl;	/* I (except mcast) */
	usbnet_mcast_cb		uno_mcast;		/* I */
	usbnet_init_cb		uno_init;		/* I */
	usbnet_mii_read_reg_cb	uno_read_reg;		/* C */
	usbnet_mii_write_reg_cb uno_write_reg;		/* C */
	usbnet_mii_statchg_cb	uno_statchg;		/* C */
	usbnet_tx_prepare_cb	uno_tx_prepare;		/* T */
	usbnet_rx_loop_cb	uno_rx_loop;		/* R */
	usbnet_tick_cb		uno_tick;		/* n */
	usbnet_intr_cb		uno_intr;		/* n */
};

/*
 * USB interrupt pipe support.  Use this if usbd_open_pipe_intr() should
 * be used for the interrupt pipe.
 */
struct usbnet_intr {
	/*
	 * Point un_intr to this structure to use usbd_open_pipe_intr() not
	 * usbd_open_pipe() for USBNET_ENDPT_INTR, with this buffer, size,
	 * and interval.
	 */
	void			*uni_buf;
	unsigned		uni_bufsz;
	unsigned		uni_interval;
};

/*
 * Structure to setup MII.  Use the USBNET_MII_DECL_DEFAULT() macro for
 * sane default.  Pass a copy to usbnet_attach_ifp().  Not used
 * after the usbnet_attach_ifp() function returns.
 */
struct usbnet_mii {
	int			un_mii_flags;
	int			un_mii_capmask;
	int			un_mii_phyloc;
	int			un_mii_offset;
};

#define USBNET_MII_DECL(name, capmask, loc, off, flags)	\
	struct usbnet_mii name = {			\
		.un_mii_capmask = capmask,		\
		.un_mii_phyloc = loc,			\
		.un_mii_offset = off,			\
		.un_mii_flags = flags,			\
	}
#define USBNET_MII_DECL_DEFAULT(name)				\
	USBNET_MII_DECL(name, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0)

/*
 * Generic USB ethernet structure.  Use this as ifp->if_softc and set as
 * device_private() in attach unless already using struct usbnet here.
 *
 * Devices without MII should call usbnet_attach_ifp() with have_mii set
 * to true, and should ensure that the un_statchg_cb callback sets the
 * un_link member.  Devices without MII have this forced to true.
 */
struct usbnet_private;
struct usbnet {
	/*
	 * This section should be filled in before calling
	 * usbnet_attach().
	 */
	void			*un_sc;			/* real softc */
	device_t		un_dev;
	struct usbd_interface	*un_iface;
	struct usbd_device	*un_udev;
	const struct usbnet_ops	*un_ops;
	struct usbnet_intr	*un_intr;

	/* Inputs for rx/tx chain control. */
	unsigned		un_rx_bufsz;
	unsigned		un_tx_bufsz;
	unsigned		un_rx_list_cnt;
	unsigned		un_tx_list_cnt;
	int			un_rx_xfer_flags;
	int			un_tx_xfer_flags;

	/*
	 * This section should be filled in before calling
	 * usbnet_attach_ifp().
	 *
	 * XXX This should be of type "uByte".  enum usbnet_ep
	 * is the index.  Fix this in a kernel version bump.
	 */
	enum usbnet_ep		un_ed[USBNET_ENDPT_MAX];

	/* MII specific. Not used without MII. */
	int			un_phyno;
	/* Ethernet specific. All zeroes indicates non-Ethernet. */
	uint8_t			un_eaddr[ETHER_ADDR_LEN];

	/*
	 * This section is for driver to use, not touched by usbnet.
	 */
	unsigned		un_flags;

	/*
	 * This section is private to usbnet. Don't touch.
	 */
	struct usbnet_private	*un_pri;
};

/* Various accessors. */

void usbnet_set_link(struct usbnet *, bool);

struct ifnet *usbnet_ifp(struct usbnet *);
struct ethercom *usbnet_ec(struct usbnet *);
struct mii_data *usbnet_mii(struct usbnet *);
krndsource_t *usbnet_rndsrc(struct usbnet *);
void *usbnet_softc(struct usbnet *);

bool usbnet_havelink(struct usbnet *);
bool usbnet_isdying(struct usbnet *);


/*
 * Locking.  Note that the isowned() are implemented here so that
 * empty-KASSERT() causes them to be elided for non-DIAG builds.
 */
void	usbnet_lock_core(struct usbnet *);
void	usbnet_unlock_core(struct usbnet *);
kmutex_t *usbnet_mutex_core(struct usbnet *);
static __inline__ void
usbnet_isowned_core(struct usbnet *un)
{
	KASSERT(mutex_owned(usbnet_mutex_core(un)));
}

/*
 * Endpoint / rx/tx chain management:
 *
 * usbnet_attach() initialises usbnet and allocates rx and tx chains
 * usbnet_init_rx_tx() open pipes, initialises the rx/tx chains for use
 * usbnet_stop() stops pipes, cleans (not frees) rx/tx chains, locked
 *               version assumes un_lock is held
 * usbnet_detach() frees the rx/tx chains
 *
 * Setup un_ed[] with valid end points before calling usbnet_attach().
 * Call usbnet_init_rx_tx() to initialise pipes, which will be open
 * upon success.
 */
int	usbnet_init_rx_tx(struct usbnet * const);

/* MII. */
int	usbnet_mii_readreg(device_t, int, int, uint16_t *);
int	usbnet_mii_writereg(device_t, int, int, uint16_t);
void	usbnet_mii_statchg(struct ifnet *);

/* interrupt handling */
void	usbnet_enqueue(struct usbnet * const, uint8_t *, size_t, int,
		       uint32_t, int);
void	usbnet_input(struct usbnet * const, uint8_t *, size_t);

/* autoconf */
void	usbnet_attach(struct usbnet *un, const char *);
void	usbnet_attach_ifp(struct usbnet *, unsigned, unsigned,
			  const struct usbnet_mii *);
int	usbnet_detach(device_t, int);
int	usbnet_activate(device_t, devact_t);

/* stop backend */
void	usbnet_stop(struct usbnet *, struct ifnet *, int);

/* module hook up */

#ifdef _MODULE
#define USBNET_INIT(name)						\
	error = config_init_component(cfdriver_ioconf_##name,		\
	    cfattach_ioconf_##name, cfdata_ioconf_##name);
#define USBNET_FINI(name)						\
	error = config_fini_component(cfdriver_ioconf_##name,		\
	    cfattach_ioconf_##name, cfdata_ioconf_##name);
#else
#define USBNET_INIT(name)
#define USBNET_FINI(name)
#endif

#define USBNET_MODULE(name)						\
									\
MODULE(MODULE_CLASS_DRIVER, if_##name, "usbnet");			\
									\
static int								\
if_##name##_modcmd(modcmd_t cmd, void *aux)				\
{									\
	int error = 0;							\
									\
	switch (cmd) {							\
	case MODULE_CMD_INIT:						\
		USBNET_INIT(name)					\
		return error;						\
	case MODULE_CMD_FINI:						\
		USBNET_FINI(name)					\
		return error;						\
	default:							\
		return ENOTTY;						\
	}								\
}

#endif /* _DEV_USB_USBNET_H */
