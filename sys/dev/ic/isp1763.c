/*	$NetBSD: isp1763.c,v 1.1 2026/10/11 16:09:06 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * ISP1763A USB 2.0 host controller.
 *
 * The chip runs transfer descriptors via 16 slots / queue (its own memory).
 * It's a PIO-only dumb thing, but it's less dumb than, let's say, slhci.
 *
 * Control and bulk run via ATL queue, interrupts via INT queue. There's an
 * internal hub with TT, so full/low speed devices are supported.
 *
 * TODO: Isochronous, some other frontend driver than RapidRoad, extensive
 * testing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isp1763.c,v 1.1 2026/10/11 16:09:06 rkujawa Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbroothub.h>

#include <dev/ic/isp1763reg.h>
#include <dev/ic/isp1763var.h>

#ifdef ISP1763_DEBUG
#define	DPRINTF(...)	printf(__VA_ARGS__)
#else
#define	DPRINTF(...)	__nothing
#endif

#define	ISP1763_POLL_TRIES	1000	/* 1 ms each */
#define	ISP1763_ATL_MEMSIZE	1024
#define	ISP1763_INT_MEMSIZE	256
#define	ISP1763_RETIRE_DELAY	250	/* us, 2 microframes */
#define	ISP1763_SPLIT_RETIRE_DELAY 1000	/* us, full-speed frame */
#define	ISP1763_PORT_RESET_DONE	2	/* ms */
#define	ISP1763_USCS_SPLIT	0x1c
#define	ISP1763_SCRATCH_TEST	0x5aa5
#define	ISP1763_PTD_STATUS_DW	4	/* DW0-DW3 */

enum isp1763hc_phase {
	ISP1763_PHASE_SETUP,
	ISP1763_PHASE_DATA,
	ISP1763_PHASE_STATUS
};

struct isp1763hc_pipe {
	struct usbd_pipe	ip_pipe;
	struct isp1763hc_ptdreq	ip_req;
	struct isp1763hc_queue	*ip_queue;
	bool			ip_toggle;
	bool			ip_ping;
};

struct isp1763hc_xfer {
	struct usbd_xfer	ix_xfer;
	TAILQ_ENTRY(isp1763hc_xfer) ix_wait;
	struct isp1763hc_queue	*ix_queue;
	enum isp1763hc_phase	ix_phase;
	int			ix_slot;
	uint32_t		ix_off;
	uint16_t		ix_round;
	bool			ix_waiting;
	bool			ix_zlp;
};

#define	ISP1763_BUS2SC(bus)	((struct isp1763hc_softc *)(bus)->ub_hcpriv)
#define	ISP1763_PIPE2SC(pipe)	ISP1763_BUS2SC((pipe)->up_dev->ud_bus)
#define	ISP1763_XFER2SC(xfer)	ISP1763_BUS2SC((xfer)->ux_bus)
#define	ISP1763_PIPE2IP(pipe)	((struct isp1763hc_pipe *)(pipe))
#define	ISP1763_XFER2IX(xfer)	((struct isp1763hc_xfer *)(xfer))

static uint32_t	isp1763hc_read4(struct isp1763hc_softc *, bus_size_t);
static void	isp1763hc_write4(struct isp1763hc_softc *, bus_size_t,
		    uint32_t);
static int	isp1763hc_wait4(struct isp1763hc_softc *, bus_size_t,
		    uint32_t, uint32_t);
static void	isp1763hc_set_mode(struct isp1763hc_softc *, uint16_t);
static int	isp1763hc_swreset(struct isp1763hc_softc *, uint8_t, bool);
static void	isp1763hc_mem_clear(struct isp1763hc_softc *);
static void	isp1763hc_queue_init(struct isp1763hc_queue *, bus_size_t,
		    uint16_t, uint16_t, uint16_t, uint16_t);
static int	isp1763hc_chip_reset(struct isp1763hc_softc *);
static int	isp1763hc_chip_start(struct isp1763hc_softc *);
static int	isp1763hc_chip_init(struct isp1763hc_softc *);

static int	isp1763hc_slot_get(struct isp1763hc_queue *);
static void	isp1763hc_slot_take(struct isp1763hc_queue *,
		    struct isp1763hc_xfer *, int);
static void	isp1763hc_slot_put(struct isp1763hc_softc *,
		    struct isp1763hc_queue *, int);
static void	isp1763hc_release(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *);
static void	isp1763hc_ptd_write(struct isp1763hc_softc *,
		    struct isp1763hc_queue *, int, const uint32_t *);
static void	isp1763hc_arm(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *);
static void	isp1763hc_kick(struct isp1763hc_softc *,
		    struct isp1763hc_queue *);
static void	isp1763hc_finish(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *, usbd_status);
static bool	isp1763hc_zlp_owed(struct isp1763hc_xfer *, u_int);
static bool	isp1763hc_round_failed(uint32_t);
static bool	isp1763hc_round_stalled(uint32_t);
static uint32_t	isp1763hc_round_account(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *, const uint32_t *);
static void	isp1763hc_round_next(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *, uint32_t);
static void	isp1763hc_round_done(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *, const uint32_t *);
static void	isp1763hc_scan(struct isp1763hc_softc *,
		    struct isp1763hc_queue *);

static usbd_status isp1763hc_open(struct usbd_pipe *);
static void	isp1763hc_softintr(void *);
static void	isp1763hc_intr_soft(void *);
static void	isp1763hc_poll(struct usbd_bus *);
static struct usbd_xfer *isp1763hc_allocx(struct usbd_bus *, unsigned int);
static void	isp1763hc_freex(struct usbd_bus *, struct usbd_xfer *);
static void	isp1763hc_abortx(struct usbd_xfer *);
static void	isp1763hc_ptd_retire(struct isp1763hc_softc *,
		    struct isp1763hc_xfer *);
static bool	isp1763hc_dying(struct usbd_bus *);
static void	isp1763hc_get_lock(struct usbd_bus *, kmutex_t **);
static int	isp1763hc_roothub_ctrl(struct usbd_bus *,
		    usb_device_request_t *, void *, int);
static int	isp1763hc_roothub_ctrl_locked(struct usbd_bus *,
		    usb_device_request_t *, void *, int);
static int	isp1763hc_roothub_hubdesc(void *, int);
static int	isp1763hc_roothub_port_status(struct isp1763hc_softc *,
		    void *, int);
static int	isp1763hc_roothub_port_clear(struct isp1763hc_softc *, int);
static int	isp1763hc_roothub_port_set(struct isp1763hc_softc *, int);
static void	isp1763hc_portsc_modify(struct isp1763hc_softc *, uint32_t,
		    uint32_t);
static int	isp1763hc_port_reset(struct isp1763hc_softc *);

static usbd_status isp1763hc_root_intr_transfer(struct usbd_xfer *);
static usbd_status isp1763hc_root_intr_start(struct usbd_xfer *);
static void	isp1763hc_root_intr_abort(struct usbd_xfer *);
static void	isp1763hc_root_intr_close(struct usbd_pipe *);
static void	isp1763hc_root_intr_done(struct usbd_xfer *);

static usbd_status isp1763hc_device_transfer(struct usbd_xfer *);
static usbd_status isp1763hc_device_start(struct usbd_xfer *);
static void	isp1763hc_device_abort(struct usbd_xfer *);
static void	isp1763hc_device_close(struct usbd_pipe *);
static void	isp1763hc_device_clear_toggle(struct usbd_pipe *);
static void	isp1763hc_device_done(struct usbd_xfer *);

static const struct usbd_bus_methods isp1763hc_bus_methods = {
	.ubm_open = isp1763hc_open,
	.ubm_softint = isp1763hc_softintr,
	.ubm_dopoll = isp1763hc_poll,
	.ubm_allocx = isp1763hc_allocx,
	.ubm_freex = isp1763hc_freex,
	.ubm_abortx = isp1763hc_abortx,
	.ubm_dying = isp1763hc_dying,
	.ubm_getlock = isp1763hc_get_lock,
	.ubm_rhctrl = isp1763hc_roothub_ctrl,
};

static const struct usbd_pipe_methods isp1763hc_root_intr_methods = {
	.upm_transfer = isp1763hc_root_intr_transfer,
	.upm_start = isp1763hc_root_intr_start,
	.upm_abort = isp1763hc_root_intr_abort,
	.upm_close = isp1763hc_root_intr_close,
	.upm_cleartoggle = isp1763hc_device_clear_toggle,
	.upm_done = isp1763hc_root_intr_done,
};

static const struct usbd_pipe_methods isp1763hc_device_methods = {
	.upm_transfer = isp1763hc_device_transfer,
	.upm_start = isp1763hc_device_start,
	.upm_abort = isp1763hc_device_abort,
	.upm_close = isp1763hc_device_close,
	.upm_cleartoggle = isp1763hc_device_clear_toggle,
	.upm_done = isp1763hc_device_done,
};

static const uint8_t isp1763hc_zero[64];

static uint32_t
isp1763hc_read4(struct isp1763hc_softc *sc, bus_size_t reg)
{
	uint32_t lo;

	lo = ISP_READ2(sc, reg);

	return lo | ((uint32_t)ISP_READ2(sc, reg + 2) << 16);
}

static void
isp1763hc_write4(struct isp1763hc_softc *sc, bus_size_t reg, uint32_t val)
{

	ISP_WRITE2(sc, reg, val & 0xffff);
	ISP_WRITE2(sc, reg + 2, val >> 16);
}

static int
isp1763hc_wait4(struct isp1763hc_softc *sc, bus_size_t reg, uint32_t mask,
    uint32_t want)
{
	int i;

	for (i = 0; i < ISP1763_POLL_TRIES; i++) {
		if ((isp1763hc_read4(sc, reg) & mask) == want)
			return 0;
		delay(1000);
	}

	return ETIMEDOUT;
}

static void
isp1763hc_set_mode(struct isp1763hc_softc *sc, uint16_t extra)
{
	uint16_t mode;

	mode = sc->sc_hwmode | ISP1763_HWMODE_INTFLOCK |
	    ISP1763_HWMODE_INTPOL | extra;

	ISP_WRITE1(sc, ISP1763_HWMODE, mode & 0xff);
	ISP_WRITE2(sc, ISP1763_HWMODE, mode);
}

static int
isp1763hc_swreset(struct isp1763hc_softc *sc, uint8_t bit, bool pulse)
{
	int i;

	ISP_WRITE1(sc, ISP1763_SWRESET, bit);
	ISP_WRITE1(sc, ISP1763_SWRESET, bit);
	if (pulse) {
		delay(10000);
		ISP_WRITE1(sc, ISP1763_SWRESET, 0);
		ISP_WRITE1(sc, ISP1763_SWRESET, 0);
	}

	for (i = 0; i < ISP1763_POLL_TRIES; i++) {
		if (((ISP_READ1(sc, ISP1763_SWRESET) |
		    ISP_READ1(sc, ISP1763_SWRESET)) & bit) == 0)
			return 0;
		delay(1000);
	}

	return ETIMEDOUT;
}

static void
isp1763hc_mem_clear(struct isp1763hc_softc *sc)
{
	uint16_t a;

	for (a = ISP1763_ISO_PTD_BASE; a < ISP1763_MEM_END;
	    a += sizeof(isp1763hc_zero))
		ISP_MEMWRITE(sc, a, isp1763hc_zero, sizeof(isp1763hc_zero));
}

static void
isp1763hc_queue_init(struct isp1763hc_queue *q, bus_size_t regdone,
    uint16_t irq, uint16_t ptdbase, uint16_t membase, uint16_t memsize)
{

	memset(q, 0, sizeof(*q));
	TAILQ_INIT(&q->q_wait);

	q->q_regdone = regdone;
	q->q_regskip = regdone + 2;
	q->q_reglast = regdone + 4;
	q->q_irq = irq;
	q->q_ptdbase = ptdbase;
	q->q_membase = membase;
	q->q_memsize = memsize;
	q->q_skip = 0xffff;
}

static int
isp1763hc_chip_reset(struct isp1763hc_softc *sc)
{
	uint32_t id;

	/* reset everything */
	if (isp1763hc_swreset(sc, ISP1763_SWRESET_ALL, true) != 0 ||
	    isp1763hc_swreset(sc, ISP1763_SWRESET_HC, false) != 0) {
		aprint_error_dev(sc->sc_dev, "software reset timed out\n");
		return ETIMEDOUT;
	}

	ISP_WRITE1(sc, ISP1763_SWRESET, ISP1763_SWRESET_ATX);
	ISP_WRITE1(sc, ISP1763_SWRESET, ISP1763_SWRESET_ATX);
	delay(1000);
	ISP_WRITE1(sc, ISP1763_SWRESET, 0);
	ISP_WRITE1(sc, ISP1763_SWRESET, 0);
	isp1763hc_set_mode(sc, 0);

	id = isp1763hc_read4(sc, ISP1763_CHIPID);
	ISP_WRITE2(sc, ISP1763_SCRATCH, ISP1763_SCRATCH_TEST);
	(void)isp1763hc_read4(sc, ISP1763_CHIPID);

	/* make sure chip is alive */
	if (id != ISP1763_CHIPID_VALUE ||
	    ISP_READ2(sc, ISP1763_SCRATCH) != ISP1763_SCRATCH_TEST) {
		aprint_error_dev(sc->sc_dev, "chip id 0x%08x, bad scratch\n",
		    id);
		return ENXIO;
	}

	isp1763hc_write4(sc, ISP1763_USBCMD, ISP1763_USBCMD_HCRESET);
	if (isp1763hc_wait4(sc, ISP1763_USBCMD, ISP1763_USBCMD_HCRESET,
	    0) != 0) {
		aprint_error_dev(sc->sc_dev, "controller reset timed out\n");
		return ETIMEDOUT;
	}

	return 0;
}

static int
isp1763hc_chip_start(struct isp1763hc_softc *sc)
{

	isp1763hc_set_mode(sc, 0);
	ISP_WRITE2(sc, ISP1763_BUFSTS, 0);
	isp1763hc_mem_clear(sc);

	ISP_WRITE2(sc, ISP1763_ISO_SKIP, 0xffff);
	ISP_WRITE2(sc, ISP1763_INT_SKIP, 0xffff);
	ISP_WRITE2(sc, ISP1763_ATL_SKIP, 0xffff);
	ISP_WRITE2(sc, ISP1763_ISO_LAST, isp1763hc_last_map(0));
	ISP_WRITE2(sc, ISP1763_INT_LAST, isp1763hc_last_map(0));
	ISP_WRITE2(sc, ISP1763_ATL_LAST, isp1763hc_last_map(0));
	(void)ISP_READ2(sc, ISP1763_INT_DONE);
	(void)ISP_READ2(sc, ISP1763_ATL_DONE);

	isp1763hc_write4(sc, ISP1763_USBSTS,
	    ISP1763_USBSTS_PCD | ISP1763_USBSTS_FLR);
	isp1763hc_write4(sc, ISP1763_USBINTR, 0);
	ISP_WRITE2(sc, ISP1763_HCINT, ISP1763_HCINT_ALL);
	ISP_WRITE2(sc, ISP1763_HCINTEN, ISP1763_HCINT_ATL | ISP1763_HCINT_INT);
	ISP_WRITE2(sc, ISP1763_INT_ORMASK, 0xffff);
	ISP_WRITE2(sc, ISP1763_ATL_ORMASK, 0xffff);
	ISP_WRITE2(sc, ISP1763_INT_ANDMASK, 0);
	ISP_WRITE2(sc, ISP1763_ATL_ANDMASK, 0);
	ISP_WRITE2(sc, ISP1763_BUFSTS, ISP1763_BUFSTS_ATL | ISP1763_BUFSTS_INT);

	ISP_WRITE2(sc, ISP1763_OTGCTL_SET, ISP1763_OTGCTL_OTGDIS);
	ISP_WRITE2(sc, ISP1763_OTGCTL_CLR, ISP1763_OTGCTL_SWSELHC);
	ISP_WRITE2(sc, ISP1763_OTGCTL_CLR, ISP1763_OTGCTL_HC2DIS);

	isp1763hc_write4(sc, ISP1763_USBCMD, ISP1763_USBCMD_RS);
	delay(10000);
	isp1763hc_write4(sc, ISP1763_CONFIGFLAG, ISP1763_CONFIGFLAG_CF);
	delay(1000);
	isp1763hc_write4(sc, ISP1763_PORTSC1, ISP1763_PORTSC_PP);
	delay(50000);

	/* internal hub sits on the root port */
	if (isp1763hc_wait4(sc, ISP1763_PORTSC1, ISP1763_PORTSC_ECCS,
	    ISP1763_PORTSC_ECCS) != 0) {
		aprint_error_dev(sc->sc_dev, "internal hub not connected\n");
		return ETIMEDOUT;
	}

	isp1763hc_set_mode(sc, ISP1763_HWMODE_GLINTEN);

	return 0;
}

static int
isp1763hc_chip_init(struct isp1763hc_softc *sc)
{
	int error;

	error = isp1763hc_chip_reset(sc);
	if (error != 0)
		return error;

	return isp1763hc_chip_start(sc);
}

int
isp1763hc_attach(struct isp1763hc_softc *sc)
{
	int error;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_rhlock, MUTEX_DEFAULT, IPL_NONE);

	isp1763hc_queue_init(&sc->sc_atl, ISP1763_ATL_DONE,
	    ISP1763_HCINT_ATL, ISP1763_ATL_PTD_BASE, ISP1763_PAYLOAD_BASE,
	    ISP1763_ATL_MEMSIZE);
	isp1763hc_queue_init(&sc->sc_int, ISP1763_INT_DONE,
	    ISP1763_HCINT_INT, ISP1763_INT_PTD_BASE,
	    ISP1763_PAYLOAD_BASE + ISP1763_NSLOTS * ISP1763_ATL_MEMSIZE,
	    ISP1763_INT_MEMSIZE);

	error = isp1763hc_chip_init(sc);
	if (error != 0) {
		mutex_destroy(&sc->sc_rhlock);
		mutex_destroy(&sc->sc_lock);
		return error;
	}

	sc->sc_xferpool = pool_cache_init(sizeof(struct isp1763hc_xfer), 0,
	    0, 0, "isp1763xfer", NULL, IPL_USB, NULL, NULL, NULL);
	sc->sc_softint = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    isp1763hc_intr_soft, sc);

	sc->sc_bus.ub_methods = &isp1763hc_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct isp1763hc_pipe);
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_bus.ub_usedma = false;
	sc->sc_bus.ub_hcpriv = sc;

	sc->sc_child = config_found(sc->sc_dev, &sc->sc_bus, usbctlprint,
	    CFARGS_NONE);

	ISP_INTR_ENABLE(sc, true);

	return 0;
}

int
isp1763hc_detach(struct isp1763hc_softc *sc, int flags)
{
	int error;

	if (sc->sc_child != NULL) {
		error = config_detach(sc->sc_child, flags);
		if (error != 0)
			return error;
	}

	mutex_enter(&sc->sc_lock);
	sc->sc_dying = true;
	mutex_exit(&sc->sc_lock);

	isp1763hc_shutdown(sc);

	softint_disestablish(sc->sc_softint);
	pool_cache_destroy(sc->sc_xferpool);
	mutex_destroy(&sc->sc_rhlock);
	mutex_destroy(&sc->sc_lock);

	return 0;
}

int
isp1763hc_activate(device_t self, enum devact act)
{
	struct isp1763hc_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
isp1763hc_childdet(device_t self, device_t child)
{
	struct isp1763hc_softc *sc = device_private(self);

	KASSERT(sc->sc_child == child);
	sc->sc_child = NULL;
}

void
isp1763hc_shutdown(struct isp1763hc_softc *sc)
{

	ISP_INTR_ENABLE(sc, false);
	isp1763hc_set_mode(sc, 0);
	isp1763hc_write4(sc, ISP1763_USBCMD, ISP1763_USBCMD_HCRESET);
}

void
isp1763hc_intr(struct isp1763hc_softc *sc)
{

	kpreempt_disable();
	softint_schedule(sc->sc_softint);
	kpreempt_enable();
}

static void
isp1763hc_intr_soft(void *arg)
{
	struct isp1763hc_softc *sc = arg;

	mutex_enter(&sc->sc_lock);
	isp1763hc_softintr(&sc->sc_bus);
	mutex_exit(&sc->sc_lock);
}

static int
isp1763hc_slot_get(struct isp1763hc_queue *q)
{
	int slot;

	for (slot = 0; slot < ISP1763_NSLOTS; slot++) {
		if ((q->q_busy & __BIT(slot)) == 0)
			return slot;
	}

	return -1;
}

static void
isp1763hc_slot_take(struct isp1763hc_queue *q, struct isp1763hc_xfer *ix,
    int slot)
{

	ix->ix_slot = slot;
	q->q_xfer[slot] = ix;
	q->q_busy |= __BIT(slot);
}

static void
isp1763hc_slot_put(struct isp1763hc_softc *sc, struct isp1763hc_queue *q,
    int slot)
{

	q->q_xfer[slot] = NULL;
	q->q_busy &= ~__BIT(slot);
	q->q_skip |= __BIT(slot);
	if (sc->sc_dying)
		return;

	ISP_WRITE2(sc, q->q_regskip, q->q_skip);
	ISP_WRITE2(sc, q->q_reglast, isp1763hc_last_map(q->q_busy));
}

static void
isp1763hc_release(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix)
{
	struct isp1763hc_queue *q = ix->ix_queue;

	isp1763hc_slot_put(sc, q, ix->ix_slot);
	ix->ix_slot = -1;
	isp1763hc_kick(sc, q);
}

static void
isp1763hc_ptd_write(struct isp1763hc_softc *sc, struct isp1763hc_queue *q,
    int slot, const uint32_t *dw)
{
	uint8_t buf[ISP1763_PTD_SIZE];
	uint16_t ptd;

	ptd = q->q_ptdbase + slot * ISP1763_PTD_SIZE;
	isp1763hc_ptd_encode(dw, buf, ISP1763_PTD_SIZE / 4);

	/* slot is skipped while its descriptor is written */
	q->q_skip |= __BIT(slot);
	ISP_WRITE2(sc, q->q_regskip, q->q_skip);

	ISP_MEMWRITE(sc, ptd + 4, buf + 4, ISP1763_PTD_SIZE - 4);
	ISP_MEMWRITE(sc, ptd, buf, 4);
	ISP_WRITE2(sc, q->q_reglast, isp1763hc_last_map(q->q_busy));

	q->q_skip &= ~__BIT(slot);
	ISP_WRITE2(sc, q->q_regskip, q->q_skip);
}

static void
isp1763hc_arm(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix)
{
	struct usbd_xfer *xfer = &ix->ix_xfer;
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(xfer->ux_pipe);
	struct isp1763hc_queue *q = ix->ix_queue;
	usb_endpoint_descriptor_t *ed;
	struct isp1763hc_ptdreq pr;
	uint32_t dw[ISP1763_PTD_SIZE / 4];
	bool isread;

	ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	isread = usbd_xfer_isread(xfer);

	pr = ip->ip_req;
	pr.pr_addr = xfer->ux_pipe->up_dev->ud_addr;
	pr.pr_mps = UE_GET_SIZE(UGETW(ed->wMaxPacketSize));
	pr.pr_mem = q->q_membase + ix->ix_slot * q->q_memsize;

	switch (ix->ix_phase) {
	case ISP1763_PHASE_SETUP:
		pr.pr_token = ISP1763_TOKEN_SETUP;
		pr.pr_len = sizeof(xfer->ux_request);
		ISP_MEMWRITE(sc, pr.pr_mem, &xfer->ux_request, pr.pr_len);
		break;
	case ISP1763_PHASE_DATA:
		pr.pr_token = isread ? ISP1763_TOKEN_IN : ISP1763_TOKEN_OUT;
		pr.pr_len = MIN(xfer->ux_length - ix->ix_off, q->q_memsize);
		pr.pr_toggle = ip->ip_toggle;
		pr.pr_ping = ip->ip_ping && !isread;
		if (!isread && pr.pr_len > 0) {
			ISP_MEMWRITE(sc, pr.pr_mem,
			    (uint8_t *)xfer->ux_buf + ix->ix_off, pr.pr_len);
		}
		break;
	case ISP1763_PHASE_STATUS:
		pr.pr_token = isread && xfer->ux_length > 0 ?
		    ISP1763_TOKEN_OUT : ISP1763_TOKEN_IN;
		pr.pr_len = 0;
		pr.pr_toggle = true;
		break;
	}
	ix->ix_round = pr.pr_len;

	isp1763hc_ptd_build(&pr, dw);
	isp1763hc_ptd_write(sc, q, ix->ix_slot, dw);
}

/* Give free slots to waiting transfers. */
static void
isp1763hc_kick(struct isp1763hc_softc *sc, struct isp1763hc_queue *q)
{
	struct isp1763hc_xfer *ix;
	int slot;

	while ((ix = TAILQ_FIRST(&q->q_wait)) != NULL &&
	    (slot = isp1763hc_slot_get(q)) >= 0) {
		TAILQ_REMOVE(&q->q_wait, ix, ix_wait);
		ix->ix_waiting = false;
		isp1763hc_slot_take(q, ix, slot);
		isp1763hc_arm(sc, ix);
	}
}

static void
isp1763hc_finish(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix,
    usbd_status status)
{
	struct usbd_xfer *xfer = &ix->ix_xfer;

	isp1763hc_release(sc, ix);

	if (!usbd_xfer_trycomplete(xfer))
		return;
	xfer->ux_actlen = ix->ix_off;
	xfer->ux_status = status;
	usb_transfer_complete(xfer);
}

static bool
isp1763hc_zlp_owed(struct isp1763hc_xfer *ix, u_int mps)
{
	struct usbd_xfer *xfer = &ix->ix_xfer;

	if (ix->ix_zlp || usbd_xfer_isread(xfer))
		return false;
	if ((xfer->ux_flags & USBD_FORCE_SHORT_XFER) == 0)
		return false;

	return xfer->ux_length % mps == 0;
}

/* Something messed up. */
static bool
isp1763hc_round_failed(uint32_t dw3)
{

	return (dw3 & (ISP1763_PTD_DW3_H | ISP1763_PTD_DW3_X |
	    ISP1763_PTD_DW3_B)) != 0;
}

static bool
isp1763hc_round_stalled(uint32_t dw3)
{

	return (dw3 & ISP1763_PTD_DW3_H) != 0 &&
	    (dw3 & (ISP1763_PTD_DW3_X | ISP1763_PTD_DW3_B)) == 0 &&
	    __SHIFTOUT(dw3, ISP1763_PTD_DW3_CERR) != 0;
}

static uint32_t
isp1763hc_round_account(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix,
    const uint32_t *dw)
{
	struct usbd_xfer *xfer = &ix->ix_xfer;
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(xfer->ux_pipe);
	struct isp1763hc_queue *q = ix->ix_queue;
	uint32_t n;

	ip->ip_toggle = (dw[3] & ISP1763_PTD_DW3_DT) != 0;
	ip->ip_ping = q == &sc->sc_atl && !ip->ip_req.pr_split &&
	    (dw[3] & ISP1763_PTD_DW3_PING) != 0;
	if (ix->ix_phase != ISP1763_PHASE_DATA)
		return 0;

	if (ip->ip_req.pr_split)
		n = __SHIFTOUT(dw[3], ISP1763_PTD_DW3_NDONE_SPLIT);
	else
		n = __SHIFTOUT(dw[3], ISP1763_PTD_DW3_NDONE);
	n = MIN(n, ix->ix_round);

	if (usbd_xfer_isread(xfer) && n > 0) {
		ISP_MEMREAD(sc, q->q_membase + ix->ix_slot * q->q_memsize,
		    (uint8_t *)xfer->ux_buf + ix->ix_off, n);
	}
	ix->ix_off += n;

	return n;
}

static void
isp1763hc_round_next(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix,
    uint32_t n)
{
	struct usbd_xfer *xfer = &ix->ix_xfer;
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(xfer->ux_pipe);
	usb_endpoint_descriptor_t *ed;

	switch (ix->ix_phase) {
	case ISP1763_PHASE_SETUP:
		ip->ip_toggle = true;
		ix->ix_phase = xfer->ux_length > 0 ?
		    ISP1763_PHASE_DATA : ISP1763_PHASE_STATUS;
		isp1763hc_arm(sc, ix);
		return;
	case ISP1763_PHASE_STATUS:
		isp1763hc_finish(sc, ix, USBD_NORMAL_COMPLETION);
		return;
	case ISP1763_PHASE_DATA:
		break;
	}

	if (n == ix->ix_round && ix->ix_off < xfer->ux_length) {
		isp1763hc_arm(sc, ix);
		return;
	}

	ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	if (UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL) {
		ix->ix_phase = ISP1763_PHASE_STATUS;
		isp1763hc_arm(sc, ix);
		return;
	}
	if (isp1763hc_zlp_owed(ix, UE_GET_SIZE(UGETW(ed->wMaxPacketSize)))) {
		ix->ix_zlp = true;
		isp1763hc_arm(sc, ix);
		return;
	}

	isp1763hc_finish(sc, ix, USBD_NORMAL_COMPLETION);
}

static void
isp1763hc_round_done(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix,
    const uint32_t *dw)
{
	uint32_t n;

	if (isp1763hc_round_failed(dw[3])) {
		DPRINTF("%s: error dw3 0x%08x\n", device_xname(sc->sc_dev),
		    dw[3]);
		isp1763hc_finish(sc, ix, isp1763hc_round_stalled(dw[3]) ?
		    USBD_STALLED : USBD_IOERROR);
		return;
	}

	n = isp1763hc_round_account(sc, ix, dw);

	/*  chip gave up round up */
	if ((dw[3] & ISP1763_PTD_DW3_A) != 0) {
		isp1763hc_arm(sc, ix);
		return;
	}

	isp1763hc_round_next(sc, ix, n);
}

static void
isp1763hc_scan(struct isp1763hc_softc *sc, struct isp1763hc_queue *q)
{
	uint8_t buf[ISP1763_PTD_STATUS_DW * 4];
	uint32_t dw[ISP1763_PTD_STATUS_DW];
	int slot;

	/* Read back the done maps (but they are a hint only). */
	for (slot = 0; slot < ISP1763_NSLOTS; slot++) {
		if ((q->q_busy & __BIT(slot)) == 0)
			continue;

		ISP_MEMREAD(sc, q->q_ptdbase + slot * ISP1763_PTD_SIZE, buf,
		    sizeof(buf));
		isp1763hc_ptd_decode(buf, dw, ISP1763_PTD_STATUS_DW);
		if ((dw[0] & ISP1763_PTD_DW0_V) != 0)
			continue;

		isp1763hc_round_done(sc, q->q_xfer[slot], dw);
	}
}

static void
isp1763hc_softintr(void *v)
{
	struct usbd_bus *bus = v;
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);
	uint16_t atldone, hcint, intdone;

	KASSERT(bus->ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return;

	hcint = ISP_READ2(sc, ISP1763_HCINT);
	intdone = ISP_READ2(sc, ISP1763_INT_DONE);
	atldone = ISP_READ2(sc, ISP1763_ATL_DONE);
	if (hcint != 0)
		ISP_WRITE2(sc, ISP1763_HCINT, hcint);

	if ((hcint & sc->sc_atl.q_irq) != 0 || atldone != 0 ||
	    bus->ub_usepolling)
		isp1763hc_scan(sc, &sc->sc_atl);
	if ((hcint & sc->sc_int.q_irq) != 0 || intdone != 0 ||
	    bus->ub_usepolling)
		isp1763hc_scan(sc, &sc->sc_int);

	ISP_INTR_ENABLE(sc, true);
}

static void
isp1763hc_poll(struct usbd_bus *bus)
{

	isp1763hc_softintr(bus);
}

static usbd_status
isp1763hc_open(struct usbd_pipe *pipe)
{
	struct isp1763hc_softc *sc = ISP1763_PIPE2SC(pipe);
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(pipe);
	struct usbd_device *dev = pipe->up_dev;
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	struct isp1763hc_ptdreq *pr = &ip->ip_req;
	u_int mps, mult;
	int ival;

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (dev->ud_addr == dev->ud_bus->ub_rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			return USBD_NORMAL_COMPLETION;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &isp1763hc_root_intr_methods;
			return USBD_NORMAL_COMPLETION;
		default:
			return USBD_INVAL;
		}
	}

	memset(pr, 0, sizeof(*pr));
	pr->pr_ep = UE_GET_ADDR(ed->bEndpointAddress);
	mps = UE_GET_SIZE(UGETW(ed->wMaxPacketSize));
	mult = UE_GET_TRANS(UGETW(ed->wMaxPacketSize)) + 1;

	if (dev->ud_speed != USB_SPEED_HIGH) {
		if (dev->ud_myhsport == NULL)
			return USBD_INVAL;
		pr->pr_split = true;
		pr->pr_hub = dev->ud_myhsport->up_parent->ud_addr;
		pr->pr_port = dev->ud_myhsport->up_portno;
		pr->pr_se = dev->ud_speed == USB_SPEED_LOW ?
		    ISP1763_SE_LOW : ISP1763_SE_FULL;
	} else {
		pr->pr_mult = mult;
	}

	switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
	case UE_CONTROL:
		pr->pr_eptype = ISP1763_EPTYPE_CTRL;
		ip->ip_queue = &sc->sc_atl;
		break;
	case UE_BULK:
		pr->pr_eptype = ISP1763_EPTYPE_BULK;
		ip->ip_queue = &sc->sc_atl;
		break;
	case UE_INTERRUPT:
		if (mps * mult > ISP1763_INT_MEMSIZE)
			return USBD_INVAL;
		pr->pr_eptype = ISP1763_EPTYPE_INTR;
		ival = pipe->up_interval;
		if (ival == USBD_DEFAULT_INTERVAL)
			ival = ed->bInterval;
		isp1763hc_intr_sched(!pr->pr_split, ival, &pr->pr_uframe,
		    &pr->pr_usa);
		if (pr->pr_split)
			pr->pr_uscs = ISP1763_USCS_SPLIT;
		ip->ip_queue = &sc->sc_int;
		break;
	default:
		return USBD_INVAL;
	}

	ip->ip_toggle = pipe->up_endpoint->ue_toggle != 0;
	ip->ip_ping = false;
	pipe->up_methods = &isp1763hc_device_methods;

	return USBD_NORMAL_COMPLETION;
}

static struct usbd_xfer *
isp1763hc_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);
	struct usbd_xfer *xfer;

	xfer = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct isp1763hc_xfer));
		ISP1763_XFER2IX(xfer)->ix_slot = -1;
#ifdef DIAGNOSTIC
		xfer->ux_state = XFER_BUSY;
#endif
	}

	return xfer;
}

static void
isp1763hc_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);

	KASSERTMSG(xfer->ux_state == XFER_BUSY ||
	    xfer->ux_status == USBD_NOT_STARTED,
	    "xfer %p state %d\n", xfer, xfer->ux_state);
#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_FREE;
#endif

	pool_cache_put(sc->sc_xferpool, xfer);
}

/* Get descriptor off the chip. */
static void
isp1763hc_ptd_retire(struct isp1763hc_softc *sc, struct isp1763hc_xfer *ix)
{
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(ix->ix_xfer.ux_pipe);
	struct isp1763hc_queue *q = ix->ix_queue;
	uint8_t buf[ISP1763_PTD_STATUS_DW * 4];
	uint32_t dw[ISP1763_PTD_STATUS_DW];
	uint16_t ptd;

	ptd = q->q_ptdbase + ix->ix_slot * ISP1763_PTD_SIZE;

	if (q == &sc->sc_atl && ip->ip_req.pr_split) {
		ISP_MEMREAD(sc, ptd + 4, buf, 4);

		isp1763hc_ptd_decode(buf, dw, 1);
		dw[0] |= ISP1763_PTD_DW1_RETIRE;
		isp1763hc_ptd_encode(dw, buf, 1);

		ISP_MEMWRITE(sc, ptd + 4, buf, 4);

		delay(ISP1763_SPLIT_RETIRE_DELAY);
	}

	q->q_skip |= __BIT(ix->ix_slot);
	ISP_WRITE2(sc, q->q_regskip, q->q_skip);

	ISP_MEMREAD(sc, ptd, buf, sizeof(buf));
	isp1763hc_ptd_decode(buf, dw, ISP1763_PTD_STATUS_DW);
	ip->ip_toggle = (dw[3] & ISP1763_PTD_DW3_DT) != 0;
	dw[0] &= ~ISP1763_PTD_DW0_V;
	isp1763hc_ptd_encode(dw, buf, 1);
	ISP_MEMWRITE(sc, ptd, buf, 4);

	delay(ISP1763_RETIRE_DELAY);
}

/* Get a transfer off the queue. */
static void
isp1763hc_abortx(struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_XFER2SC(xfer);
	struct isp1763hc_xfer *ix = ISP1763_XFER2IX(xfer);
	struct isp1763hc_queue *q = ix->ix_queue;

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERTMSG(xfer->ux_status == USBD_CANCELLED ||
	    xfer->ux_status == USBD_TIMEOUT,
	    "bad abort status: %d", xfer->ux_status);

	if (ix->ix_waiting) {
		TAILQ_REMOVE(&q->q_wait, ix, ix_wait);
		ix->ix_waiting = false;
		return;
	}
	if (ix->ix_slot < 0)
		return;

	if (!sc->sc_dying)
		isp1763hc_ptd_retire(sc, ix);

	isp1763hc_release(sc, ix);
}

static bool
isp1763hc_dying(struct usbd_bus *bus)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);

	return sc->sc_dying;
}

static void
isp1763hc_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

static int
isp1763hc_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);
	int actlen;

	mutex_enter(&sc->sc_rhlock);
	actlen = isp1763hc_roothub_ctrl_locked(bus, req, buf, buflen);
	mutex_exit(&sc->sc_rhlock);

	return actlen;
}

static int
isp1763hc_port_reset(struct isp1763hc_softc *sc)
{
	uint32_t v;

	mutex_enter(&sc->sc_lock);
	v = isp1763hc_read4(sc, ISP1763_PORTSC1) &
	    ~(ISP1763_PORTSC_PED | ISP1763_PORTSC_ECSC);
	isp1763hc_write4(sc, ISP1763_PORTSC1, v | ISP1763_PORTSC_PR);
	mutex_exit(&sc->sc_lock);

	usb_delay_ms(&sc->sc_bus, USB_PORT_ROOT_RESET_DELAY);
	if (sc->sc_dying)
		return -1;

	mutex_enter(&sc->sc_lock);
	v = isp1763hc_read4(sc, ISP1763_PORTSC1) & ~ISP1763_PORTSC_ECSC;
	isp1763hc_write4(sc, ISP1763_PORTSC1, v & ~ISP1763_PORTSC_PR);
	mutex_exit(&sc->sc_lock);

	usb_delay_ms(&sc->sc_bus, ISP1763_PORT_RESET_DONE);
	if (sc->sc_dying)
		return -1;

	mutex_enter(&sc->sc_lock);
	v = isp1763hc_read4(sc, ISP1763_PORTSC1);
	mutex_exit(&sc->sc_lock);

	if ((v & (ISP1763_PORTSC_PR | ISP1763_PORTSC_PED)) !=
	    ISP1763_PORTSC_PED) {
		aprint_error_dev(sc->sc_dev, "root port reset failed: "
		    "portsc 0x%08x\n", v);
		return -1;
	}
	sc->sc_isreset = true;

	return 0;
}

static void
isp1763hc_portsc_modify(struct isp1763hc_softc *sc, uint32_t set,
    uint32_t clr)
{
	uint32_t v;

	mutex_enter(&sc->sc_lock);
	v = isp1763hc_read4(sc, ISP1763_PORTSC1) & ~ISP1763_PORTSC_ECSC;
	isp1763hc_write4(sc, ISP1763_PORTSC1, (v & ~clr) | set);
	mutex_exit(&sc->sc_lock);
}

static int
isp1763hc_roothub_hubdesc(void *buf, int buflen)
{
	usb_hub_descriptor_t hubd;
	int totlen;

	totlen = uimin(buflen, sizeof(hubd));
	memcpy(&hubd, buf, totlen);
	hubd.bNbrPorts = 1;
	USETW(hubd.wHubCharacteristics, UHD_PWR_INDIVIDUAL);
	hubd.bPwrOn2PwrGood = 10;
	hubd.DeviceRemovable[0] = __BIT(1);
	hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + 1;
	totlen = uimin(totlen, hubd.bDescLength);
	memcpy(buf, &hubd, totlen);

	return totlen;
}

static int
isp1763hc_roothub_port_status(struct isp1763hc_softc *sc, void *buf, int len)
{
	usb_port_status_t ps;
	uint32_t v;
	int status, change, totlen;

	mutex_enter(&sc->sc_lock);
	v = isp1763hc_read4(sc, ISP1763_PORTSC1);
	mutex_exit(&sc->sc_lock);

	status = UPS_HIGH_SPEED;
	if ((v & ISP1763_PORTSC_ECCS) != 0)
		status |= UPS_CURRENT_CONNECT_STATUS;
	if ((v & ISP1763_PORTSC_PED) != 0)
		status |= UPS_PORT_ENABLED;
	if ((v & ISP1763_PORTSC_SUSP) != 0)
		status |= UPS_SUSPEND;
	if ((v & ISP1763_PORTSC_PR) != 0)
		status |= UPS_RESET;
	if ((v & ISP1763_PORTSC_PP) != 0)
		status |= UPS_PORT_POWER;

	change = 0;
	if ((v & ISP1763_PORTSC_ECSC) != 0)
		change |= UPS_C_CONNECT_STATUS;
	if (sc->sc_isreset)
		change |= UPS_C_PORT_RESET;

	USETW(ps.wPortStatus, status);
	USETW(ps.wPortChange, change);
	totlen = uimin(len, sizeof(ps));
	memcpy(buf, &ps, totlen);

	return totlen;
}

static int
isp1763hc_roothub_port_clear(struct isp1763hc_softc *sc, int feature)
{

	switch (feature) {
	case UHF_PORT_ENABLE:
		isp1763hc_portsc_modify(sc, 0, ISP1763_PORTSC_PED);
		return 0;
	case UHF_PORT_POWER:
		isp1763hc_portsc_modify(sc, 0, ISP1763_PORTSC_PP);
		return 0;
	case UHF_C_PORT_CONNECTION:
		isp1763hc_portsc_modify(sc, ISP1763_PORTSC_ECSC, 0);
		return 0;
	case UHF_C_PORT_RESET:
		sc->sc_isreset = false;
		return 0;
	case UHF_PORT_SUSPEND:
	case UHF_PORT_TEST:
	case UHF_PORT_INDICATOR:
	case UHF_C_PORT_ENABLE:
	case UHF_C_PORT_SUSPEND:
	case UHF_C_PORT_OVER_CURRENT:
		return 0;
	default:
		return -1;
	}
}

static int
isp1763hc_roothub_port_set(struct isp1763hc_softc *sc, int feature)
{

	switch (feature) {
	case UHF_PORT_RESET:
		return isp1763hc_port_reset(sc);
	case UHF_PORT_POWER:
		isp1763hc_portsc_modify(sc, ISP1763_PORTSC_PP, 0);
		return 0;
	case UHF_PORT_ENABLE:
	case UHF_PORT_SUSPEND:
	case UHF_PORT_TEST:
	case UHF_PORT_INDICATOR:
		return 0;
	default:
		return -1;
	}
}

static int
isp1763hc_roothub_ctrl_locked(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct isp1763hc_softc *sc = ISP1763_BUS2SC(bus);
	uint16_t len, value, index;

	KASSERT(mutex_owned(&sc->sc_rhlock));

	if (sc->sc_dying)
		return -1;

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	/* Meh */
#define	C(x, y)	((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		if (len == 0)
			return 0;
		if (value != C(2, UDESC_STRING))
			return buflen;
		return usb_makestrdesc((usb_string_descriptor_t *)buf, len,
		    "ISP1763 root hub");
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			return 0;
		if ((value & 0xff) != 0)
			return -1;
		return isp1763hc_roothub_hubdesc(buf, buflen);
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4)
			return -1;
		memset(buf, 0, len);
		return len;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		if (index != 1 || len != 4)
			return -1;
		return isp1763hc_roothub_port_status(sc, buf, len);
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		return 0;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index != 1)
			return -1;
		return isp1763hc_roothub_port_clear(sc, value);
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index != 1)
			return -1;
		return isp1763hc_roothub_port_set(sc, value);
	case C(UR_CLEAR_TT_BUFFER, UT_WRITE_CLASS_OTHER):
	case C(UR_RESET_TT, UT_WRITE_CLASS_OTHER):
	case C(UR_GET_TT_STATE, UT_READ_CLASS_OTHER):
	case C(UR_STOP_TT, UT_WRITE_CLASS_OTHER):
		return 0;
	default:
		return buflen;
	}
#undef C
}

static usbd_status
isp1763hc_root_intr_transfer(struct usbd_xfer *xfer)
{

	return isp1763hc_root_intr_start(SIMPLEQ_FIRST(
	    &xfer->ux_pipe->up_queue));
}

static usbd_status
isp1763hc_root_intr_start(struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_XFER2SC(xfer);

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(sc->sc_intrxfer == NULL);
	sc->sc_intrxfer = xfer;
	xfer->ux_status = USBD_IN_PROGRESS;

	return USBD_IN_PROGRESS;
}

static void
isp1763hc_root_intr_abort(struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	if (sc->sc_intrxfer == NULL)
		return;

	KASSERT(sc->sc_intrxfer == xfer);
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);
	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

static void
isp1763hc_root_intr_close(struct usbd_pipe *pipe)
{
	struct isp1763hc_softc *sc __diagused = ISP1763_PIPE2SC(pipe);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(sc->sc_intrxfer == NULL);
}

static void
isp1763hc_root_intr_done(struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_XFER2SC(xfer);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(sc->sc_intrxfer == xfer);
	KASSERT(xfer->ux_status != USBD_IN_PROGRESS);
	sc->sc_intrxfer = NULL;
}

static usbd_status
isp1763hc_device_transfer(struct usbd_xfer *xfer)
{

	return isp1763hc_device_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
isp1763hc_device_start(struct usbd_xfer *xfer)
{
	struct isp1763hc_softc *sc = ISP1763_XFER2SC(xfer);
	struct isp1763hc_xfer *ix = ISP1763_XFER2IX(xfer);
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(xfer->ux_pipe);
	usb_endpoint_descriptor_t *ed = xfer->ux_pipe->up_endpoint->ue_edesc;
	struct isp1763hc_queue *q = ip->ip_queue;
	int slot;

	KASSERT(sc->sc_bus.ub_usepolling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	ix->ix_queue = q;
	ix->ix_slot = -1;
	ix->ix_off = 0;
	ix->ix_waiting = false;
	ix->ix_zlp = xfer->ux_length == 0;
	ix->ix_phase = UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL ?
	    ISP1763_PHASE_SETUP : ISP1763_PHASE_DATA;

	xfer->ux_status = USBD_IN_PROGRESS;
	usbd_xfer_schedule_timeout(xfer);

	slot = isp1763hc_slot_get(q);
	if (slot < 0) {
		TAILQ_INSERT_TAIL(&q->q_wait, ix, ix_wait);
		ix->ix_waiting = true;
		return USBD_IN_PROGRESS;
	}
	isp1763hc_slot_take(q, ix, slot);
	isp1763hc_arm(sc, ix);

	return USBD_IN_PROGRESS;
}

static void
isp1763hc_device_abort(struct usbd_xfer *xfer)
{

	usbd_xfer_abort(xfer);
}

static void
isp1763hc_device_close(struct usbd_pipe *pipe)
{
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(pipe);

	pipe->up_endpoint->ue_toggle = ip->ip_toggle;
}

static void
isp1763hc_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct isp1763hc_pipe *ip = ISP1763_PIPE2IP(pipe);

	ip->ip_toggle = false;
}

static void
isp1763hc_device_done(struct usbd_xfer *xfer)
{
	/* nothing left */
}

