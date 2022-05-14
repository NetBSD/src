/*	$NetBSD: xhci.c,v 1.165 2022/05/14 19:44:26 riastradh Exp $	*/

/*
 * Copyright (c) 2013 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * USB rev 2.0 and rev 3.1 specification
 *  http://www.usb.org/developers/docs/
 * xHCI rev 1.1 specification
 *  http://www.intel.com/technology/usb/spec.htm
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xhci.c,v 1.165 2022/05/14 19:44:26 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>

#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>
#include <dev/usb/usbroothub.h>


#ifdef USB_DEBUG
#ifndef XHCI_DEBUG
#define xhcidebug 0
#else /* !XHCI_DEBUG */
#define HEXDUMP(a, b, c) \
    do { \
	    if (xhcidebug > 0) \
		    hexdump(printf, a, b, c); \
    } while (/*CONSTCOND*/0)
static int xhcidebug = 0;

SYSCTL_SETUP(sysctl_hw_xhci_setup, "sysctl hw.xhci setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "xhci",
	    SYSCTL_DESCR("xhci global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &xhcidebug, sizeof(xhcidebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}

#endif /* !XHCI_DEBUG */
#endif /* USB_DEBUG */

#ifndef HEXDUMP
#define HEXDUMP(a, b, c)
#endif

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(xhcidebug,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(xhcidebug,N,FMT,A,B,C,D)
#define XHCIHIST_FUNC()		USBHIST_FUNC()
#define XHCIHIST_CALLED(name)	USBHIST_CALLED(xhcidebug)
#define XHCIHIST_CALLARGS(FMT,A,B,C,D) \
				USBHIST_CALLARGS(xhcidebug,FMT,A,B,C,D)

#define XHCI_DCI_SLOT 0
#define XHCI_DCI_EP_CONTROL 1

#define XHCI_ICI_INPUT_CONTROL 0

struct xhci_pipe {
	struct usbd_pipe xp_pipe;
	struct usb_task xp_async_task;
	int16_t xp_isoc_next; /* next frame */
	uint8_t xp_maxb; /* max burst */
	uint8_t xp_mult;
};

#define XHCI_COMMAND_RING_TRBS 256
#define XHCI_EVENT_RING_TRBS 256
#define XHCI_EVENT_RING_SEGMENTS 1
#define XHCI_TRB_3_ED_BIT XHCI_TRB_3_ISP_BIT

static usbd_status xhci_open(struct usbd_pipe *);
static void xhci_close_pipe(struct usbd_pipe *);
static int xhci_intr1(struct xhci_softc * const);
static void xhci_softintr(void *);
static void xhci_poll(struct usbd_bus *);
static struct usbd_xfer *xhci_allocx(struct usbd_bus *, unsigned int);
static void xhci_freex(struct usbd_bus *, struct usbd_xfer *);
static void xhci_abortx(struct usbd_xfer *);
static bool xhci_dying(struct usbd_bus *);
static void xhci_get_lock(struct usbd_bus *, kmutex_t **);
static usbd_status xhci_new_device(device_t, struct usbd_bus *, int, int, int,
    struct usbd_port *);
static int xhci_roothub_ctrl(struct usbd_bus *, usb_device_request_t *,
    void *, int);

static void xhci_pipe_restart(struct usbd_pipe *);
static void xhci_pipe_restart_async_task(void *);
static void xhci_pipe_restart_async(struct usbd_pipe *);

static usbd_status xhci_configure_endpoint(struct usbd_pipe *);
//static usbd_status xhci_unconfigure_endpoint(struct usbd_pipe *);
static void xhci_reset_endpoint(struct usbd_pipe *);
static usbd_status xhci_stop_endpoint_cmd(struct xhci_softc *,
    struct xhci_slot *, u_int, uint32_t);
static usbd_status xhci_stop_endpoint(struct usbd_pipe *);

static void xhci_host_dequeue(struct xhci_ring * const);
static void xhci_set_dequeue(struct usbd_pipe *);

static usbd_status xhci_do_command(struct xhci_softc * const,
    struct xhci_soft_trb * const, int);
static usbd_status xhci_do_command_locked(struct xhci_softc * const,
    struct xhci_soft_trb * const, int);
static usbd_status xhci_init_slot(struct usbd_device *, uint32_t);
static void xhci_free_slot(struct xhci_softc *, struct xhci_slot *);
static usbd_status xhci_set_address(struct usbd_device *, uint32_t, bool);
static usbd_status xhci_enable_slot(struct xhci_softc * const,
    uint8_t * const);
static usbd_status xhci_disable_slot(struct xhci_softc * const, uint8_t);
static usbd_status xhci_address_device(struct xhci_softc * const,
    uint64_t, uint8_t, bool);
static void xhci_set_dcba(struct xhci_softc * const, uint64_t, int);
static usbd_status xhci_update_ep0_mps(struct xhci_softc * const,
    struct xhci_slot * const, u_int);
static usbd_status xhci_ring_init(struct xhci_softc * const,
    struct xhci_ring **, size_t, size_t);
static void xhci_ring_free(struct xhci_softc * const,
    struct xhci_ring ** const);

static void xhci_setup_ctx(struct usbd_pipe *);
static void xhci_setup_route(struct usbd_pipe *, uint32_t *);
static void xhci_setup_tthub(struct usbd_pipe *, uint32_t *);
static void xhci_setup_maxburst(struct usbd_pipe *, uint32_t *);
static uint32_t xhci_bival2ival(uint32_t, uint32_t);

static void xhci_noop(struct usbd_pipe *);

static usbd_status xhci_root_intr_transfer(struct usbd_xfer *);
static usbd_status xhci_root_intr_start(struct usbd_xfer *);
static void xhci_root_intr_abort(struct usbd_xfer *);
static void xhci_root_intr_close(struct usbd_pipe *);
static void xhci_root_intr_done(struct usbd_xfer *);

static usbd_status xhci_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status xhci_device_ctrl_start(struct usbd_xfer *);
static void xhci_device_ctrl_abort(struct usbd_xfer *);
static void xhci_device_ctrl_close(struct usbd_pipe *);
static void xhci_device_ctrl_done(struct usbd_xfer *);

static usbd_status xhci_device_isoc_transfer(struct usbd_xfer *);
static usbd_status xhci_device_isoc_enter(struct usbd_xfer *);
static void xhci_device_isoc_abort(struct usbd_xfer *);
static void xhci_device_isoc_close(struct usbd_pipe *);
static void xhci_device_isoc_done(struct usbd_xfer *);

static usbd_status xhci_device_intr_transfer(struct usbd_xfer *);
static usbd_status xhci_device_intr_start(struct usbd_xfer *);
static void xhci_device_intr_abort(struct usbd_xfer *);
static void xhci_device_intr_close(struct usbd_pipe *);
static void xhci_device_intr_done(struct usbd_xfer *);

static usbd_status xhci_device_bulk_transfer(struct usbd_xfer *);
static usbd_status xhci_device_bulk_start(struct usbd_xfer *);
static void xhci_device_bulk_abort(struct usbd_xfer *);
static void xhci_device_bulk_close(struct usbd_pipe *);
static void xhci_device_bulk_done(struct usbd_xfer *);

static const struct usbd_bus_methods xhci_bus_methods = {
	.ubm_open = xhci_open,
	.ubm_softint = xhci_softintr,
	.ubm_dopoll = xhci_poll,
	.ubm_allocx = xhci_allocx,
	.ubm_freex = xhci_freex,
	.ubm_abortx = xhci_abortx,
	.ubm_dying = xhci_dying,
	.ubm_getlock = xhci_get_lock,
	.ubm_newdev = xhci_new_device,
	.ubm_rhctrl = xhci_roothub_ctrl,
};

static const struct usbd_pipe_methods xhci_root_intr_methods = {
	.upm_transfer = xhci_root_intr_transfer,
	.upm_start = xhci_root_intr_start,
	.upm_abort = xhci_root_intr_abort,
	.upm_close = xhci_root_intr_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_root_intr_done,
};


static const struct usbd_pipe_methods xhci_device_ctrl_methods = {
	.upm_transfer = xhci_device_ctrl_transfer,
	.upm_start = xhci_device_ctrl_start,
	.upm_abort = xhci_device_ctrl_abort,
	.upm_close = xhci_device_ctrl_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_ctrl_done,
};

static const struct usbd_pipe_methods xhci_device_isoc_methods = {
	.upm_transfer = xhci_device_isoc_transfer,
	.upm_abort = xhci_device_isoc_abort,
	.upm_close = xhci_device_isoc_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_isoc_done,
};

static const struct usbd_pipe_methods xhci_device_bulk_methods = {
	.upm_transfer = xhci_device_bulk_transfer,
	.upm_start = xhci_device_bulk_start,
	.upm_abort = xhci_device_bulk_abort,
	.upm_close = xhci_device_bulk_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_bulk_done,
};

static const struct usbd_pipe_methods xhci_device_intr_methods = {
	.upm_transfer = xhci_device_intr_transfer,
	.upm_start = xhci_device_intr_start,
	.upm_abort = xhci_device_intr_abort,
	.upm_close = xhci_device_intr_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_intr_done,
};

static inline uint32_t
xhci_read_1(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, offset);
}

static inline uint32_t
xhci_read_2(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, offset);
}

static inline uint32_t
xhci_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset);
}

static inline void
xhci_write_1(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, offset, value);
}

#if 0 /* unused */
static inline void
xhci_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, value);
}
#endif /* unused */

static inline void
xhci_barrier(const struct xhci_softc * const sc, int flags)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_ios, flags);
}

static inline uint32_t
xhci_cap_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_cbh, offset);
}

static inline uint32_t
xhci_op_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_obh, offset);
}

static inline void
xhci_op_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_obh, offset, value);
}

static inline uint64_t
xhci_op_read_8(const struct xhci_softc * const sc, bus_size_t offset)
{
	uint64_t value;

#ifdef XHCI_USE_BUS_SPACE_8
	value = bus_space_read_8(sc->sc_iot, sc->sc_obh, offset);
#else
	value = bus_space_read_4(sc->sc_iot, sc->sc_obh, offset);
	value |= (uint64_t)bus_space_read_4(sc->sc_iot, sc->sc_obh,
	    offset + 4) << 32;
#endif

	return value;
}

static inline void
xhci_op_write_8(const struct xhci_softc * const sc, bus_size_t offset,
    uint64_t value)
{
#ifdef XHCI_USE_BUS_SPACE_8
	bus_space_write_8(sc->sc_iot, sc->sc_obh, offset, value);
#else
	bus_space_write_4(sc->sc_iot, sc->sc_obh, offset + 0,
	    (value >> 0) & 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_obh, offset + 4,
	    (value >> 32) & 0xffffffff);
#endif
}

static inline uint32_t
xhci_rt_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_rbh, offset);
}

static inline void
xhci_rt_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset, value);
}

static inline uint64_t
xhci_rt_read_8(const struct xhci_softc * const sc, bus_size_t offset)
{
	uint64_t value;

#ifdef XHCI_USE_BUS_SPACE_8
	value = bus_space_read_8(sc->sc_iot, sc->sc_rbh, offset);
#else
	value = bus_space_read_4(sc->sc_iot, sc->sc_rbh, offset);
	value |= (uint64_t)bus_space_read_4(sc->sc_iot, sc->sc_rbh,
	    offset + 4) << 32;
#endif

	return value;
}

static inline void
xhci_rt_write_8(const struct xhci_softc * const sc, bus_size_t offset,
    uint64_t value)
{
#ifdef XHCI_USE_BUS_SPACE_8
	bus_space_write_8(sc->sc_iot, sc->sc_rbh, offset, value);
#else
	bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset + 0,
	    (value >> 0) & 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset + 4,
	    (value >> 32) & 0xffffffff);
#endif
}

#if 0 /* unused */
static inline uint32_t
xhci_db_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_dbh, offset);
}
#endif /* unused */

static inline void
xhci_db_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_dbh, offset, value);
}

/* --- */

static inline uint8_t
xhci_ep_get_type(usb_endpoint_descriptor_t * const ed)
{
	u_int eptype = 0;

	switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
	case UE_CONTROL:
		eptype = 0x0;
		break;
	case UE_ISOCHRONOUS:
		eptype = 0x1;
		break;
	case UE_BULK:
		eptype = 0x2;
		break;
	case UE_INTERRUPT:
		eptype = 0x3;
		break;
	}

	if ((UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL) ||
	    (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN))
		return eptype | 0x4;
	else
		return eptype;
}

static u_int
xhci_ep_get_dci(usb_endpoint_descriptor_t * const ed)
{
	/* xHCI 1.0 section 4.5.1 */
	u_int epaddr = UE_GET_ADDR(ed->bEndpointAddress);
	u_int in = 0;

	if ((UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL) ||
	    (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN))
		in = 1;

	return epaddr * 2 + in;
}

static inline u_int
xhci_dci_to_ici(const u_int i)
{
	return i + 1;
}

static inline void *
xhci_slot_get_dcv(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int dci)
{
	return KERNADDR(&xs->xs_dc_dma, sc->sc_ctxsz * dci);
}

#if 0 /* unused */
static inline bus_addr_t
xhci_slot_get_dcp(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int dci)
{
	return DMAADDR(&xs->xs_dc_dma, sc->sc_ctxsz * dci);
}
#endif /* unused */

static inline void *
xhci_slot_get_icv(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int ici)
{
	return KERNADDR(&xs->xs_ic_dma, sc->sc_ctxsz * ici);
}

static inline bus_addr_t
xhci_slot_get_icp(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int ici)
{
	return DMAADDR(&xs->xs_ic_dma, sc->sc_ctxsz * ici);
}

static inline struct xhci_trb *
xhci_ring_trbv(struct xhci_ring * const xr, u_int idx)
{
	return KERNADDR(&xr->xr_dma, XHCI_TRB_SIZE * idx);
}

static inline bus_addr_t
xhci_ring_trbp(struct xhci_ring * const xr, u_int idx)
{
	return DMAADDR(&xr->xr_dma, XHCI_TRB_SIZE * idx);
}

static inline void
xhci_xfer_put_trb(struct xhci_xfer * const xx, u_int idx,
    uint64_t parameter, uint32_t status, uint32_t control)
{
	KASSERTMSG(idx < xx->xx_ntrb, "idx=%u xx_ntrb=%u", idx, xx->xx_ntrb);
	xx->xx_trb[idx].trb_0 = parameter;
	xx->xx_trb[idx].trb_2 = status;
	xx->xx_trb[idx].trb_3 = control;
}

static inline void
xhci_trb_put(struct xhci_trb * const trb, uint64_t parameter, uint32_t status,
    uint32_t control)
{
	trb->trb_0 = htole64(parameter);
	trb->trb_2 = htole32(status);
	trb->trb_3 = htole32(control);
}

static int
xhci_trb_get_idx(struct xhci_ring *xr, uint64_t trb_0, int *idx)
{
	/* base address of TRBs */
	bus_addr_t trbp = xhci_ring_trbp(xr, 0);

	/* trb_0 range sanity check */
	if (trb_0 == 0 || trb_0 < trbp ||
	    (trb_0 - trbp) % sizeof(struct xhci_trb) != 0 ||
	    (trb_0 - trbp) / sizeof(struct xhci_trb) >= xr->xr_ntrb) {
		return 1;
	}
	*idx = (trb_0 - trbp) / sizeof(struct xhci_trb);
	return 0;
}

static unsigned int
xhci_get_epstate(struct xhci_softc * const sc, struct xhci_slot * const xs,
    u_int dci)
{
	uint32_t *cp;

	usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);
	cp = xhci_slot_get_dcv(sc, xs, dci);
	return XHCI_EPCTX_0_EPSTATE_GET(le32toh(cp[0]));
}

static inline unsigned int
xhci_ctlrport2bus(struct xhci_softc * const sc, unsigned int ctlrport)
{
	const unsigned int port = ctlrport - 1;
	const uint8_t bit = __BIT(port % NBBY);

	return __SHIFTOUT(sc->sc_ctlrportbus[port / NBBY], bit);
}

/*
 * Return the roothub port for a controller port.  Both are 1..n.
 */
static inline unsigned int
xhci_ctlrport2rhport(struct xhci_softc * const sc, unsigned int ctrlport)
{

	return sc->sc_ctlrportmap[ctrlport - 1];
}

/*
 * Return the controller port for a bus roothub port.  Both are 1..n.
 */
static inline unsigned int
xhci_rhport2ctlrport(struct xhci_softc * const sc, unsigned int bn,
    unsigned int rhport)
{

	return sc->sc_rhportmap[bn][rhport - 1];
}

/* --- */

void
xhci_childdet(device_t self, device_t child)
{
	struct xhci_softc * const sc = device_private(self);

	KASSERT((sc->sc_child == child) || (sc->sc_child2 == child));
	if (child == sc->sc_child2)
		sc->sc_child2 = NULL;
	else if (child == sc->sc_child)
		sc->sc_child = NULL;
}

int
xhci_detach(struct xhci_softc *sc, int flags)
{
	int rv = 0;

	if (sc->sc_child2 != NULL) {
		rv = config_detach(sc->sc_child2, flags);
		if (rv != 0)
			return rv;
		KASSERT(sc->sc_child2 == NULL);
	}

	if (sc->sc_child != NULL) {
		rv = config_detach(sc->sc_child, flags);
		if (rv != 0)
			return rv;
		KASSERT(sc->sc_child == NULL);
	}

	/* XXX unconfigure/free slots */

	/* verify: */
	xhci_rt_write_4(sc, XHCI_IMAN(0), 0);
	xhci_op_write_4(sc, XHCI_USBCMD, 0);
	/* do we need to wait for stop? */

	xhci_op_write_8(sc, XHCI_CRCR, 0);
	xhci_ring_free(sc, &sc->sc_cr);
	cv_destroy(&sc->sc_command_cv);
	cv_destroy(&sc->sc_cmdbusy_cv);

	xhci_rt_write_4(sc, XHCI_ERSTSZ(0), 0);
	xhci_rt_write_8(sc, XHCI_ERSTBA(0), 0);
	xhci_rt_write_8(sc, XHCI_ERDP(0), 0 | XHCI_ERDP_BUSY);
	xhci_ring_free(sc, &sc->sc_er);

	usb_freemem(&sc->sc_eventst_dma);

	xhci_op_write_8(sc, XHCI_DCBAAP, 0);
	usb_freemem(&sc->sc_dcbaa_dma);

	kmem_free(sc->sc_slots, sizeof(*sc->sc_slots) * sc->sc_maxslots);

	kmem_free(sc->sc_ctlrportbus,
	    howmany(sc->sc_maxports * sizeof(uint8_t), NBBY));
	kmem_free(sc->sc_ctlrportmap, sc->sc_maxports * sizeof(int));

	for (size_t j = 0; j < __arraycount(sc->sc_rhportmap); j++) {
		kmem_free(sc->sc_rhportmap[j], sc->sc_maxports * sizeof(int));
	}

	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);

	pool_cache_destroy(sc->sc_xferpool);

	return rv;
}

int
xhci_activate(device_t self, enum devact act)
{
	struct xhci_softc * const sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

bool
xhci_suspend(device_t self, const pmf_qual_t *qual)
{
	struct xhci_softc * const sc = device_private(self);
	size_t i, j, bn, dci;
	int port;
	uint32_t v;
	usbd_status err;
	bool ok = false;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/*
	 * Block issuance of new commands, and wait for all pending
	 * commands to complete.
	 */
	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_suspender == NULL);
	sc->sc_suspender = curlwp;
	while (sc->sc_command_addr != 0)
		cv_wait(&sc->sc_cmdbusy_cv, &sc->sc_lock);
	mutex_exit(&sc->sc_lock);

	/*
	 * Block roothub xfers which might touch portsc registers until
	 * we're done suspending.
	 */
	mutex_enter(&sc->sc_rhlock);

	/*
	 * xHCI Requirements Specification 1.2, May 2019, Sec. 4.23.2:
	 * xHCI Power Management, p. 342
	 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf#page=342
	 */

	/*
	 * `1. Stop all USB activity by issuing Stop Endpoint Commands
	 *     for Busy endpoints in the Running state.  If the Force
	 *     Save Context Capability (FSC = ``0'') is not supported,
	 *     then Stop Endpoint Commands shall be issued for all idle
	 *     endpoints in the Running state as well.  The Stop
	 *     Endpoint Command causes the xHC to update the respective
	 *     Endpoint or Stream Contexts in system memory, e.g. the
	 *     TR Dequeue Pointer, DCS, etc. fields.  Refer to
	 *     Implementation Note "0".'
	 */
	for (i = 0; i < sc->sc_maxslots; i++) {
		struct xhci_slot *xs = &sc->sc_slots[i];

		/* Skip if the slot is not in use.  */
		if (xs->xs_idx == 0)
			continue;

		for (dci = XHCI_DCI_SLOT; dci <= XHCI_MAX_DCI; dci++) {
			/* Skip if the endpoint is not Running.  */
			/* XXX What about Busy?  */
			if (xhci_get_epstate(sc, xs, dci) !=
			    XHCI_EPSTATE_RUNNING)
				continue;

			/* Stop endpoint.  */
			mutex_enter(&sc->sc_lock);
			err = xhci_stop_endpoint_cmd(sc, xs, dci,
			    XHCI_TRB_3_SUSP_EP_BIT);
			mutex_exit(&sc->sc_lock);
			if (err) {
				device_printf(self, "failed to stop endpoint"
				    " slot %zu dci %zu err %d\n",
				    i, dci, err);
				goto out;
			}
		}
	}

	/*
	 * Next, suspend all the ports:
	 *
	 * xHCI Requirements Specification 1.2, May 2019, Sec. 4.15:
	 * Suspend-Resume, pp. 276-283
	 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf#page=276
	 */
	for (bn = 0; bn < 2; bn++) {
		for (i = 1; i <= sc->sc_rhportcount[bn]; i++) {
			/* 4.15.1: Port Suspend.  */
			port = XHCI_PORTSC(xhci_rhport2ctlrport(sc, bn, i));

			/*
			 * `System software places individual ports
			 *  into suspend mode by writing a ``3'' into
			 *  the appropriate PORTSC register Port Link
			 *  State (PLS) field (refer to Section 5.4.8).
			 *  Software should only set the PLS field to
			 *  ``3'' when the port is in the Enabled
			 *  state.'
			 *
			 * `Software should not attempt to suspend a
			 *  port unless the port reports that it is in
			 *  the enabled (PED = ``1''; PLS < ``3'')
			 *  state (refer to Section 5.4.8 for more
			 *  information about PED and PLS).'
			 */
			v = xhci_op_read_4(sc, port);
			if (((v & XHCI_PS_PED) == 0) ||
			    XHCI_PS_PLS_GET(v) >= XHCI_PS_PLS_U3)
				continue;
			v &= ~(XHCI_PS_PLS_MASK | XHCI_PS_CLEAR);
			v |= XHCI_PS_LWS | XHCI_PS_PLS_SET(XHCI_PS_PLS_SETU3);
			xhci_op_write_4(sc, port, v);

			/*
			 * `When the PLS field is written with U3
			 *  (``3''), the status of the PLS bit will not
			 *  change to the target U state U3 until the
			 *  suspend signaling has completed to the
			 *  attached device (which may be as long as
			 *  10ms.).'
			 *
			 * `Software is required to wait for U3
			 *  transitions to complete before it puts the
			 *  xHC into a low power state, and before
			 *  resuming the port.'
			 *
			 * XXX Take advantage of the technique to
			 * reduce polling on host controllers that
			 * support the U3C capability.
			 */
			for (j = 0; j < XHCI_WAIT_PLS_U3; j++) {
				v = xhci_op_read_4(sc, port);
				if (XHCI_PS_PLS_GET(v) == XHCI_PS_PLS_U3)
					break;
				usb_delay_ms(&sc->sc_bus, 1);
			}
			if (j == XHCI_WAIT_PLS_U3) {
				device_printf(self,
				    "suspend timeout on bus %zu port %zu\n",
				    bn, i);
				goto out;
			}
		}
	}

	/*
	 * `2. Ensure that the Command Ring is in the Stopped state
	 *     (CRR = ``0'') or Idle (i.e. the Command Transfer Ring is
	 *     empty), and all Command Completion Events associated
	 *     with them have been received.'
	 *
	 * XXX
	 */

	/* `3. Stop the controller by setting Run/Stop (R/S) = ``0''.'  */
	xhci_op_write_4(sc, XHCI_USBCMD,
	    xhci_op_read_4(sc, XHCI_USBCMD) & ~XHCI_CMD_RS);

	/*
	 * `4. Read the Operational Runtime, and VTIO registers in the
	 *     following order: USBCMD, DNCTRL, DCBAAP, CONFIG, ERSTSZ,
	 *     ERSTBA, ERDP, IMAN, IMOD, and VTIO and save their
	 *     state.'
	 *
	 * (We don't use VTIO here (XXX for now?).)
	 */
	sc->sc_regs.usbcmd = xhci_op_read_4(sc, XHCI_USBCMD);
	sc->sc_regs.dnctrl = xhci_op_read_4(sc, XHCI_DNCTRL);
	sc->sc_regs.dcbaap = xhci_op_read_8(sc, XHCI_DCBAAP);
	sc->sc_regs.config = xhci_op_read_4(sc, XHCI_CONFIG);
	sc->sc_regs.erstsz0 = xhci_rt_read_4(sc, XHCI_ERSTSZ(0));
	sc->sc_regs.erstba0 = xhci_rt_read_8(sc, XHCI_ERSTBA(0));
	sc->sc_regs.erdp0 = xhci_rt_read_8(sc, XHCI_ERDP(0));
	sc->sc_regs.iman0 = xhci_rt_read_4(sc, XHCI_IMAN(0));
	sc->sc_regs.imod0 = xhci_rt_read_4(sc, XHCI_IMOD(0));

	/*
	 * `5. Set the Controller Save State (CSS) flag in the USBCMD
	 *     register (5.4.1)...'
	 */
	xhci_op_write_4(sc, XHCI_USBCMD,
	    xhci_op_read_4(sc, XHCI_USBCMD) | XHCI_CMD_CSS);

	/*
	 *    `...and wait for the Save State Status (SSS) flag in the
	 *     USBSTS register (5.4.2) to transition to ``0''.'
	 */
	for (i = 0; i < XHCI_WAIT_SSS; i++) {
		if ((xhci_op_read_4(sc, XHCI_USBSTS) & XHCI_STS_SSS) == 0)
			break;
		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (i >= XHCI_WAIT_SSS) {
		device_printf(self, "suspend timeout, USBSTS.SSS\n");
		/*
		 * Just optimistically go on and check SRE anyway --
		 * what's the worst that could happen?
		 */
	}

	/*
	 * `Note: After a Save or Restore operation completes, the
	 *  Save/Restore Error (SRE) flag in the USBSTS register should
	 *  be checked to ensure that the operation completed
	 *  successfully.'
	 */
	if (xhci_op_read_4(sc, XHCI_USBSTS) & XHCI_STS_SRE) {
		device_printf(self, "suspend error, USBSTS.SRE\n");
		goto out;
	}

	/* Success!  */
	ok = true;

out:	mutex_exit(&sc->sc_rhlock);
	return ok;
}

bool
xhci_resume(device_t self, const pmf_qual_t *qual)
{
	struct xhci_softc * const sc = device_private(self);
	size_t i, j, bn, dci;
	int port;
	uint32_t v;
	bool ok = false;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(sc->sc_suspender);

	/*
	 * Block roothub xfers which might touch portsc registers until
	 * we're done resuming.
	 */
	mutex_enter(&sc->sc_rhlock);

	/*
	 * xHCI Requirements Specification 1.2, May 2019, Sec. 4.23.2:
	 * xHCI Power Management, p. 343
	 * https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf#page=343
	 */

	/*
	 * `4. Restore the Operational Runtime, and VTIO registers with
	 *     their previously saved state in the following order:
	 *     DNCTRL, DCBAAP, CONFIG, ERSTSZ, ERSTBA, ERDP, IMAN,
	 *     IMOD, and VTIO.'
	 *
	 * (We don't use VTIO here (for now?).)
	 */
	xhci_op_write_4(sc, XHCI_USBCMD, sc->sc_regs.usbcmd);
	xhci_op_write_4(sc, XHCI_DNCTRL, sc->sc_regs.dnctrl);
	xhci_op_write_8(sc, XHCI_DCBAAP, sc->sc_regs.dcbaap);
	xhci_op_write_4(sc, XHCI_CONFIG, sc->sc_regs.config);
	xhci_rt_write_4(sc, XHCI_ERSTSZ(0), sc->sc_regs.erstsz0);
	xhci_rt_write_8(sc, XHCI_ERSTBA(0), sc->sc_regs.erstba0);
	xhci_rt_write_8(sc, XHCI_ERDP(0), sc->sc_regs.erdp0);
	xhci_rt_write_4(sc, XHCI_IMAN(0), sc->sc_regs.iman0);
	xhci_rt_write_4(sc, XHCI_IMOD(0), sc->sc_regs.imod0);

	memset(&sc->sc_regs, 0, sizeof(sc->sc_regs)); /* paranoia */

	/*
	 * `5. Set the Controller Restore State (CRS) flag in the
	 *     USBCMD register (5.4.1) to ``1''...'
	 */
	xhci_op_write_4(sc, XHCI_USBCMD,
	    xhci_op_read_4(sc, XHCI_USBCMD) | XHCI_CMD_CRS);

	/*
	 *    `...and wait for the Restore State Status (RSS) in the
	 *     USBSTS register (5.4.2) to transition to ``0''.'
	 */
	for (i = 0; i < XHCI_WAIT_RSS; i++) {
		if ((xhci_op_read_4(sc, XHCI_USBSTS) & XHCI_STS_RSS) == 0)
			break;
		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (i >= XHCI_WAIT_RSS) {
		device_printf(self, "resume timeout, USBSTS.RSS\n");
		goto out;
	}

	/*
	 * `6. Reinitialize the Command Ring, i.e. so its Cycle bits
	 *     are consistent with the RCS values to be written to the
	 *     CRCR.'
	 *
	 * XXX Hope just zeroing it is good enough!
	 */
	xhci_host_dequeue(sc->sc_cr);

	/*
	 * `7. Write the CRCR with the address and RCS value of the
	 *     reinitialized Command Ring.  Note that this write will
	 *     cause the Command Ring to restart at the address
	 *     specified by the CRCR.'
	 */
	xhci_op_write_8(sc, XHCI_CRCR, xhci_ring_trbp(sc->sc_cr, 0) |
	    sc->sc_cr->xr_cs);

	/*
	 * `8. Enable the controller by setting Run/Stop (R/S) =
	 *     ``1''.'
	 */
	xhci_op_write_4(sc, XHCI_USBCMD,
	    xhci_op_read_4(sc, XHCI_USBCMD) | XHCI_CMD_RS);

	/*
	 * `9. Software shall walk the USB topology and initialize each
	 *     of the xHC PORTSC, PORTPMSC, and PORTLI registers, and
	 *     external hub ports attached to USB devices.'
	 *
	 * This follows the procedure in 4.15 `Suspend-Resume', 4.15.2
	 * `Port Resume', 4.15.2.2 `Host Initiated'.
	 *
	 * XXX We should maybe batch up initiating the state
	 * transitions, and then wait for them to complete all at once.
	 */
	for (bn = 0; bn < 2; bn++) {
		for (i = 1; i <= sc->sc_rhportcount[bn]; i++) {
			port = XHCI_PORTSC(xhci_rhport2ctlrport(sc, bn, i));

			/* `When a port is in the U3 state: ...' */
			v = xhci_op_read_4(sc, port);
			if (XHCI_PS_PLS_GET(v) != XHCI_PS_PLS_U3)
				continue;

			/*
			 * `For a USB2 protocol port, software shall
			 *  write a ``15'' (Resume) to the PLS field to
			 *  initiate resume signaling.  The port shall
			 *  transition to the Resume substate and the
			 *  xHC shall transmit the resume signaling
			 *  within 1ms (T_URSM).  Software shall ensure
			 *  that resume is signaled for at least 20ms
			 *  (T_DRSMDN).  Software shall start timing
			 *  T_DRSMDN from the write of ``15'' (Resume)
			 *  to PLS.'
			 */
			if (bn == 1) {
				KASSERT(sc->sc_bus2.ub_revision == USBREV_2_0);
				v &= ~(XHCI_PS_PLS_MASK | XHCI_PS_CLEAR);
				v |= XHCI_PS_LWS;
				v |= XHCI_PS_PLS_SET(XHCI_PS_PLS_SETRESUME);
				xhci_op_write_4(sc, port, v);
				usb_delay_ms(&sc->sc_bus, USB_RESUME_WAIT);
			} else {
				KASSERT(sc->sc_bus.ub_revision > USBREV_2_0);
			}

			/*
			 * `For a USB3 protocol port [and a USB2
			 *  protocol port after transitioning to
			 *  Resume], software shall write a ``0'' (U0)
			 *  to the PLS field...'
			 */
			v = xhci_op_read_4(sc, port);
			v &= ~(XHCI_PS_PLS_MASK | XHCI_PS_CLEAR);
			v |= XHCI_PS_LWS | XHCI_PS_PLS_SET(XHCI_PS_PLS_SETU0);
			xhci_op_write_4(sc, port, v);

			for (j = 0; j < XHCI_WAIT_PLS_U0; j++) {
				v = xhci_op_read_4(sc, port);
				if (XHCI_PS_PLS_GET(v) == XHCI_PS_PLS_U0)
					break;
				usb_delay_ms(&sc->sc_bus, 1);
			}
			if (j == XHCI_WAIT_PLS_U0) {
				device_printf(self,
				    "resume timeout on bus %zu port %zu\n",
				    bn, i);
				goto out;
			}
		}
	}

	/*
	 * `10. Restart each of the previously Running endpoints by
	 *      ringing their doorbells.'
	 */
	for (i = 0; i < sc->sc_maxslots; i++) {
		struct xhci_slot *xs = &sc->sc_slots[i];

		/* Skip if the slot is not in use.  */
		if (xs->xs_idx == 0)
			continue;

		for (dci = XHCI_DCI_SLOT; dci <= XHCI_MAX_DCI; dci++) {
			/* Skip if the endpoint is not Running.  */
			if (xhci_get_epstate(sc, xs, dci) !=
			    XHCI_EPSTATE_RUNNING)
				continue;

			/* Ring the doorbell.  */
			xhci_db_write_4(sc, XHCI_DOORBELL(xs->xs_idx), dci);
		}
	}

	/*
	 * `Note: After a Save or Restore operation completes, the
	 *  Save/Restore Error (SRE) flag in the USBSTS register should
	 *  be checked to ensure that the operation completed
	 *  successfully.'
	 */
	if (xhci_op_read_4(sc, XHCI_USBSTS) & XHCI_STS_SRE) {
		device_printf(self, "resume error, USBSTS.SRE\n");
		goto out;
	}

	/* Resume command issuance.  */
	mutex_enter(&sc->sc_lock);
	KASSERT(sc->sc_suspender);
	sc->sc_suspender = NULL;
	cv_broadcast(&sc->sc_cmdbusy_cv);
	mutex_exit(&sc->sc_lock);

	/* Success!  */
	ok = true;

out:	mutex_exit(&sc->sc_rhlock);
	return ok;
}

bool
xhci_shutdown(device_t self, int flags)
{
	return false;
}

static int
xhci_hc_reset(struct xhci_softc * const sc)
{
	uint32_t usbcmd, usbsts;
	int i;

	/* Check controller not ready */
	for (i = 0; i < XHCI_WAIT_CNR; i++) {
		usbsts = xhci_op_read_4(sc, XHCI_USBSTS);
		if ((usbsts & XHCI_STS_CNR) == 0)
			break;
		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (i >= XHCI_WAIT_CNR) {
		aprint_error_dev(sc->sc_dev, "controller not ready timeout\n");
		return EIO;
	}

	/* Halt controller */
	usbcmd = 0;
	xhci_op_write_4(sc, XHCI_USBCMD, usbcmd);
	usb_delay_ms(&sc->sc_bus, 1);

	/* Reset controller */
	usbcmd = XHCI_CMD_HCRST;
	xhci_op_write_4(sc, XHCI_USBCMD, usbcmd);
	for (i = 0; i < XHCI_WAIT_HCRST; i++) {
		/*
		 * Wait 1ms first. Existing Intel xHCI requires 1ms delay to
		 * prevent system hang (Errata).
		 */
		usb_delay_ms(&sc->sc_bus, 1);
		usbcmd = xhci_op_read_4(sc, XHCI_USBCMD);
		if ((usbcmd & XHCI_CMD_HCRST) == 0)
			break;
	}
	if (i >= XHCI_WAIT_HCRST) {
		aprint_error_dev(sc->sc_dev, "host controller reset timeout\n");
		return EIO;
	}

	/* Check controller not ready */
	for (i = 0; i < XHCI_WAIT_CNR; i++) {
		usbsts = xhci_op_read_4(sc, XHCI_USBSTS);
		if ((usbsts & XHCI_STS_CNR) == 0)
			break;
		usb_delay_ms(&sc->sc_bus, 1);
	}
	if (i >= XHCI_WAIT_CNR) {
		aprint_error_dev(sc->sc_dev,
		    "controller not ready timeout after reset\n");
		return EIO;
	}

	return 0;
}

/* 7.2 xHCI Support Protocol Capability */
static void
xhci_id_protocols(struct xhci_softc *sc, bus_size_t ecp)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* XXX Cache this lot */

	const uint32_t w0 = xhci_read_4(sc, ecp);
	const uint32_t w4 = xhci_read_4(sc, ecp + 4);
	const uint32_t w8 = xhci_read_4(sc, ecp + 8);
	const uint32_t wc = xhci_read_4(sc, ecp + 0xc);

	aprint_debug_dev(sc->sc_dev,
	    " SP: 0x%08x 0x%08x 0x%08x 0x%08x\n", w0, w4, w8, wc);

	if (w4 != XHCI_XECP_USBID)
		return;

	const int major = XHCI_XECP_SP_W0_MAJOR(w0);
	const int minor = XHCI_XECP_SP_W0_MINOR(w0);
	const uint8_t cpo = XHCI_XECP_SP_W8_CPO(w8);
	const uint8_t cpc = XHCI_XECP_SP_W8_CPC(w8);

	const uint16_t mm = __SHIFTOUT(w0, __BITS(31, 16));
	switch (mm) {
	case 0x0200:
	case 0x0300:
	case 0x0301:
	case 0x0310:
	case 0x0320:
		aprint_debug_dev(sc->sc_dev, " %s ports %d - %d\n",
		    major == 3 ? "ss" : "hs", cpo, cpo + cpc -1);
		break;
	default:
		aprint_error_dev(sc->sc_dev, " unknown major/minor (%d/%d)\n",
		    major, minor);
		return;
	}

	const size_t bus = (major == 3) ? 0 : 1;

	/* Index arrays with 0..n-1 where ports are numbered 1..n */
	for (size_t cp = cpo - 1; cp < cpo + cpc - 1; cp++) {
		if (sc->sc_ctlrportmap[cp] != 0) {
			aprint_error_dev(sc->sc_dev, "controller port %zu "
			    "already assigned", cp);
			continue;
		}

		sc->sc_ctlrportbus[cp / NBBY] |=
		    bus == 0 ? 0 : __BIT(cp % NBBY);

		const size_t rhp = sc->sc_rhportcount[bus]++;

		KASSERTMSG(sc->sc_rhportmap[bus][rhp] == 0,
		    "bus %zu rhp %zu is %d", bus, rhp,
		    sc->sc_rhportmap[bus][rhp]);

		sc->sc_rhportmap[bus][rhp] = cp + 1;
		sc->sc_ctlrportmap[cp] = rhp + 1;
	}
}

/* Process extended capabilities */
static void
xhci_ecp(struct xhci_softc *sc)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	bus_size_t ecp = XHCI_HCC_XECP(sc->sc_hcc) * 4;
	while (ecp != 0) {
		uint32_t ecr = xhci_read_4(sc, ecp);
		aprint_debug_dev(sc->sc_dev, "ECR: 0x%08x\n", ecr);
		switch (XHCI_XECP_ID(ecr)) {
		case XHCI_ID_PROTOCOLS: {
			xhci_id_protocols(sc, ecp);
			break;
		}
		case XHCI_ID_USB_LEGACY: {
			uint8_t bios_sem;

			/* Take host controller ownership from BIOS */
			bios_sem = xhci_read_1(sc, ecp + XHCI_XECP_BIOS_SEM);
			if (bios_sem) {
				/* sets xHCI to be owned by OS */
				xhci_write_1(sc, ecp + XHCI_XECP_OS_SEM, 1);
				aprint_debug_dev(sc->sc_dev,
				    "waiting for BIOS to give up control\n");
				for (int i = 0; i < 5000; i++) {
					bios_sem = xhci_read_1(sc, ecp +
					    XHCI_XECP_BIOS_SEM);
					if (bios_sem == 0)
						break;
					DELAY(1000);
				}
				if (bios_sem) {
					aprint_error_dev(sc->sc_dev,
					    "timed out waiting for BIOS\n");
				}
			}
			break;
		}
		default:
			break;
		}
		ecr = xhci_read_4(sc, ecp);
		if (XHCI_XECP_NEXT(ecr) == 0) {
			ecp = 0;
		} else {
			ecp += XHCI_XECP_NEXT(ecr) * 4;
		}
	}
}

#define XHCI_HCCPREV1_BITS	\
	"\177\020"	/* New bitmask */			\
	"f\020\020XECP\0"					\
	"f\014\4MAXPSA\0"					\
	"b\013CFC\0"						\
	"b\012SEC\0"						\
	"b\011SBD\0"						\
	"b\010FSE\0"						\
	"b\7NSS\0"						\
	"b\6LTC\0"						\
	"b\5LHRC\0"						\
	"b\4PIND\0"						\
	"b\3PPC\0"						\
	"b\2CZC\0"						\
	"b\1BNC\0"						\
	"b\0AC64\0"						\
	"\0"
#define XHCI_HCCV1_x_BITS	\
	"\177\020"	/* New bitmask */			\
	"f\020\020XECP\0"					\
	"f\014\4MAXPSA\0"					\
	"b\013CFC\0"						\
	"b\012SEC\0"						\
	"b\011SPC\0"						\
	"b\010PAE\0"						\
	"b\7NSS\0"						\
	"b\6LTC\0"						\
	"b\5LHRC\0"						\
	"b\4PIND\0"						\
	"b\3PPC\0"						\
	"b\2CSZ\0"						\
	"b\1BNC\0"						\
	"b\0AC64\0"						\
	"\0"

#define XHCI_HCC2_BITS	\
	"\177\020"	/* New bitmask */			\
	"b\7ETC_TSC\0"						\
	"b\6ETC\0"						\
	"b\5CIC\0"						\
	"b\4LEC\0"						\
	"b\3CTC\0"						\
	"b\2FSC\0"						\
	"b\1CMC\0"						\
	"b\0U3C\0"						\
	"\0"

void
xhci_start(struct xhci_softc *sc)
{
	xhci_rt_write_4(sc, XHCI_IMAN(0), XHCI_IMAN_INTR_ENA);
	if ((sc->sc_quirks & XHCI_QUIRK_INTEL) != 0)
		/* Intel xhci needs interrupt rate moderated. */
		xhci_rt_write_4(sc, XHCI_IMOD(0), XHCI_IMOD_DEFAULT_LP);
	else
		xhci_rt_write_4(sc, XHCI_IMOD(0), 0);
	aprint_debug_dev(sc->sc_dev, "current IMOD %u\n",
	    xhci_rt_read_4(sc, XHCI_IMOD(0)));

	/* Go! */
	xhci_op_write_4(sc, XHCI_USBCMD, XHCI_CMD_INTE|XHCI_CMD_RS);
	aprint_debug_dev(sc->sc_dev, "USBCMD 0x%08"PRIx32"\n",
	    xhci_op_read_4(sc, XHCI_USBCMD));
}

int
xhci_init(struct xhci_softc *sc)
{
	bus_size_t bsz;
	uint32_t hcs1, hcs2, hcs3, dboff, rtsoff;
	uint32_t pagesize, config;
	int i = 0;
	uint16_t hciversion;
	uint8_t caplength;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Set up the bus struct for the usb 3 and usb 2 buses */
	sc->sc_bus.ub_methods = &xhci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct xhci_pipe);
	sc->sc_bus.ub_usedma = true;
	sc->sc_bus.ub_hcpriv = sc;

	sc->sc_bus2.ub_methods = &xhci_bus_methods;
	sc->sc_bus2.ub_pipesize = sizeof(struct xhci_pipe);
	sc->sc_bus2.ub_revision = USBREV_2_0;
	sc->sc_bus2.ub_usedma = true;
	sc->sc_bus2.ub_hcpriv = sc;
	sc->sc_bus2.ub_dmatag = sc->sc_bus.ub_dmatag;

	caplength = xhci_read_1(sc, XHCI_CAPLENGTH);
	hciversion = xhci_read_2(sc, XHCI_HCIVERSION);

	if (hciversion < XHCI_HCIVERSION_0_96 ||
	    hciversion >= 0x0200) {
		aprint_normal_dev(sc->sc_dev,
		    "xHCI version %x.%x not known to be supported\n",
		    (hciversion >> 8) & 0xff, (hciversion >> 0) & 0xff);
	} else {
		aprint_verbose_dev(sc->sc_dev, "xHCI version %x.%x\n",
		    (hciversion >> 8) & 0xff, (hciversion >> 0) & 0xff);
	}

	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, 0, caplength,
	    &sc->sc_cbh) != 0) {
		aprint_error_dev(sc->sc_dev, "capability subregion failure\n");
		return ENOMEM;
	}

	hcs1 = xhci_cap_read_4(sc, XHCI_HCSPARAMS1);
	sc->sc_maxslots = XHCI_HCS1_MAXSLOTS(hcs1);
	sc->sc_maxintrs = XHCI_HCS1_MAXINTRS(hcs1);
	sc->sc_maxports = XHCI_HCS1_MAXPORTS(hcs1);
	hcs2 = xhci_cap_read_4(sc, XHCI_HCSPARAMS2);
	hcs3 = xhci_cap_read_4(sc, XHCI_HCSPARAMS3);
	aprint_debug_dev(sc->sc_dev,
	    "hcs1=%"PRIx32" hcs2=%"PRIx32" hcs3=%"PRIx32"\n", hcs1, hcs2, hcs3);

	sc->sc_hcc = xhci_cap_read_4(sc, XHCI_HCCPARAMS);
	sc->sc_ctxsz = XHCI_HCC_CSZ(sc->sc_hcc) ? 64 : 32;

	char sbuf[128];
	if (hciversion < XHCI_HCIVERSION_1_0)
		snprintb(sbuf, sizeof(sbuf), XHCI_HCCPREV1_BITS, sc->sc_hcc);
	else
		snprintb(sbuf, sizeof(sbuf), XHCI_HCCV1_x_BITS, sc->sc_hcc);
	aprint_debug_dev(sc->sc_dev, "hcc=%s\n", sbuf);
	aprint_debug_dev(sc->sc_dev, "xECP %" __PRIxBITS "\n",
	    XHCI_HCC_XECP(sc->sc_hcc) * 4);
	if (hciversion >= XHCI_HCIVERSION_1_1) {
		sc->sc_hcc2 = xhci_cap_read_4(sc, XHCI_HCCPARAMS2);
		snprintb(sbuf, sizeof(sbuf), XHCI_HCC2_BITS, sc->sc_hcc2);
		aprint_debug_dev(sc->sc_dev, "hcc2=%s\n", sbuf);
	}

	/* default all ports to bus 0, i.e. usb 3 */
	sc->sc_ctlrportbus = kmem_zalloc(
	    howmany(sc->sc_maxports * sizeof(uint8_t), NBBY), KM_SLEEP);
	sc->sc_ctlrportmap = kmem_zalloc(sc->sc_maxports * sizeof(int), KM_SLEEP);

	/* controller port to bus roothub port map */
	for (size_t j = 0; j < __arraycount(sc->sc_rhportmap); j++) {
		sc->sc_rhportmap[j] = kmem_zalloc(sc->sc_maxports * sizeof(int), KM_SLEEP);
	}

	/*
	 * Process all Extended Capabilities
	 */
	xhci_ecp(sc);

	bsz = XHCI_PORTSC(sc->sc_maxports);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, caplength, bsz,
	    &sc->sc_obh) != 0) {
		aprint_error_dev(sc->sc_dev, "operational subregion failure\n");
		return ENOMEM;
	}

	dboff = xhci_cap_read_4(sc, XHCI_DBOFF);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, dboff,
	    sc->sc_maxslots * 4, &sc->sc_dbh) != 0) {
		aprint_error_dev(sc->sc_dev, "doorbell subregion failure\n");
		return ENOMEM;
	}

	rtsoff = xhci_cap_read_4(sc, XHCI_RTSOFF);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, rtsoff,
	    sc->sc_maxintrs * 0x20, &sc->sc_rbh) != 0) {
		aprint_error_dev(sc->sc_dev, "runtime subregion failure\n");
		return ENOMEM;
	}

	int rv;
	rv = xhci_hc_reset(sc);
	if (rv != 0) {
		return rv;
	}

	if (sc->sc_vendor_init)
		sc->sc_vendor_init(sc);

	pagesize = xhci_op_read_4(sc, XHCI_PAGESIZE);
	aprint_debug_dev(sc->sc_dev, "PAGESIZE 0x%08x\n", pagesize);
	pagesize = ffs(pagesize);
	if (pagesize == 0) {
		aprint_error_dev(sc->sc_dev, "pagesize is 0\n");
		return EIO;
	}
	sc->sc_pgsz = 1 << (12 + (pagesize - 1));
	aprint_debug_dev(sc->sc_dev, "sc_pgsz 0x%08x\n", (uint32_t)sc->sc_pgsz);
	aprint_debug_dev(sc->sc_dev, "sc_maxslots 0x%08x\n",
	    (uint32_t)sc->sc_maxslots);
	aprint_debug_dev(sc->sc_dev, "sc_maxports %d\n", sc->sc_maxports);

	int err;
	sc->sc_maxspbuf = XHCI_HCS2_MAXSPBUF(hcs2);
	aprint_debug_dev(sc->sc_dev, "sc_maxspbuf %d\n", sc->sc_maxspbuf);
	if (sc->sc_maxspbuf != 0) {
		err = usb_allocmem(sc->sc_bus.ub_dmatag,
		    sizeof(uint64_t) * sc->sc_maxspbuf, sizeof(uint64_t),
		    USBMALLOC_COHERENT | USBMALLOC_ZERO,
		    &sc->sc_spbufarray_dma);
		if (err) {
			aprint_error_dev(sc->sc_dev,
			    "spbufarray init fail, err %d\n", err);
			return ENOMEM;
		}

		sc->sc_spbuf_dma = kmem_zalloc(sizeof(*sc->sc_spbuf_dma) *
		    sc->sc_maxspbuf, KM_SLEEP);
		uint64_t *spbufarray = KERNADDR(&sc->sc_spbufarray_dma, 0);
		for (i = 0; i < sc->sc_maxspbuf; i++) {
			usb_dma_t * const dma = &sc->sc_spbuf_dma[i];
			/* allocate contexts */
			err = usb_allocmem(sc->sc_bus.ub_dmatag, sc->sc_pgsz,
			    sc->sc_pgsz, USBMALLOC_COHERENT | USBMALLOC_ZERO,
			    dma);
			if (err) {
				aprint_error_dev(sc->sc_dev,
				    "spbufarray_dma init fail, err %d\n", err);
				rv = ENOMEM;
				goto bad1;
			}
			spbufarray[i] = htole64(DMAADDR(dma, 0));
			usb_syncmem(dma, 0, sc->sc_pgsz,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		}

		usb_syncmem(&sc->sc_spbufarray_dma, 0,
		    sizeof(uint64_t) * sc->sc_maxspbuf, BUS_DMASYNC_PREWRITE);
	}

	config = xhci_op_read_4(sc, XHCI_CONFIG);
	config &= ~0xFF;
	config |= sc->sc_maxslots & 0xFF;
	xhci_op_write_4(sc, XHCI_CONFIG, config);

	err = xhci_ring_init(sc, &sc->sc_cr, XHCI_COMMAND_RING_TRBS,
	    XHCI_COMMAND_RING_SEGMENTS_ALIGN);
	if (err) {
		aprint_error_dev(sc->sc_dev, "command ring init fail, err %d\n",
		    err);
		rv = ENOMEM;
		goto bad1;
	}

	err = xhci_ring_init(sc, &sc->sc_er, XHCI_EVENT_RING_TRBS,
	    XHCI_EVENT_RING_SEGMENTS_ALIGN);
	if (err) {
		aprint_error_dev(sc->sc_dev, "event ring init fail, err %d\n",
		    err);
		rv = ENOMEM;
		goto bad2;
	}

	usb_dma_t *dma;
	size_t size;
	size_t align;

	dma = &sc->sc_eventst_dma;
	size = roundup2(XHCI_EVENT_RING_SEGMENTS * XHCI_ERSTE_SIZE,
	    XHCI_EVENT_RING_SEGMENT_TABLE_ALIGN);
	KASSERTMSG(size <= (512 * 1024), "eventst size %zu too large", size);
	align = XHCI_EVENT_RING_SEGMENT_TABLE_ALIGN;
	err = usb_allocmem(sc->sc_bus.ub_dmatag, size, align,
	    USBMALLOC_COHERENT | USBMALLOC_ZERO, dma);
	if (err) {
		aprint_error_dev(sc->sc_dev, "eventst init fail, err %d\n",
		    err);
		rv = ENOMEM;
		goto bad3;
	}

	aprint_debug_dev(sc->sc_dev, "eventst: 0x%016jx %p %zx\n",
	    (uintmax_t)DMAADDR(&sc->sc_eventst_dma, 0),
	    KERNADDR(&sc->sc_eventst_dma, 0),
	    sc->sc_eventst_dma.udma_block->size);

	dma = &sc->sc_dcbaa_dma;
	size = (1 + sc->sc_maxslots) * sizeof(uint64_t);
	KASSERTMSG(size <= 2048, "dcbaa size %zu too large", size);
	align = XHCI_DEVICE_CONTEXT_BASE_ADDRESS_ARRAY_ALIGN;
	err = usb_allocmem(sc->sc_bus.ub_dmatag, size, align,
	    USBMALLOC_COHERENT | USBMALLOC_ZERO, dma);
	if (err) {
		aprint_error_dev(sc->sc_dev, "dcbaa init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad4;
	}
	aprint_debug_dev(sc->sc_dev, "dcbaa: 0x%016jx %p %zx\n",
	    (uintmax_t)DMAADDR(&sc->sc_dcbaa_dma, 0),
	    KERNADDR(&sc->sc_dcbaa_dma, 0),
	    sc->sc_dcbaa_dma.udma_block->size);

	if (sc->sc_maxspbuf != 0) {
		/*
		 * DCBA entry 0 hold the scratchbuf array pointer.
		 */
		*(uint64_t *)KERNADDR(dma, 0) =
		    htole64(DMAADDR(&sc->sc_spbufarray_dma, 0));
		usb_syncmem(dma, 0, size, BUS_DMASYNC_PREWRITE);
	}

	sc->sc_slots = kmem_zalloc(sizeof(*sc->sc_slots) * sc->sc_maxslots,
	    KM_SLEEP);
	if (sc->sc_slots == NULL) {
		aprint_error_dev(sc->sc_dev, "slots init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad;
	}

	sc->sc_xferpool = pool_cache_init(sizeof(struct xhci_xfer), 0, 0, 0,
	    "xhcixfer", NULL, IPL_USB, NULL, NULL, NULL);
	if (sc->sc_xferpool == NULL) {
		aprint_error_dev(sc->sc_dev, "pool_cache init fail, err %d\n",
		    err);
		rv = ENOMEM;
		goto bad;
	}

	cv_init(&sc->sc_command_cv, "xhcicmd");
	cv_init(&sc->sc_cmdbusy_cv, "xhcicmdq");
	mutex_init(&sc->sc_rhlock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);

	struct xhci_erste *erst;
	erst = KERNADDR(&sc->sc_eventst_dma, 0);
	erst[0].erste_0 = htole64(xhci_ring_trbp(sc->sc_er, 0));
	erst[0].erste_2 = htole32(sc->sc_er->xr_ntrb);
	erst[0].erste_3 = htole32(0);
	usb_syncmem(&sc->sc_eventst_dma, 0,
	    XHCI_ERSTE_SIZE * XHCI_EVENT_RING_SEGMENTS, BUS_DMASYNC_PREWRITE);

	xhci_rt_write_4(sc, XHCI_ERSTSZ(0), XHCI_EVENT_RING_SEGMENTS);
	xhci_rt_write_8(sc, XHCI_ERSTBA(0), DMAADDR(&sc->sc_eventst_dma, 0));
	xhci_rt_write_8(sc, XHCI_ERDP(0), xhci_ring_trbp(sc->sc_er, 0) |
	    XHCI_ERDP_BUSY);

	xhci_op_write_8(sc, XHCI_DCBAAP, DMAADDR(&sc->sc_dcbaa_dma, 0));
	xhci_op_write_8(sc, XHCI_CRCR, xhci_ring_trbp(sc->sc_cr, 0) |
	    sc->sc_cr->xr_cs);

	xhci_barrier(sc, BUS_SPACE_BARRIER_WRITE);

	HEXDUMP("eventst", KERNADDR(&sc->sc_eventst_dma, 0),
	    XHCI_ERSTE_SIZE * XHCI_EVENT_RING_SEGMENTS);

	if ((sc->sc_quirks & XHCI_DEFERRED_START) == 0)
		xhci_start(sc);

	return 0;

 bad:
	if (sc->sc_xferpool) {
		pool_cache_destroy(sc->sc_xferpool);
		sc->sc_xferpool = NULL;
	}

	if (sc->sc_slots) {
		kmem_free(sc->sc_slots, sizeof(*sc->sc_slots) *
		    sc->sc_maxslots);
		sc->sc_slots = NULL;
	}

	usb_freemem(&sc->sc_dcbaa_dma);
 bad4:
	usb_freemem(&sc->sc_eventst_dma);
 bad3:
	xhci_ring_free(sc, &sc->sc_er);
 bad2:
	xhci_ring_free(sc, &sc->sc_cr);
	i = sc->sc_maxspbuf;
 bad1:
	for (int j = 0; j < i; j++)
		usb_freemem(&sc->sc_spbuf_dma[j]);
	usb_freemem(&sc->sc_spbufarray_dma);

	return rv;
}

static inline bool
xhci_polling_p(struct xhci_softc * const sc)
{
	return sc->sc_bus.ub_usepolling || sc->sc_bus2.ub_usepolling;
}

int
xhci_intr(void *v)
{
	struct xhci_softc * const sc = v;
	int ret = 0;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	if (sc == NULL)
		return 0;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (sc->sc_dying || !device_has_power(sc->sc_dev))
		goto done;

	/* If we get an interrupt while polling, then just ignore it. */
	if (xhci_polling_p(sc)) {
#ifdef DIAGNOSTIC
		DPRINTFN(16, "ignored interrupt while polling", 0, 0, 0, 0);
#endif
		goto done;
	}

	ret = xhci_intr1(sc);
	if (ret) {
		KASSERT(sc->sc_child || sc->sc_child2);

		/*
		 * One of child busses could be already detached. It doesn't
		 * matter on which of the two the softintr is scheduled.
		 */
		if (sc->sc_child)
			usb_schedsoftintr(&sc->sc_bus);
		else
			usb_schedsoftintr(&sc->sc_bus2);
	}
done:
	mutex_spin_exit(&sc->sc_intr_lock);
	return ret;
}

int
xhci_intr1(struct xhci_softc * const sc)
{
	uint32_t usbsts;
	uint32_t iman;

	XHCIHIST_FUNC();

	usbsts = xhci_op_read_4(sc, XHCI_USBSTS);
	XHCIHIST_CALLARGS("USBSTS 0x%08jx", usbsts, 0, 0, 0);
	if ((usbsts & (XHCI_STS_HSE | XHCI_STS_EINT | XHCI_STS_PCD |
	    XHCI_STS_HCE)) == 0) {
		DPRINTFN(16, "ignored intr not for %jd",
		    device_unit(sc->sc_dev), 0, 0, 0);
		return 0;
	}

	/*
	 * Clear EINT and other transient flags, to not misenterpret
	 * next shared interrupt. Also, to avoid race, EINT must be cleared
	 * before XHCI_IMAN_INTR_PEND is cleared.
	 */
	xhci_op_write_4(sc, XHCI_USBSTS, usbsts & XHCI_STS_RSVDP0);

#ifdef XHCI_DEBUG
	usbsts = xhci_op_read_4(sc, XHCI_USBSTS);
	DPRINTFN(16, "USBSTS 0x%08jx", usbsts, 0, 0, 0);
#endif

	iman = xhci_rt_read_4(sc, XHCI_IMAN(0));
	DPRINTFN(16, "IMAN0 0x%08jx", iman, 0, 0, 0);
	iman |= XHCI_IMAN_INTR_PEND;
	xhci_rt_write_4(sc, XHCI_IMAN(0), iman);

#ifdef XHCI_DEBUG
	iman = xhci_rt_read_4(sc, XHCI_IMAN(0));
	DPRINTFN(16, "IMAN0 0x%08jx", iman, 0, 0, 0);
	usbsts = xhci_op_read_4(sc, XHCI_USBSTS);
	DPRINTFN(16, "USBSTS 0x%08jx", usbsts, 0, 0, 0);
#endif

	return 1;
}

/*
 * 3 port speed types used in USB stack
 *
 * usbdi speed
 *	definition: USB_SPEED_* in usb.h
 *	They are used in struct usbd_device in USB stack.
 *	ioctl interface uses these values too.
 * port_status speed
 *	definition: UPS_*_SPEED in usb.h
 *	They are used in usb_port_status_t and valid only for USB 2.0.
 *	Speed value is always 0 for Super Speed or more, and dwExtPortStatus
 *	of usb_port_status_ext_t indicates port speed.
 *	Note that some 3.0 values overlap with 2.0 values.
 *	(e.g. 0x200 means UPS_POER_POWER_SS in SS and
 *	            means UPS_LOW_SPEED in HS.)
 *	port status returned from hub also uses these values.
 *	On NetBSD UPS_OTHER_SPEED indicates port speed is super speed
 *	or more.
 * xspeed:
 *	definition: Protocol Speed ID (PSI) (xHCI 1.1 7.2.1)
 *	They are used in only slot context and PORTSC reg of xhci.
 *	The difference between usbdi speed and xspeed is
 *	that FS and LS values are swapped.
 */

/* convert usbdi speed to xspeed */
static int
xhci_speed2xspeed(int speed)
{
	switch (speed) {
	case USB_SPEED_LOW:	return 2;
	case USB_SPEED_FULL:	return 1;
	default:		return speed;
	}
}

#if 0
/* convert xspeed to usbdi speed */
static int
xhci_xspeed2speed(int xspeed)
{
	switch (xspeed) {
	case 1: return USB_SPEED_FULL;
	case 2: return USB_SPEED_LOW;
	default: return xspeed;
	}
}
#endif

/* convert xspeed to port status speed */
static int
xhci_xspeed2psspeed(int xspeed)
{
	switch (xspeed) {
	case 0: return 0;
	case 1: return UPS_FULL_SPEED;
	case 2: return UPS_LOW_SPEED;
	case 3: return UPS_HIGH_SPEED;
	default: return UPS_OTHER_SPEED;
	}
}

/*
 * Construct input contexts and issue TRB to open pipe.
 */
static usbd_status
xhci_configure_endpoint(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
#ifdef USB_DEBUG
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);
#endif
	struct xhci_soft_trb trb;
	usbd_status err;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju dci %ju epaddr 0x%02jx attr 0x%02jx",
	    xs->xs_idx, dci, pipe->up_endpoint->ue_edesc->bEndpointAddress,
	    pipe->up_endpoint->ue_edesc->bmAttributes);

	/* XXX ensure input context is available? */

	memset(xhci_slot_get_icv(sc, xs, 0), 0, sc->sc_pgsz);

	/* set up context */
	xhci_setup_ctx(pipe);

	HEXDUMP("input control context", xhci_slot_get_icv(sc, xs, 0),
	    sc->sc_ctxsz * 1);
	HEXDUMP("input endpoint context", xhci_slot_get_icv(sc, xs,
	    xhci_dci_to_ici(dci)), sc->sc_ctxsz * 1);

	trb.trb_0 = xhci_slot_get_icp(sc, xs, 0);
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_CONFIGURE_EP);

	err = xhci_do_command(sc, &trb, USBD_DEFAULT_TIMEOUT);

	usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);
	HEXDUMP("output context", xhci_slot_get_dcv(sc, xs, dci),
	    sc->sc_ctxsz * 1);

	return err;
}

#if 0
static usbd_status
xhci_unconfigure_endpoint(struct usbd_pipe *pipe)
{
#ifdef USB_DEBUG
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
#endif

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju", xs->xs_idx, 0, 0, 0);

	return USBD_NORMAL_COMPLETION;
}
#endif

/* 4.6.8, 6.4.3.7 */
static void
xhci_reset_endpoint(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);
	struct xhci_soft_trb trb;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju dci %ju", xs->xs_idx, dci, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	trb.trb_0 = 0;
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_EP_SET(dci) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_RESET_EP);

	if (xhci_do_command_locked(sc, &trb, USBD_DEFAULT_TIMEOUT)) {
		device_printf(sc->sc_dev, "%s: endpoint 0x%x: timed out\n",
		    __func__, pipe->up_endpoint->ue_edesc->bEndpointAddress);
	}
}

/*
 * 4.6.9, 6.4.3.8
 * Stop execution of TDs on xfer ring.
 * Should be called with sc_lock held.
 */
static usbd_status
xhci_stop_endpoint_cmd(struct xhci_softc *sc, struct xhci_slot *xs, u_int dci,
    uint32_t trb3flags)
{
	struct xhci_soft_trb trb;
	usbd_status err;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju dci %ju", xs->xs_idx, dci, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	trb.trb_0 = 0;
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_EP_SET(dci) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_STOP_EP) |
	    trb3flags;

	err = xhci_do_command_locked(sc, &trb, USBD_DEFAULT_TIMEOUT);

	return err;
}

static usbd_status
xhci_stop_endpoint(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju dci %ju", xs->xs_idx, dci, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));

	return xhci_stop_endpoint_cmd(sc, xs, dci, 0);
}

/*
 * Set TR Dequeue Pointer.
 * xHCI 1.1  4.6.10  6.4.3.9
 * Purge all of the TRBs on ring and reinitialize ring.
 * Set TR dequeue Pointer to 0 and Cycle State to 1.
 * EPSTATE of endpoint must be ERROR or STOPPED, otherwise CONTEXT_STATE
 * error will be generated.
 */
static void
xhci_set_dequeue(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const xr = xs->xs_xr[dci];
	struct xhci_soft_trb trb;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju dci %ju", xs->xs_idx, dci, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xr != NULL);

	xhci_host_dequeue(xr);

	/* set DCS */
	trb.trb_0 = xhci_ring_trbp(xr, 0) | 1; /* XXX */
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_EP_SET(dci) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_SET_TR_DEQUEUE);

	if (xhci_do_command_locked(sc, &trb, USBD_DEFAULT_TIMEOUT)) {
		device_printf(sc->sc_dev, "%s: endpoint 0x%x: timed out\n",
		    __func__, pipe->up_endpoint->ue_edesc->bEndpointAddress);
	}
}

/*
 * Open new pipe: called from usbd_setup_pipe_flags.
 * Fills methods of pipe.
 * If pipe is not for ep0, calls configure_endpoint.
 */
static usbd_status
xhci_open(struct usbd_pipe *pipe)
{
	struct usbd_device * const dev = pipe->up_dev;
	struct xhci_pipe * const xpipe = (struct xhci_pipe *)pipe;
	struct xhci_softc * const sc = XHCI_BUS2SC(dev->ud_bus);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	usb_endpoint_descriptor_t * const ed = pipe->up_endpoint->ue_edesc;
	const u_int dci = xhci_ep_get_dci(ed);
	const uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	usbd_status err;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("addr %jd depth %jd port %jd speed %jd", dev->ud_addr,
	    dev->ud_depth, dev->ud_powersrc->up_portno, dev->ud_speed);
	DPRINTFN(1, " dci %ju type 0x%02jx epaddr 0x%02jx attr 0x%02jx",
	    xhci_ep_get_dci(ed), ed->bDescriptorType, ed->bEndpointAddress,
	    ed->bmAttributes);
	DPRINTFN(1, " mps %ju ival %ju", UGETW(ed->wMaxPacketSize),
	    ed->bInterval, 0, 0);

	if (sc->sc_dying)
		return USBD_IOERROR;

	/* Root Hub */
	if (dev->ud_depth == 0 && dev->ud_powersrc->up_portno == 0) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &xhci_root_intr_methods;
			break;
		default:
			pipe->up_methods = NULL;
			DPRINTFN(0, "bad bEndpointAddress 0x%02jx",
			    ed->bEndpointAddress, 0, 0, 0);
			return USBD_INVAL;
		}
		return USBD_NORMAL_COMPLETION;
	}

	usb_init_task(&xpipe->xp_async_task, xhci_pipe_restart_async_task,
	    pipe, USB_TASKQ_MPSAFE);

	switch (xfertype) {
	case UE_CONTROL:
		pipe->up_methods = &xhci_device_ctrl_methods;
		break;
	case UE_ISOCHRONOUS:
		pipe->up_methods = &xhci_device_isoc_methods;
		pipe->up_serialise = false;
		xpipe->xp_isoc_next = -1;
		break;
	case UE_BULK:
		pipe->up_methods = &xhci_device_bulk_methods;
		break;
	case UE_INTERRUPT:
		pipe->up_methods = &xhci_device_intr_methods;
		break;
	default:
		return USBD_IOERROR;
		break;
	}

	KASSERT(xs != NULL);
	KASSERT(xs->xs_xr[dci] == NULL);

	/* allocate transfer ring */
	err = xhci_ring_init(sc, &xs->xs_xr[dci], XHCI_TRANSFER_RING_TRBS,
	    XHCI_TRB_ALIGN);
	if (err) {
		DPRINTFN(1, "ring alloc failed %jd", err, 0, 0, 0);
		return err;
	}

	if (ed->bEndpointAddress != USB_CONTROL_ENDPOINT)
		return xhci_configure_endpoint(pipe);

	return USBD_NORMAL_COMPLETION;
}

/*
 * Closes pipe, called from usbd_kill_pipe via close methods.
 * If the endpoint to be closed is ep0, disable_slot.
 * Should be called with sc_lock held.
 */
static void
xhci_close_pipe(struct usbd_pipe *pipe)
{
	struct xhci_pipe * const xp =
	    container_of(pipe, struct xhci_pipe, xp_pipe);
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	usb_endpoint_descriptor_t * const ed = pipe->up_endpoint->ue_edesc;
	const u_int dci = xhci_ep_get_dci(ed);
	struct xhci_soft_trb trb;
	uint32_t *cp;

	XHCIHIST_FUNC();

	usb_rem_task_wait(pipe->up_dev, &xp->xp_async_task, USB_TASKQ_HC,
	    &sc->sc_lock);

	if (sc->sc_dying)
		return;

	/* xs is uninitialized before xhci_init_slot */
	if (xs == NULL || xs->xs_idx == 0)
		return;

	XHCIHIST_CALLARGS("pipe %#jx slot %ju dci %ju",
	    (uintptr_t)pipe, xs->xs_idx, dci, 0);

	KASSERTMSG(!cpu_intr_p() && !cpu_softintr_p(), "called from intr ctx");
	KASSERT(mutex_owned(&sc->sc_lock));

	if (pipe->up_dev->ud_depth == 0)
		return;

	if (dci == XHCI_DCI_EP_CONTROL) {
		DPRINTFN(4, "closing ep0", 0, 0, 0, 0);
		/* This frees all rings */
		xhci_disable_slot(sc, xs->xs_idx);
		return;
	}

	if (xhci_get_epstate(sc, xs, dci) != XHCI_EPSTATE_STOPPED)
		(void)xhci_stop_endpoint(pipe);

	/*
	 * set appropriate bit to be dropped.
	 * don't set DC bit to 1, otherwise all endpoints
	 * would be deconfigured.
	 */
	cp = xhci_slot_get_icv(sc, xs, XHCI_ICI_INPUT_CONTROL);
	cp[0] = htole32(XHCI_INCTX_0_DROP_MASK(dci));
	cp[1] = htole32(0);

	/* XXX should be most significant one, not dci? */
	cp = xhci_slot_get_icv(sc, xs, xhci_dci_to_ici(XHCI_DCI_SLOT));
	cp[0] = htole32(XHCI_SCTX_0_CTX_NUM_SET(dci));

	/* configure ep context performs an implicit dequeue */
	xhci_host_dequeue(xs->xs_xr[dci]);

	/* sync input contexts before they are read from memory */
	usb_syncmem(&xs->xs_ic_dma, 0, sc->sc_pgsz, BUS_DMASYNC_PREWRITE);

	trb.trb_0 = xhci_slot_get_icp(sc, xs, 0);
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_CONFIGURE_EP);

	(void)xhci_do_command_locked(sc, &trb, USBD_DEFAULT_TIMEOUT);
	usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);

	xhci_ring_free(sc, &xs->xs_xr[dci]);
	xs->xs_xr[dci] = NULL;
}

/*
 * Abort transfer.  Must be called with sc_lock held.  Releases and
 * reacquires sc_lock to sleep until hardware acknowledges abort.
 */
static void
xhci_abortx(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC();
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);

	XHCIHIST_CALLARGS("xfer %#jx pipe %#jx",
	    (uintptr_t)xfer, (uintptr_t)xfer->ux_pipe, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERTMSG((xfer->ux_status == USBD_CANCELLED ||
		xfer->ux_status == USBD_TIMEOUT),
	    "bad abort status: %d", xfer->ux_status);

	xhci_pipe_restart(xfer->ux_pipe);

	DPRINTFN(14, "end", 0, 0, 0, 0);
}

static void
xhci_host_dequeue(struct xhci_ring * const xr)
{
	/* When dequeueing the controller, update our struct copy too */
	memset(xr->xr_trb, 0, xr->xr_ntrb * XHCI_TRB_SIZE);
	usb_syncmem(&xr->xr_dma, 0, xr->xr_ntrb * XHCI_TRB_SIZE,
	    BUS_DMASYNC_PREWRITE);
	memset(xr->xr_cookies, 0, xr->xr_ntrb * sizeof(*xr->xr_cookies));

	xr->xr_ep = 0;
	xr->xr_cs = 1;
}

/*
 * Recover STALLed endpoint, or stop endpoint to abort a pipe.
 * xHCI 1.1 sect 4.10.2.1
 * Issue RESET_EP to recover halt condition and SET_TR_DEQUEUE to remove
 * all transfers on transfer ring.
 */
static void
xhci_pipe_restart(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("pipe %#jx slot %ju dci %ju",
	    (uintptr_t)pipe, xs->xs_idx, dci, 0);

	KASSERT(xhci_polling_p(sc) || mutex_owned(&sc->sc_lock));

	/*
	 * - If the endpoint is halted, indicating a stall, reset it.
	 * - If the endpoint is stopped, we're already good.
	 * - Otherwise, someone wanted to abort the pipe, so stop the
	 *   endpoint.
	 *
	 * In any case, clear the ring.
	 */
	switch (xhci_get_epstate(sc, xs, dci)) {
	case XHCI_EPSTATE_HALTED:
		xhci_reset_endpoint(pipe);
		break;
	case XHCI_EPSTATE_STOPPED:
		break;
	default:
		xhci_stop_endpoint(pipe);
		break;
	}

	switch (xhci_get_epstate(sc, xs, dci)) {
	case XHCI_EPSTATE_STOPPED:
		break;
	case XHCI_EPSTATE_ERROR:
		device_printf(sc->sc_dev, "endpoint 0x%x error\n",
		    pipe->up_endpoint->ue_edesc->bEndpointAddress);
		break;
	default:
		device_printf(sc->sc_dev, "endpoint 0x%x failed to stop\n",
		    pipe->up_endpoint->ue_edesc->bEndpointAddress);
	}

	xhci_set_dequeue(pipe);

	DPRINTFN(4, "ends", 0, 0, 0, 0);
}

static void
xhci_pipe_restart_async_task(void *cookie)
{
	struct usbd_pipe * const pipe = cookie;
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];
	struct usbd_xfer *xfer;

	mutex_enter(&sc->sc_lock);

	xhci_pipe_restart(pipe);

	/*
	 * We halted our own queue because it stalled.  Mark it no
	 * longer halted and start issuing queued transfers again.
	 */
	tr->is_halted = false;
	xfer = SIMPLEQ_FIRST(&pipe->up_queue);
	if (xfer)
		(*pipe->up_methods->upm_start)(xfer);

	mutex_exit(&sc->sc_lock);
}

static void
xhci_pipe_restart_async(struct usbd_pipe *pipe)
{
	struct xhci_pipe * const xp =
	    container_of(pipe, struct xhci_pipe, xp_pipe);
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct xhci_slot * const xs = pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("pipe %#jx", (uintptr_t)pipe, 0, 0, 0);

	KASSERT(xhci_polling_p(sc) || mutex_owned(&sc->sc_lock));

	tr->is_halted = true;
	usb_add_task(pipe->up_dev, &xp->xp_async_task, USB_TASKQ_HC);

	DPRINTFN(4, "ends", 0, 0, 0, 0);
}

/* Process roothub port status/change events and notify to uhub_intr. */
static void
xhci_rhpsc(struct xhci_softc * const sc, u_int ctlrport)
{
	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("xhci%jd: port %ju status change",
	   device_unit(sc->sc_dev), ctlrport, 0, 0);

	if (ctlrport > sc->sc_maxports)
		return;

	const size_t bn = xhci_ctlrport2bus(sc, ctlrport);
	const size_t rhp = xhci_ctlrport2rhport(sc, ctlrport);
	struct usbd_xfer * const xfer = sc->sc_intrxfer[bn];

	DPRINTFN(4, "xhci%jd: bus %jd bp %ju xfer %#jx status change",
	    device_unit(sc->sc_dev), bn, rhp, (uintptr_t)xfer);

	if (xfer == NULL)
		return;
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	uint8_t *p = xfer->ux_buf;
	memset(p, 0, xfer->ux_length);
	p[rhp / NBBY] |= 1 << (rhp % NBBY);
	xfer->ux_actlen = xfer->ux_length;
	xfer->ux_status = USBD_NORMAL_COMPLETION;
	usb_transfer_complete(xfer);
}

/* Process Transfer Events */
static void
xhci_event_transfer(struct xhci_softc * const sc,
    const struct xhci_trb * const trb)
{
	uint64_t trb_0;
	uint32_t trb_2, trb_3;
	uint8_t trbcode;
	u_int slot, dci;
	struct xhci_slot *xs;
	struct xhci_ring *xr;
	struct xhci_xfer *xx;
	struct usbd_xfer *xfer;
	usbd_status err;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	trb_0 = le64toh(trb->trb_0);
	trb_2 = le32toh(trb->trb_2);
	trb_3 = le32toh(trb->trb_3);
	trbcode = XHCI_TRB_2_ERROR_GET(trb_2);
	slot = XHCI_TRB_3_SLOT_GET(trb_3);
	dci = XHCI_TRB_3_EP_GET(trb_3);
	xs = &sc->sc_slots[slot];
	xr = xs->xs_xr[dci];

	/* sanity check */
	KASSERT(xr != NULL);
	KASSERTMSG(xs->xs_idx != 0 && xs->xs_idx <= sc->sc_maxslots,
	    "invalid xs_idx %u slot %u", xs->xs_idx, slot);

	int idx = 0;
	if ((trb_3 & XHCI_TRB_3_ED_BIT) == 0) {
		if (xhci_trb_get_idx(xr, trb_0, &idx)) {
			DPRINTFN(0, "invalid trb_0 %#jx", trb_0, 0, 0, 0);
			return;
		}
		xx = xr->xr_cookies[idx];

		/* clear cookie of consumed TRB */
		xr->xr_cookies[idx] = NULL;

		/*
		 * xx is NULL if pipe is opened but xfer is not started.
		 * It happens when stopping idle pipe.
		 */
		if (xx == NULL || trbcode == XHCI_TRB_ERROR_LENGTH) {
			DPRINTFN(1, "Ignore #%ju: cookie %#jx cc %ju dci %ju",
			    idx, (uintptr_t)xx, trbcode, dci);
			DPRINTFN(1, " orig TRB %#jx type %ju", trb_0,
			    XHCI_TRB_3_TYPE_GET(le32toh(xr->xr_trb[idx].trb_3)),
			    0, 0);
			return;
		}
	} else {
		/* When ED != 0, trb_0 is virtual addr of struct xhci_xfer. */
		xx = (void *)(uintptr_t)(trb_0 & ~0x3);
	}
	/* XXX this may not happen */
	if (xx == NULL) {
		DPRINTFN(1, "xfer done: xx is NULL", 0, 0, 0, 0);
		return;
	}
	xfer = &xx->xx_xfer;
	/* XXX this may happen when detaching */
	if (xfer == NULL) {
		DPRINTFN(1, "xx(%#jx)->xx_xfer is NULL trb_0 %#jx",
		    (uintptr_t)xx, trb_0, 0, 0);
		return;
	}
	DPRINTFN(14, "xfer %#jx", (uintptr_t)xfer, 0, 0, 0);
	/* XXX I dunno why this happens */
	KASSERTMSG(xfer->ux_pipe != NULL, "xfer(%p)->ux_pipe is NULL", xfer);

	if (!xfer->ux_pipe->up_repeat &&
	    SIMPLEQ_EMPTY(&xfer->ux_pipe->up_queue)) {
		DPRINTFN(1, "xfer(%#jx)->pipe not queued", (uintptr_t)xfer,
		    0, 0, 0);
		return;
	}

	const uint8_t xfertype =
	    UE_GET_XFERTYPE(xfer->ux_pipe->up_endpoint->ue_edesc->bmAttributes);

	/* 4.11.5.2 Event Data TRB */
	if ((trb_3 & XHCI_TRB_3_ED_BIT) != 0) {
		DPRINTFN(14, "transfer Event Data: 0x%016jx 0x%08jx"
		    " %02jx", trb_0, XHCI_TRB_2_REM_GET(trb_2), trbcode, 0);
		if ((trb_0 & 0x3) == 0x3) {
			xfer->ux_actlen = XHCI_TRB_2_REM_GET(trb_2);
		}
	}

	switch (trbcode) {
	case XHCI_TRB_ERROR_SHORT_PKT:
	case XHCI_TRB_ERROR_SUCCESS:
		/*
		 * A ctrl transfer can generate two events if it has a Data
		 * stage.  A short data stage can be OK and should not
		 * complete the transfer as the status stage needs to be
		 * performed.
		 *
		 * Note: Data and Status stage events point at same xfer.
		 * ux_actlen and ux_dmabuf will be passed to
		 * usb_transfer_complete after the Status stage event.
		 *
		 * It can be distingished which stage generates the event:
		 * + by checking least 3 bits of trb_0 if ED==1.
		 *   (see xhci_device_ctrl_start).
		 * + by checking the type of original TRB if ED==0.
		 *
		 * In addition, intr, bulk, and isoc transfer currently
		 * consists of single TD, so the "skip" is not needed.
		 * ctrl xfer uses EVENT_DATA, and others do not.
		 * Thus driver can switch the flow by checking ED bit.
		 */
		if (xfertype == UE_ISOCHRONOUS) {
			xfer->ux_frlengths[xx->xx_isoc_done] -=
			    XHCI_TRB_2_REM_GET(trb_2);
			xfer->ux_actlen += xfer->ux_frlengths[xx->xx_isoc_done];
			if (++xx->xx_isoc_done < xfer->ux_nframes)
				return;
		} else
		if ((trb_3 & XHCI_TRB_3_ED_BIT) == 0) {
			if (xfer->ux_actlen == 0)
				xfer->ux_actlen = xfer->ux_length -
				    XHCI_TRB_2_REM_GET(trb_2);
			if (XHCI_TRB_3_TYPE_GET(le32toh(xr->xr_trb[idx].trb_3))
			    == XHCI_TRB_TYPE_DATA_STAGE) {
				return;
			}
		} else if ((trb_0 & 0x3) == 0x3) {
			return;
		}
		err = USBD_NORMAL_COMPLETION;
		break;
	case XHCI_TRB_ERROR_STOPPED:
	case XHCI_TRB_ERROR_LENGTH:
	case XHCI_TRB_ERROR_STOPPED_SHORT:
		err = USBD_IOERROR;
		break;
	case XHCI_TRB_ERROR_STALL:
	case XHCI_TRB_ERROR_BABBLE:
		DPRINTFN(1, "ERR %ju slot %ju dci %ju", trbcode, slot, dci, 0);
		xhci_pipe_restart_async(xfer->ux_pipe);
		err = USBD_STALLED;
		break;
	default:
		DPRINTFN(1, "ERR %ju slot %ju dci %ju", trbcode, slot, dci, 0);
		err = USBD_IOERROR;
		break;
	}

	if ((trb_3 & XHCI_TRB_3_ED_BIT) == 0 ||
	    (trb_0 & 0x3) == 0x0) {
		/*
		 * Try to claim this xfer for completion.  If it has
		 * already completed or aborted, drop it on the floor.
		 */
		if (!usbd_xfer_trycomplete(xfer))
			return;

		/* Set the status.  */
		xfer->ux_status = err;

		usb_transfer_complete(xfer);
	}
}

/* Process Command complete events */
static void
xhci_event_cmd(struct xhci_softc * const sc, const struct xhci_trb * const trb)
{
	uint64_t trb_0;
	uint32_t trb_2, trb_3;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	trb_0 = le64toh(trb->trb_0);
	trb_2 = le32toh(trb->trb_2);
	trb_3 = le32toh(trb->trb_3);

	if (trb_0 == sc->sc_command_addr) {
		sc->sc_resultpending = false;

		sc->sc_result_trb.trb_0 = trb_0;
		sc->sc_result_trb.trb_2 = trb_2;
		sc->sc_result_trb.trb_3 = trb_3;
		if (XHCI_TRB_2_ERROR_GET(trb_2) !=
		    XHCI_TRB_ERROR_SUCCESS) {
			DPRINTFN(1, "command completion "
			    "failure: 0x%016jx 0x%08jx 0x%08jx",
			    trb_0, trb_2, trb_3, 0);
		}
		cv_signal(&sc->sc_command_cv);
	} else {
		DPRINTFN(1, "spurious event: %#jx 0x%016jx "
		    "0x%08jx 0x%08jx", (uintptr_t)trb, trb_0, trb_2, trb_3);
	}
}

/*
 * Process events.
 * called from xhci_softintr
 */
static void
xhci_handle_event(struct xhci_softc * const sc,
    const struct xhci_trb * const trb)
{
	uint64_t trb_0;
	uint32_t trb_2, trb_3;

	XHCIHIST_FUNC();

	trb_0 = le64toh(trb->trb_0);
	trb_2 = le32toh(trb->trb_2);
	trb_3 = le32toh(trb->trb_3);

	XHCIHIST_CALLARGS("event: %#jx 0x%016jx 0x%08jx 0x%08jx",
	    (uintptr_t)trb, trb_0, trb_2, trb_3);

	/*
	 * 4.11.3.1, 6.4.2.1
	 * TRB Pointer is invalid for these completion codes.
	 */
	switch (XHCI_TRB_2_ERROR_GET(trb_2)) {
	case XHCI_TRB_ERROR_RING_UNDERRUN:
	case XHCI_TRB_ERROR_RING_OVERRUN:
	case XHCI_TRB_ERROR_VF_RING_FULL:
		return;
	default:
		if (trb_0 == 0) {
			return;
		}
		break;
	}

	switch (XHCI_TRB_3_TYPE_GET(trb_3)) {
	case XHCI_TRB_EVENT_TRANSFER:
		xhci_event_transfer(sc, trb);
		break;
	case XHCI_TRB_EVENT_CMD_COMPLETE:
		xhci_event_cmd(sc, trb);
		break;
	case XHCI_TRB_EVENT_PORT_STS_CHANGE:
		xhci_rhpsc(sc, (uint32_t)((trb_0 >> 24) & 0xff));
		break;
	default:
		break;
	}
}

static void
xhci_softintr(void *v)
{
	struct usbd_bus * const bus = v;
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);
	struct xhci_ring * const er = sc->sc_er;
	struct xhci_trb *trb;
	int i, j, k;

	XHCIHIST_FUNC();

	KASSERT(xhci_polling_p(sc) || mutex_owned(&sc->sc_lock));

	i = er->xr_ep;
	j = er->xr_cs;

	XHCIHIST_CALLARGS("er: xr_ep %jd xr_cs %jd", i, j, 0, 0);

	while (1) {
		usb_syncmem(&er->xr_dma, XHCI_TRB_SIZE * i, XHCI_TRB_SIZE,
		    BUS_DMASYNC_POSTREAD);
		trb = &er->xr_trb[i];
		k = (le32toh(trb->trb_3) & XHCI_TRB_3_CYCLE_BIT) ? 1 : 0;

		if (j != k)
			break;

		xhci_handle_event(sc, trb);

		i++;
		if (i == er->xr_ntrb) {
			i = 0;
			j ^= 1;
		}
	}

	er->xr_ep = i;
	er->xr_cs = j;

	xhci_rt_write_8(sc, XHCI_ERDP(0), xhci_ring_trbp(er, er->xr_ep) |
	    XHCI_ERDP_BUSY);

	DPRINTFN(16, "ends", 0, 0, 0, 0);

	return;
}

static void
xhci_poll(struct usbd_bus *bus)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	mutex_enter(&sc->sc_intr_lock);
	int ret = xhci_intr1(sc);
	if (ret) {
		xhci_softintr(bus);
	}
	mutex_exit(&sc->sc_intr_lock);

	return;
}

static struct usbd_xfer *
xhci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);
	struct xhci_xfer *xx;
	u_int ntrbs;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	ntrbs = uimax(3, nframes);
	const size_t trbsz = sizeof(*xx->xx_trb) * ntrbs;

	xx = pool_cache_get(sc->sc_xferpool, PR_WAITOK);
	if (xx != NULL) {
		memset(xx, 0, sizeof(*xx));
		if (ntrbs > 0) {
			xx->xx_trb = kmem_alloc(trbsz, KM_SLEEP);
			xx->xx_ntrb = ntrbs;
		}
#ifdef DIAGNOSTIC
		xx->xx_xfer.ux_state = XFER_BUSY;
#endif
	}

	return &xx->xx_xfer;
}

static void
xhci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);
	struct xhci_xfer * const xx = XHCI_XFER2XXFER(xfer);

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

#ifdef DIAGNOSTIC
	if (xfer->ux_state != XFER_BUSY &&
	    xfer->ux_status != USBD_NOT_STARTED) {
		DPRINTFN(0, "xfer=%#jx not busy, 0x%08jx",
		    (uintptr_t)xfer, xfer->ux_state, 0, 0);
	}
	xfer->ux_state = XFER_FREE;
#endif
	if (xx->xx_ntrb > 0) {
		kmem_free(xx->xx_trb, xx->xx_ntrb * sizeof(*xx->xx_trb));
		xx->xx_trb = NULL;
		xx->xx_ntrb = 0;
	}
	pool_cache_put(sc->sc_xferpool, xx);
}

static bool
xhci_dying(struct usbd_bus *bus)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);

	return sc->sc_dying;
}

static void
xhci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

extern uint32_t usb_cookie_no;

/*
 * xHCI 4.3
 * Called when uhub_explore finds a new device (via usbd_new_device).
 * Port initialization and speed detection (4.3.1) are already done in uhub.c.
 * This function does:
 *   Allocate and construct dev structure of default endpoint (ep0).
 *   Allocate and open pipe of ep0.
 *   Enable slot and initialize slot context.
 *   Set Address.
 *   Read initial device descriptor.
 *   Determine initial MaxPacketSize (mps) by speed.
 *   Read full device descriptor.
 *   Register this device.
 * Finally state of device transitions ADDRESSED.
 */
static usbd_status
xhci_new_device(device_t parent, struct usbd_bus *bus, int depth,
    int speed, int port, struct usbd_port *up)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);
	struct usbd_device *dev;
	usbd_status err;
	usb_device_descriptor_t *dd;
	struct xhci_slot *xs;
	uint32_t *cp;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("port %ju depth %ju speed %ju up %#jx",
	    port, depth, speed, (uintptr_t)up);

	KASSERT(KERNEL_LOCKED_P());

	dev = kmem_zalloc(sizeof(*dev), KM_SLEEP);
	dev->ud_bus = bus;
	dev->ud_quirks = &usbd_no_quirk;
	dev->ud_addr = 0;
	dev->ud_ddesc.bMaxPacketSize = 0;
	dev->ud_depth = depth;
	dev->ud_powersrc = up;
	dev->ud_myhub = up->up_parent;
	dev->ud_speed = speed;
	dev->ud_langid = USBD_NOLANG;
	dev->ud_cookie.cookie = ++usb_cookie_no;

	/* Set up default endpoint handle. */
	dev->ud_ep0.ue_edesc = &dev->ud_ep0desc;
	/* doesn't matter, just don't let it uninitialized */
	dev->ud_ep0.ue_toggle = 0;

	/* Set up default endpoint descriptor. */
	dev->ud_ep0desc.bLength = USB_ENDPOINT_DESCRIPTOR_SIZE;
	dev->ud_ep0desc.bDescriptorType = UDESC_ENDPOINT;
	dev->ud_ep0desc.bEndpointAddress = USB_CONTROL_ENDPOINT;
	dev->ud_ep0desc.bmAttributes = UE_CONTROL;
	dev->ud_ep0desc.bInterval = 0;

	/* 4.3,  4.8.2.1 */
	switch (speed) {
	case USB_SPEED_SUPER:
	case USB_SPEED_SUPER_PLUS:
		USETW(dev->ud_ep0desc.wMaxPacketSize, USB_3_MAX_CTRL_PACKET);
		break;
	case USB_SPEED_FULL:
		/* XXX using 64 as initial mps of ep0 in FS */
	case USB_SPEED_HIGH:
		USETW(dev->ud_ep0desc.wMaxPacketSize, USB_2_MAX_CTRL_PACKET);
		break;
	case USB_SPEED_LOW:
	default:
		USETW(dev->ud_ep0desc.wMaxPacketSize, USB_MAX_IPACKET);
		break;
	}

	up->up_dev = dev;

	dd = &dev->ud_ddesc;

	if (depth == 0 && port == 0) {
		KASSERT(bus->ub_devices[USB_ROOTHUB_INDEX] == NULL);
		bus->ub_devices[USB_ROOTHUB_INDEX] = dev;

		/* Establish the default pipe. */
		err = usbd_setup_pipe(dev, 0, &dev->ud_ep0,
		    USBD_DEFAULT_INTERVAL, &dev->ud_pipe0);
		if (err) {
			DPRINTFN(1, "setup default pipe failed %jd", err,0,0,0);
			goto bad;
		}
		err = usbd_get_initial_ddesc(dev, dd);
		if (err) {
			DPRINTFN(1, "get_initial_ddesc %ju", err, 0, 0, 0);
			goto bad;
		}
	} else {
		uint8_t slot = 0;

		/* 4.3.2 */
		err = xhci_enable_slot(sc, &slot);
		if (err) {
			DPRINTFN(1, "enable slot %ju", err, 0, 0, 0);
			goto bad;
		}

		xs = &sc->sc_slots[slot];
		dev->ud_hcpriv = xs;

		/* 4.3.3 initialize slot structure */
		err = xhci_init_slot(dev, slot);
		if (err) {
			DPRINTFN(1, "init slot %ju", err, 0, 0, 0);
			dev->ud_hcpriv = NULL;
			/*
			 * We have to disable_slot here because
			 * xs->xs_idx == 0 when xhci_init_slot fails,
			 * in that case usbd_remove_dev won't work.
			 */
			mutex_enter(&sc->sc_lock);
			xhci_disable_slot(sc, slot);
			mutex_exit(&sc->sc_lock);
			goto bad;
		}

		/*
		 * We have to establish the default pipe _after_ slot
		 * structure has been prepared.
		 */
		err = usbd_setup_pipe(dev, 0, &dev->ud_ep0,
		    USBD_DEFAULT_INTERVAL, &dev->ud_pipe0);
		if (err) {
			DPRINTFN(1, "setup default pipe failed %jd", err, 0, 0,
			    0);
			goto bad;
		}

		/* 4.3.4 Address Assignment */
		err = xhci_set_address(dev, slot, false);
		if (err) {
			DPRINTFN(1, "failed! to set address: %ju", err, 0, 0, 0);
			goto bad;
		}

		/* Allow device time to set new address */
		usbd_delay_ms(dev, USB_SET_ADDRESS_SETTLE);

		usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);
		cp = xhci_slot_get_dcv(sc, xs, XHCI_DCI_SLOT);
		HEXDUMP("slot context", cp, sc->sc_ctxsz);
		uint8_t addr = XHCI_SCTX_3_DEV_ADDR_GET(le32toh(cp[3]));
		DPRINTFN(4, "device address %ju", addr, 0, 0, 0);
		/*
		 * XXX ensure we know when the hardware does something
		 * we can't yet cope with
		 */
		KASSERTMSG(addr >= 1 && addr <= 127, "addr %d", addr);
		dev->ud_addr = addr;

		KASSERTMSG(bus->ub_devices[usb_addr2dindex(dev->ud_addr)] == NULL,
		    "addr %d already allocated", dev->ud_addr);
		/*
		 * The root hub is given its own slot
		 */
		bus->ub_devices[usb_addr2dindex(dev->ud_addr)] = dev;

		err = usbd_get_initial_ddesc(dev, dd);
		if (err) {
			DPRINTFN(1, "get_initial_ddesc %ju", err, 0, 0, 0);
			goto bad;
		}

		/* 4.8.2.1 */
		if (USB_IS_SS(speed)) {
			if (dd->bMaxPacketSize != 9) {
				printf("%s: invalid mps 2^%u for SS ep0,"
				    " using 512\n",
				    device_xname(sc->sc_dev),
				    dd->bMaxPacketSize);
				dd->bMaxPacketSize = 9;
			}
			USETW(dev->ud_ep0desc.wMaxPacketSize,
			    (1 << dd->bMaxPacketSize));
		} else
			USETW(dev->ud_ep0desc.wMaxPacketSize,
			    dd->bMaxPacketSize);
		DPRINTFN(4, "bMaxPacketSize %ju", dd->bMaxPacketSize, 0, 0, 0);
		err = xhci_update_ep0_mps(sc, xs,
		    UGETW(dev->ud_ep0desc.wMaxPacketSize));
		if (err) {
			DPRINTFN(1, "update mps of ep0 %ju", err, 0, 0, 0);
			goto bad;
		}
	}

	err = usbd_reload_device_desc(dev);
	if (err) {
		DPRINTFN(1, "reload desc %ju", err, 0, 0, 0);
		goto bad;
	}

	DPRINTFN(1, "adding unit addr=%jd, rev=%02jx,",
		dev->ud_addr, UGETW(dd->bcdUSB), 0, 0);
	DPRINTFN(1, " class=%jd, subclass=%jd, protocol=%jd,",
		dd->bDeviceClass, dd->bDeviceSubClass,
		dd->bDeviceProtocol, 0);
	DPRINTFN(1, " mps=%jd, len=%jd, noconf=%jd, speed=%jd",
		dd->bMaxPacketSize, dd->bLength, dd->bNumConfigurations,
		dev->ud_speed);

	usbd_get_device_strings(dev);

	usbd_add_dev_event(USB_EVENT_DEVICE_ATTACH, dev);

	if (depth == 0 && port == 0) {
		usbd_attach_roothub(parent, dev);
		DPRINTFN(1, "root hub %#jx", (uintptr_t)dev, 0, 0, 0);
		return USBD_NORMAL_COMPLETION;
	}

	err = usbd_probe_and_attach(parent, dev, port, dev->ud_addr);
 bad:
	if (err != USBD_NORMAL_COMPLETION) {
		if (depth == 0 && port == 0 && dev->ud_pipe0)
			usbd_kill_pipe(dev->ud_pipe0);
		usbd_remove_device(dev, up);
	}

	return err;
}

static usbd_status
xhci_ring_init(struct xhci_softc * const sc, struct xhci_ring **xrp,
    size_t ntrb, size_t align)
{
	size_t size = ntrb * XHCI_TRB_SIZE;
	struct xhci_ring *xr;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("xr %#jx ntrb %#jx align %#jx",
	    (uintptr_t)*xrp, ntrb, align, 0);

	xr = kmem_zalloc(sizeof(struct xhci_ring), KM_SLEEP);
	DPRINTFN(1, "ring %#jx", (uintptr_t)xr, 0, 0, 0);

	int err = usb_allocmem(sc->sc_bus.ub_dmatag, size, align,
	    USBMALLOC_COHERENT | USBMALLOC_ZERO, &xr->xr_dma);
	if (err) {
		kmem_free(xr, sizeof(struct xhci_ring));
		DPRINTFN(1, "alloc xr_dma failed %jd", err, 0, 0, 0);
		return err;
	}
	mutex_init(&xr->xr_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	xr->xr_cookies = kmem_zalloc(sizeof(*xr->xr_cookies) * ntrb, KM_SLEEP);
	xr->xr_trb = xhci_ring_trbv(xr, 0);
	xr->xr_ntrb = ntrb;
	xr->is_halted = false;
	xhci_host_dequeue(xr);
	*xrp = xr;

	return USBD_NORMAL_COMPLETION;
}

static void
xhci_ring_free(struct xhci_softc * const sc, struct xhci_ring ** const xr)
{
	if (*xr == NULL)
		return;

	usb_freemem(&(*xr)->xr_dma);
	mutex_destroy(&(*xr)->xr_lock);
	kmem_free((*xr)->xr_cookies,
	    sizeof(*(*xr)->xr_cookies) * (*xr)->xr_ntrb);
	kmem_free(*xr, sizeof(struct xhci_ring));
	*xr = NULL;
}

static void
xhci_ring_put(struct xhci_softc * const sc, struct xhci_ring * const xr,
    void *cookie, struct xhci_soft_trb * const trbs, size_t ntrbs)
{
	size_t i;
	u_int ri;
	u_int cs;
	uint64_t parameter;
	uint32_t status;
	uint32_t control;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx xr_ep %#jx xr_cs %ju",
	    (uintptr_t)xr, xr->xr_ep, xr->xr_cs, 0);

	KASSERTMSG(ntrbs < xr->xr_ntrb, "ntrbs %zu, xr->xr_ntrb %u",
	    ntrbs, xr->xr_ntrb);
	for (i = 0; i < ntrbs; i++) {
		DPRINTFN(12, "xr %#jx trbs %#jx num %ju", (uintptr_t)xr,
		    (uintptr_t)trbs, i, 0);
		DPRINTFN(12, " 0x%016jx 0x%08jx 0x%08jx",
		    trbs[i].trb_0, trbs[i].trb_2, trbs[i].trb_3, 0);
		KASSERTMSG(XHCI_TRB_3_TYPE_GET(trbs[i].trb_3) !=
		    XHCI_TRB_TYPE_LINK, "trbs[%zu].trb3 %#x", i, trbs[i].trb_3);
	}

	ri = xr->xr_ep;
	cs = xr->xr_cs;

	/*
	 * Although the xhci hardware can do scatter/gather dma from
	 * arbitrary sized buffers, there is a non-obvious restriction
	 * that a LINK trb is only allowed at the end of a burst of
	 * transfers - which might be 16kB.
	 * Arbitrary aligned LINK trb definitely fail on Ivy bridge.
	 * The simple solution is not to allow a LINK trb in the middle
	 * of anything - as here.
	 * XXX: (dsl) There are xhci controllers out there (eg some made by
	 * ASMedia) that seem to lock up if they process a LINK trb but
	 * cannot process the linked-to trb yet.
	 * The code should write the 'cycle' bit on the link trb AFTER
	 * adding the other trb.
	 */
	u_int firstep = xr->xr_ep;
	u_int firstcs = xr->xr_cs;

	for (i = 0; i < ntrbs; ) {
		u_int oldri = ri;
		u_int oldcs = cs;

		if (ri >= (xr->xr_ntrb - 1)) {
			/* Put Link TD at the end of ring */
			parameter = xhci_ring_trbp(xr, 0);
			status = 0;
			control = XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_LINK) |
			    XHCI_TRB_3_TC_BIT;
			xr->xr_cookies[ri] = NULL;
			xr->xr_ep = 0;
			xr->xr_cs ^= 1;
			ri = xr->xr_ep;
			cs = xr->xr_cs;
		} else {
			parameter = trbs[i].trb_0;
			status = trbs[i].trb_2;
			control = trbs[i].trb_3;

			xr->xr_cookies[ri] = cookie;
			ri++;
			i++;
		}
		/*
		 * If this is a first TRB, mark it invalid to prevent
		 * xHC from running it immediately.
		 */
		if (oldri == firstep) {
			if (oldcs) {
				control &= ~XHCI_TRB_3_CYCLE_BIT;
			} else {
				control |= XHCI_TRB_3_CYCLE_BIT;
			}
		} else {
			if (oldcs) {
				control |= XHCI_TRB_3_CYCLE_BIT;
			} else {
				control &= ~XHCI_TRB_3_CYCLE_BIT;
			}
		}
		xhci_trb_put(&xr->xr_trb[oldri], parameter, status, control);
		usb_syncmem(&xr->xr_dma, XHCI_TRB_SIZE * oldri,
		    XHCI_TRB_SIZE * 1, BUS_DMASYNC_PREWRITE);
	}

	/* Now invert cycle bit of first TRB */
	if (firstcs) {
		xr->xr_trb[firstep].trb_3 |= htole32(XHCI_TRB_3_CYCLE_BIT);
	} else {
		xr->xr_trb[firstep].trb_3 &= ~htole32(XHCI_TRB_3_CYCLE_BIT);
	}
	usb_syncmem(&xr->xr_dma, XHCI_TRB_SIZE * firstep,
	    XHCI_TRB_SIZE * 1, BUS_DMASYNC_PREWRITE);

	xr->xr_ep = ri;
	xr->xr_cs = cs;

	DPRINTFN(12, "%#jx xr_ep %#jx xr_cs %ju", (uintptr_t)xr, xr->xr_ep,
	    xr->xr_cs, 0);
}

static inline void
xhci_ring_put_xfer(struct xhci_softc * const sc, struct xhci_ring * const tr,
    struct xhci_xfer *xx, u_int ntrb)
{
	KASSERT(ntrb <= xx->xx_ntrb);
	xhci_ring_put(sc, tr, xx, xx->xx_trb, ntrb);
}

/*
 * Stop execution commands, purge all commands on command ring, and
 * rewind dequeue pointer.
 */
static void
xhci_abort_command(struct xhci_softc *sc)
{
	struct xhci_ring * const cr = sc->sc_cr;
	uint64_t crcr;
	int i;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("command %#jx timeout, aborting",
	    sc->sc_command_addr, 0, 0, 0);

	mutex_enter(&cr->xr_lock);

	/* 4.6.1.2 Aborting a Command */
	crcr = xhci_op_read_8(sc, XHCI_CRCR);
	xhci_op_write_8(sc, XHCI_CRCR, crcr | XHCI_CRCR_LO_CA);

	for (i = 0; i < 500; i++) {
		crcr = xhci_op_read_8(sc, XHCI_CRCR);
		if ((crcr & XHCI_CRCR_LO_CRR) == 0)
			break;
		usb_delay_ms(&sc->sc_bus, 1);
	}
	if ((crcr & XHCI_CRCR_LO_CRR) != 0) {
		DPRINTFN(1, "Command Abort timeout", 0, 0, 0, 0);
		/* reset HC here? */
	}

	/* reset command ring dequeue pointer */
	cr->xr_ep = 0;
	cr->xr_cs = 1;
	xhci_op_write_8(sc, XHCI_CRCR, xhci_ring_trbp(cr, 0) | cr->xr_cs);

	mutex_exit(&cr->xr_lock);
}

/*
 * Put a command on command ring, ring bell, set timer, and cv_timedwait.
 * Command completion is notified by cv_signal from xhci_event_cmd()
 * (called from xhci_softint), or timed-out.
 * The completion code is copied to sc->sc_result_trb in xhci_event_cmd(),
 * then do_command examines it.
 */
static usbd_status
xhci_do_command_locked(struct xhci_softc * const sc,
    struct xhci_soft_trb * const trb, int timeout)
{
	struct xhci_ring * const cr = sc->sc_cr;
	usbd_status err;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("input: 0x%016jx 0x%08jx 0x%08jx",
	    trb->trb_0, trb->trb_2, trb->trb_3, 0);

	KASSERTMSG(!cpu_intr_p() && !cpu_softintr_p(), "called from intr ctx");
	KASSERT(mutex_owned(&sc->sc_lock));

	while (sc->sc_command_addr != 0 ||
	    (sc->sc_suspender != NULL && sc->sc_suspender != curlwp))
		cv_wait(&sc->sc_cmdbusy_cv, &sc->sc_lock);

	/*
	 * If enqueue pointer points at last of ring, it's Link TRB,
	 * command TRB will be stored in 0th TRB.
	 */
	if (cr->xr_ep == cr->xr_ntrb - 1)
		sc->sc_command_addr = xhci_ring_trbp(cr, 0);
	else
		sc->sc_command_addr = xhci_ring_trbp(cr, cr->xr_ep);

	sc->sc_resultpending = true;

	mutex_enter(&cr->xr_lock);
	xhci_ring_put(sc, cr, NULL, trb, 1);
	mutex_exit(&cr->xr_lock);

	xhci_db_write_4(sc, XHCI_DOORBELL(0), 0);

	while (sc->sc_resultpending) {
		if (cv_timedwait(&sc->sc_command_cv, &sc->sc_lock,
		    MAX(1, mstohz(timeout))) == EWOULDBLOCK) {
			xhci_abort_command(sc);
			err = USBD_TIMEOUT;
			goto timedout;
		}
	}

	trb->trb_0 = sc->sc_result_trb.trb_0;
	trb->trb_2 = sc->sc_result_trb.trb_2;
	trb->trb_3 = sc->sc_result_trb.trb_3;

	DPRINTFN(12, "output: 0x%016jx 0x%08jx 0x%08jx",
	    trb->trb_0, trb->trb_2, trb->trb_3, 0);

	switch (XHCI_TRB_2_ERROR_GET(trb->trb_2)) {
	case XHCI_TRB_ERROR_SUCCESS:
		err = USBD_NORMAL_COMPLETION;
		break;
	default:
	case 192 ... 223:
		DPRINTFN(5, "error %#jx",
		    XHCI_TRB_2_ERROR_GET(trb->trb_2), 0, 0, 0);
		err = USBD_IOERROR;
		break;
	case 224 ... 255:
		err = USBD_NORMAL_COMPLETION;
		break;
	}

timedout:
	sc->sc_resultpending = false;
	sc->sc_command_addr = 0;
	cv_broadcast(&sc->sc_cmdbusy_cv);

	return err;
}

static usbd_status
xhci_do_command(struct xhci_softc * const sc, struct xhci_soft_trb * const trb,
    int timeout)
{

	mutex_enter(&sc->sc_lock);
	usbd_status ret = xhci_do_command_locked(sc, trb, timeout);
	mutex_exit(&sc->sc_lock);

	return ret;
}

static usbd_status
xhci_enable_slot(struct xhci_softc * const sc, uint8_t * const slotp)
{
	struct xhci_soft_trb trb;
	usbd_status err;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	trb.trb_0 = 0;
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_ENABLE_SLOT);

	err = xhci_do_command(sc, &trb, USBD_DEFAULT_TIMEOUT);
	if (err != USBD_NORMAL_COMPLETION) {
		return err;
	}

	*slotp = XHCI_TRB_3_SLOT_GET(trb.trb_3);

	return err;
}

/*
 * xHCI 4.6.4
 * Deallocate ring and device/input context DMA buffers, and disable_slot.
 * All endpoints in the slot should be stopped.
 * Should be called with sc_lock held.
 */
static usbd_status
xhci_disable_slot(struct xhci_softc * const sc, uint8_t slot)
{
	struct xhci_soft_trb trb;
	struct xhci_slot *xs;
	usbd_status err;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	if (sc->sc_dying)
		return USBD_IOERROR;

	trb.trb_0 = 0;
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(slot) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_DISABLE_SLOT);

	err = xhci_do_command_locked(sc, &trb, USBD_DEFAULT_TIMEOUT);

	if (!err) {
		xs = &sc->sc_slots[slot];
		if (xs->xs_idx != 0) {
			xhci_free_slot(sc, xs);
			xhci_set_dcba(sc, 0, slot);
			memset(xs, 0, sizeof(*xs));
		}
	}

	return err;
}

/*
 * Set address of device and transition slot state from ENABLED to ADDRESSED
 * if Block Setaddress Request (BSR) is false.
 * If BSR==true, transition slot state from ENABLED to DEFAULT.
 * see xHCI 1.1  4.5.3, 3.3.4
 * Should be called without sc_lock held.
 */
static usbd_status
xhci_address_device(struct xhci_softc * const sc,
    uint64_t icp, uint8_t slot_id, bool bsr)
{
	struct xhci_soft_trb trb;
	usbd_status err;

	XHCIHIST_FUNC();
	if (bsr) {
		XHCIHIST_CALLARGS("icp %#jx slot %#jx with bsr",
		    icp, slot_id, 0, 0);
	} else {
		XHCIHIST_CALLARGS("icp %#jx slot %#jx nobsr",
		    icp, slot_id, 0, 0);
	}

	trb.trb_0 = icp;
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(slot_id) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_ADDRESS_DEVICE) |
	    (bsr ? XHCI_TRB_3_BSR_BIT : 0);

	err = xhci_do_command(sc, &trb, USBD_DEFAULT_TIMEOUT);

	if (XHCI_TRB_2_ERROR_GET(trb.trb_2) == XHCI_TRB_ERROR_NO_SLOTS)
		err = USBD_NO_ADDR;

	return err;
}

static usbd_status
xhci_update_ep0_mps(struct xhci_softc * const sc,
    struct xhci_slot * const xs, u_int mps)
{
	struct xhci_soft_trb trb;
	usbd_status err;
	uint32_t * cp;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju mps %ju", xs->xs_idx, mps, 0, 0);

	cp = xhci_slot_get_icv(sc, xs, XHCI_ICI_INPUT_CONTROL);
	cp[0] = htole32(0);
	cp[1] = htole32(XHCI_INCTX_1_ADD_MASK(XHCI_DCI_EP_CONTROL));

	cp = xhci_slot_get_icv(sc, xs, xhci_dci_to_ici(XHCI_DCI_EP_CONTROL));
	cp[1] = htole32(XHCI_EPCTX_1_MAXP_SIZE_SET(mps));

	/* sync input contexts before they are read from memory */
	usb_syncmem(&xs->xs_ic_dma, 0, sc->sc_pgsz, BUS_DMASYNC_PREWRITE);
	HEXDUMP("input context", xhci_slot_get_icv(sc, xs, 0),
	    sc->sc_ctxsz * 4);

	trb.trb_0 = xhci_slot_get_icp(sc, xs, 0);
	trb.trb_2 = 0;
	trb.trb_3 = XHCI_TRB_3_SLOT_SET(xs->xs_idx) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_EVALUATE_CTX);

	err = xhci_do_command(sc, &trb, USBD_DEFAULT_TIMEOUT);
	return err;
}

static void
xhci_set_dcba(struct xhci_softc * const sc, uint64_t dcba, int si)
{
	uint64_t * const dcbaa = KERNADDR(&sc->sc_dcbaa_dma, 0);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("dcbaa %#jx dc 0x%016jx slot %jd",
	    (uintptr_t)&dcbaa[si], dcba, si, 0);

	dcbaa[si] = htole64(dcba);
	usb_syncmem(&sc->sc_dcbaa_dma, si * sizeof(uint64_t), sizeof(uint64_t),
	    BUS_DMASYNC_PREWRITE);
}

/*
 * Allocate device and input context DMA buffer, and
 * TRB DMA buffer for each endpoint.
 */
static usbd_status
xhci_init_slot(struct usbd_device *dev, uint32_t slot)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(dev->ud_bus);
	struct xhci_slot *xs;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju", slot, 0, 0, 0);

	xs = &sc->sc_slots[slot];

	/* allocate contexts */
	int err = usb_allocmem(sc->sc_bus.ub_dmatag, sc->sc_pgsz, sc->sc_pgsz,
	    USBMALLOC_COHERENT | USBMALLOC_ZERO, &xs->xs_dc_dma);
	if (err) {
		DPRINTFN(1, "failed to allocmem output device context %jd",
		    err, 0, 0, 0);
		return USBD_NOMEM;
	}

	err = usb_allocmem(sc->sc_bus.ub_dmatag, sc->sc_pgsz, sc->sc_pgsz,
	    USBMALLOC_COHERENT | USBMALLOC_ZERO, &xs->xs_ic_dma);
	if (err) {
		DPRINTFN(1, "failed to allocmem input device context %jd",
		    err, 0, 0, 0);
		goto bad1;
	}

	memset(&xs->xs_xr[0], 0, sizeof(xs->xs_xr));
	xs->xs_idx = slot;

	return USBD_NORMAL_COMPLETION;

bad1:
	usb_freemem(&xs->xs_dc_dma);
	xs->xs_idx = 0;
	return USBD_NOMEM;
}

static void
xhci_free_slot(struct xhci_softc *sc, struct xhci_slot *xs)
{
	u_int dci;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju", xs->xs_idx, 0, 0, 0);

	/* deallocate all allocated rings in the slot */
	for (dci = XHCI_DCI_SLOT; dci <= XHCI_MAX_DCI; dci++) {
		if (xs->xs_xr[dci] != NULL)
			xhci_ring_free(sc, &xs->xs_xr[dci]);
	}
	usb_freemem(&xs->xs_ic_dma);
	usb_freemem(&xs->xs_dc_dma);
	xs->xs_idx = 0;
}

/*
 * Setup slot context, set Device Context Base Address, and issue
 * Set Address Device command.
 */
static usbd_status
xhci_set_address(struct usbd_device *dev, uint32_t slot, bool bsr)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(dev->ud_bus);
	struct xhci_slot *xs;
	usbd_status err;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("slot %ju bsr %ju", slot, bsr, 0, 0);

	xs = &sc->sc_slots[slot];

	xhci_setup_ctx(dev->ud_pipe0);

	HEXDUMP("input context", xhci_slot_get_icv(sc, xs, 0),
	    sc->sc_ctxsz * 3);

	xhci_set_dcba(sc, DMAADDR(&xs->xs_dc_dma, 0), slot);

	err = xhci_address_device(sc, xhci_slot_get_icp(sc, xs, 0), slot, bsr);

	usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);
	HEXDUMP("output context", xhci_slot_get_dcv(sc, xs, 0),
	    sc->sc_ctxsz * 2);

	return err;
}

/*
 * 4.8.2, 6.2.3.2
 * construct slot/endpoint context parameters and do syncmem
 */
static void
xhci_setup_ctx(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct usbd_device *dev = pipe->up_dev;
	struct xhci_slot * const xs = dev->ud_hcpriv;
	usb_endpoint_descriptor_t * const ed = pipe->up_endpoint->ue_edesc;
	const u_int dci = xhci_ep_get_dci(ed);
	const uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	uint32_t *cp;
	uint16_t mps = UGETW(ed->wMaxPacketSize);
	uint8_t speed = dev->ud_speed;
	uint8_t ival = ed->bInterval;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("pipe %#jx: slot %ju dci %ju speed %ju",
	    (uintptr_t)pipe, xs->xs_idx, dci, speed);

	/* set up initial input control context */
	cp = xhci_slot_get_icv(sc, xs, XHCI_ICI_INPUT_CONTROL);
	cp[0] = htole32(0);
	cp[1] = htole32(XHCI_INCTX_1_ADD_MASK(dci));
	cp[1] |= htole32(XHCI_INCTX_1_ADD_MASK(XHCI_DCI_SLOT));
	cp[7] = htole32(0);

	/* set up input slot context */
	cp = xhci_slot_get_icv(sc, xs, xhci_dci_to_ici(XHCI_DCI_SLOT));
	cp[0] =
	    XHCI_SCTX_0_CTX_NUM_SET(dci) |
	    XHCI_SCTX_0_SPEED_SET(xhci_speed2xspeed(speed));
	cp[1] = 0;
	cp[2] = XHCI_SCTX_2_IRQ_TARGET_SET(0);
	cp[3] = 0;
	xhci_setup_route(pipe, cp);
	xhci_setup_tthub(pipe, cp);

	cp[0] = htole32(cp[0]);
	cp[1] = htole32(cp[1]);
	cp[2] = htole32(cp[2]);
	cp[3] = htole32(cp[3]);

	/* set up input endpoint context */
	cp = xhci_slot_get_icv(sc, xs, xhci_dci_to_ici(dci));
	cp[0] =
	    XHCI_EPCTX_0_EPSTATE_SET(0) |
	    XHCI_EPCTX_0_MULT_SET(0) |
	    XHCI_EPCTX_0_MAXP_STREAMS_SET(0) |
	    XHCI_EPCTX_0_LSA_SET(0) |
	    XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_SET(0);
	cp[1] =
	    XHCI_EPCTX_1_EPTYPE_SET(xhci_ep_get_type(ed)) |
	    XHCI_EPCTX_1_HID_SET(0) |
	    XHCI_EPCTX_1_MAXB_SET(0);

	if (xfertype != UE_ISOCHRONOUS)
		cp[1] |= XHCI_EPCTX_1_CERR_SET(3);

	if (xfertype == UE_CONTROL)
		cp[4] = XHCI_EPCTX_4_AVG_TRB_LEN_SET(8); /* 6.2.3 */
	else if (USB_IS_SS(speed))
		cp[4] = XHCI_EPCTX_4_AVG_TRB_LEN_SET(mps);
	else
		cp[4] = XHCI_EPCTX_4_AVG_TRB_LEN_SET(UE_GET_SIZE(mps));

	xhci_setup_maxburst(pipe, cp);

	switch (xfertype) {
	case UE_CONTROL:
		break;
	case UE_BULK:
		/* XXX Set MaxPStreams, HID, and LSA if streams enabled */
		break;
	case UE_INTERRUPT:
		if (pipe->up_interval != USBD_DEFAULT_INTERVAL)
			ival = pipe->up_interval;

		ival = xhci_bival2ival(ival, speed);
		cp[0] |= XHCI_EPCTX_0_IVAL_SET(ival);
		break;
	case UE_ISOCHRONOUS:
		if (pipe->up_interval != USBD_DEFAULT_INTERVAL)
			ival = pipe->up_interval;

		/* xHCI 6.2.3.6 Table 65, USB 2.0 9.6.6 */
		if (speed == USB_SPEED_FULL)
			ival += 3; /* 1ms -> 125us */
		ival--;
		cp[0] |= XHCI_EPCTX_0_IVAL_SET(ival);
		break;
	default:
		break;
	}
	DPRINTFN(4, "setting ival %ju MaxBurst %#jx",
	    XHCI_EPCTX_0_IVAL_GET(cp[0]), XHCI_EPCTX_1_MAXB_GET(cp[1]), 0, 0);

	/* rewind TR dequeue pointer in xHC */
	/* can't use xhci_ep_get_dci() yet? */
	*(uint64_t *)(&cp[2]) = htole64(
	    xhci_ring_trbp(xs->xs_xr[dci], 0) |
	    XHCI_EPCTX_2_DCS_SET(1));

	cp[0] = htole32(cp[0]);
	cp[1] = htole32(cp[1]);
	cp[4] = htole32(cp[4]);

	/* rewind TR dequeue pointer in driver */
	struct xhci_ring *xr = xs->xs_xr[dci];
	mutex_enter(&xr->xr_lock);
	xhci_host_dequeue(xr);
	mutex_exit(&xr->xr_lock);

	/* sync input contexts before they are read from memory */
	usb_syncmem(&xs->xs_ic_dma, 0, sc->sc_pgsz, BUS_DMASYNC_PREWRITE);
}

/*
 * Setup route string and roothub port of given device for slot context
 */
static void
xhci_setup_route(struct usbd_pipe *pipe, uint32_t *cp)
{
	struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);
	struct usbd_device *dev = pipe->up_dev;
	struct usbd_port *up = dev->ud_powersrc;
	struct usbd_device *hub;
	struct usbd_device *adev;
	uint8_t rhport = 0;
	uint32_t route = 0;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Locate root hub port and Determine route string */
	/* 4.3.3 route string does not include roothub port */
	for (hub = dev; hub != NULL; hub = hub->ud_myhub) {
		uint32_t dep;

		DPRINTFN(4, "hub %#jx depth %jd upport %#jx upportno %jd",
		    (uintptr_t)hub, hub->ud_depth, (uintptr_t)hub->ud_powersrc,
		    hub->ud_powersrc ? (uintptr_t)hub->ud_powersrc->up_portno :
			 -1);

		if (hub->ud_powersrc == NULL)
			break;
		dep = hub->ud_depth;
		if (dep == 0)
			break;
		rhport = hub->ud_powersrc->up_portno;
		if (dep > USB_HUB_MAX_DEPTH)
			continue;

		route |=
		    (rhport > UHD_SS_NPORTS_MAX ? UHD_SS_NPORTS_MAX : rhport)
		    << ((dep - 1) * 4);
	}
	route = route >> 4;
	size_t bn = hub == sc->sc_bus.ub_roothub ? 0 : 1;

	/* Locate port on upstream high speed hub */
	for (adev = dev, hub = up->up_parent;
	     hub != NULL && hub->ud_speed != USB_SPEED_HIGH;
	     adev = hub, hub = hub->ud_myhub)
		;
	if (hub) {
		int p;
		for (p = 1; p <= hub->ud_hub->uh_hubdesc.bNbrPorts; p++) {
			if (hub->ud_hub->uh_ports[p - 1].up_dev == adev) {
				dev->ud_myhsport = &hub->ud_hub->uh_ports[p - 1];
				goto found;
			}
		}
		panic("%s: cannot find HS port", __func__);
	found:
		DPRINTFN(4, "high speed port %jd", p, 0, 0, 0);
	} else {
		dev->ud_myhsport = NULL;
	}

	const size_t ctlrport = xhci_rhport2ctlrport(sc, bn, rhport);

	DPRINTFN(4, "rhport %ju ctlrport %ju Route %05jx hub %#jx", rhport,
	    ctlrport, route, (uintptr_t)hub);

	cp[0] |= XHCI_SCTX_0_ROUTE_SET(route);
	cp[1] |= XHCI_SCTX_1_RH_PORT_SET(ctlrport);
}

/*
 * Setup whether device is hub, whether device uses MTT, and
 * TT informations if it uses MTT.
 */
static void
xhci_setup_tthub(struct usbd_pipe *pipe, uint32_t *cp)
{
	struct usbd_device *dev = pipe->up_dev;
	struct usbd_port *myhsport = dev->ud_myhsport;
	usb_device_descriptor_t * const dd = &dev->ud_ddesc;
	uint32_t speed = dev->ud_speed;
	uint8_t rhaddr = dev->ud_bus->ub_rhaddr;
	uint8_t tthubslot, ttportnum;
	bool ishub;
	bool usemtt;

	XHCIHIST_FUNC();

	/*
	 * 6.2.2, Table 57-60, 6.2.2.1, 6.2.2.2
	 * tthubslot:
	 *   This is the slot ID of parent HS hub
	 *   if LS/FS device is connected && connected through HS hub.
	 *   This is 0 if device is not LS/FS device ||
	 *   parent hub is not HS hub ||
	 *   attached to root hub.
	 * ttportnum:
	 *   This is the downstream facing port of parent HS hub
	 *   if LS/FS device is connected.
	 *   This is 0 if device is not LS/FS device ||
	 *   parent hub is not HS hub ||
	 *   attached to root hub.
	 */
	if (myhsport &&
	    myhsport->up_parent->ud_addr != rhaddr &&
	    (speed == USB_SPEED_LOW || speed == USB_SPEED_FULL)) {
		ttportnum = myhsport->up_portno;
		tthubslot = myhsport->up_parent->ud_addr;
	} else {
		ttportnum = 0;
		tthubslot = 0;
	}
	XHCIHIST_CALLARGS("myhsport %#jx ttportnum=%jd tthubslot=%jd",
	    (uintptr_t)myhsport, ttportnum, tthubslot, 0);

	/* ishub is valid after reading UDESC_DEVICE */
	ishub = (dd->bDeviceClass == UDCLASS_HUB);

	/* dev->ud_hub is valid after reading UDESC_HUB */
	if (ishub && dev->ud_hub) {
		usb_hub_descriptor_t *hd = &dev->ud_hub->uh_hubdesc;
		uint8_t ttt =
		    __SHIFTOUT(UGETW(hd->wHubCharacteristics), UHD_TT_THINK);

		cp[1] |= XHCI_SCTX_1_NUM_PORTS_SET(hd->bNbrPorts);
		cp[2] |= XHCI_SCTX_2_TT_THINK_TIME_SET(ttt);
		DPRINTFN(4, "nports=%jd ttt=%jd", hd->bNbrPorts, ttt, 0, 0);
	}

#define IS_MTTHUB(dd) \
     ((dd)->bDeviceProtocol == UDPROTO_HSHUBMTT)

	/*
	 * MTT flag is set if
	 * 1. this is HS hub && MTTs are supported and enabled;  or
	 * 2. this is LS or FS device && there is a parent HS hub where MTTs
	 *    are supported and enabled.
	 *
	 * XXX enabled is not tested yet
	 */
	if (ishub && speed == USB_SPEED_HIGH && IS_MTTHUB(dd))
		usemtt = true;
	else if ((speed == USB_SPEED_LOW || speed == USB_SPEED_FULL) &&
	    myhsport &&
	    myhsport->up_parent->ud_addr != rhaddr &&
	    IS_MTTHUB(&myhsport->up_parent->ud_ddesc))
		usemtt = true;
	else
		usemtt = false;
	DPRINTFN(4, "class %ju proto %ju ishub %jd usemtt %jd",
	    dd->bDeviceClass, dd->bDeviceProtocol, ishub, usemtt);

#undef IS_MTTHUB

	cp[0] |=
	    XHCI_SCTX_0_HUB_SET(ishub ? 1 : 0) |
	    XHCI_SCTX_0_MTT_SET(usemtt ? 1 : 0);
	cp[2] |=
	    XHCI_SCTX_2_TT_HUB_SID_SET(tthubslot) |
	    XHCI_SCTX_2_TT_PORT_NUM_SET(ttportnum);
}

/* set up params for periodic endpoint */
static void
xhci_setup_maxburst(struct usbd_pipe *pipe, uint32_t *cp)
{
	struct xhci_pipe * const xpipe = (struct xhci_pipe *)pipe;
	struct usbd_device *dev = pipe->up_dev;
	usb_endpoint_descriptor_t * const ed = pipe->up_endpoint->ue_edesc;
	const uint8_t xfertype = UE_GET_XFERTYPE(ed->bmAttributes);
	usbd_desc_iter_t iter;
	const usb_cdc_descriptor_t *cdcd;
	uint32_t maxb = 0;
	uint16_t mps = UGETW(ed->wMaxPacketSize);
	uint8_t speed = dev->ud_speed;
	uint8_t mult = 0;
	uint8_t ep;

	/* config desc is NULL when opening ep0 */
	if (dev == NULL || dev->ud_cdesc == NULL)
		goto no_cdcd;
	cdcd = (const usb_cdc_descriptor_t *)usb_find_desc(dev,
	    UDESC_INTERFACE, USBD_CDCSUBTYPE_ANY);
	if (cdcd == NULL)
		goto no_cdcd;
	usb_desc_iter_init(dev, &iter);
	iter.cur = (const void *)cdcd;

	/* find endpoint_ss_comp desc for ep of this pipe */
	for (ep = 0;;) {
		cdcd = (const usb_cdc_descriptor_t *)usb_desc_iter_next(&iter);
		if (cdcd == NULL)
			break;
		if (ep == 0 && cdcd->bDescriptorType == UDESC_ENDPOINT) {
			ep = ((const usb_endpoint_descriptor_t *)cdcd)->
			    bEndpointAddress;
			if (UE_GET_ADDR(ep) ==
			    UE_GET_ADDR(ed->bEndpointAddress)) {
				cdcd = (const usb_cdc_descriptor_t *)
				    usb_desc_iter_next(&iter);
				break;
			}
			ep = 0;
		}
	}
	if (cdcd != NULL && cdcd->bDescriptorType == UDESC_ENDPOINT_SS_COMP) {
		const usb_endpoint_ss_comp_descriptor_t * esscd =
		    (const usb_endpoint_ss_comp_descriptor_t *)cdcd;
		maxb = esscd->bMaxBurst;
		mult = UE_GET_SS_ISO_MULT(esscd->bmAttributes);
	}

 no_cdcd:
	/* 6.2.3.4,  4.8.2.4 */
	if (USB_IS_SS(speed)) {
		/* USB 3.1  9.6.6 */
		cp[1] |= XHCI_EPCTX_1_MAXP_SIZE_SET(mps);
		/* USB 3.1  9.6.7 */
		cp[1] |= XHCI_EPCTX_1_MAXB_SET(maxb);
#ifdef notyet
		if (xfertype == UE_ISOCHRONOUS) {
		}
		if (XHCI_HCC2_LEC(sc->sc_hcc2) != 0) {
			/* use ESIT */
			cp[4] |= XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_SET(x);
			cp[0] |= XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_SET(x);

			/* XXX if LEC = 1, set ESIT instead */
			cp[0] |= XHCI_EPCTX_0_MULT_SET(0);
		} else {
			/* use ival */
		}
#endif
	} else {
		/* USB 2.0  9.6.6 */
		cp[1] |= XHCI_EPCTX_1_MAXP_SIZE_SET(UE_GET_SIZE(mps));

		/* 6.2.3.4 */
		if (speed == USB_SPEED_HIGH &&
		   (xfertype == UE_ISOCHRONOUS || xfertype == UE_INTERRUPT)) {
			maxb = UE_GET_TRANS(mps);
		} else {
			/* LS/FS or HS CTRL or HS BULK */
			maxb = 0;
		}
		cp[1] |= XHCI_EPCTX_1_MAXB_SET(maxb);
	}
	xpipe->xp_maxb = maxb + 1;
	xpipe->xp_mult = mult + 1;
}

/*
 * Convert endpoint bInterval value to endpoint context interval value
 * for Interrupt pipe.
 * xHCI 6.2.3.6 Table 65, USB 2.0 9.6.6
 */
static uint32_t
xhci_bival2ival(uint32_t ival, uint32_t speed)
{
	if (speed == USB_SPEED_LOW || speed == USB_SPEED_FULL) {
		int i;

		/*
		 * round ival down to "the nearest base 2 multiple of
		 * bInterval * 8".
		 * bInterval is at most 255 as its type is uByte.
		 * 255(ms) = 2040(x 125us) < 2^11, so start with 10.
		 */
		for (i = 10; i > 0; i--) {
			if ((ival * 8) >= (1 << i))
				break;
		}
		ival = i;
	} else {
		/* Interval = bInterval-1 for SS/HS */
		ival--;
	}

	return ival;
}

/* ----- */

static void
xhci_noop(struct usbd_pipe *pipe)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();
}

/*
 * Process root hub request.
 */
static int
xhci_roothub_ctrl_locked(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct xhci_softc * const sc = XHCI_BUS2SC(bus);
	usb_port_status_t ps;
	int l, totlen = 0;
	uint16_t len, value, index;
	int port, i;
	uint32_t v;

	XHCIHIST_FUNC();

	KASSERT(mutex_owned(&sc->sc_rhlock));

	if (sc->sc_dying)
		return -1;

	size_t bn = bus == &sc->sc_bus ? 0 : 1;

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	XHCIHIST_CALLARGS("rhreq: %04jx %04jx %04jx %04jx",
	    req->bmRequestType | (req->bRequest << 8), value, index, len);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(8, "getdesc: wValue=0x%04jx", value, 0, 0, 0);
		if (len == 0)
			break;
		switch (value) {
#define sd ((usb_string_descriptor_t *)buf)
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "xHCI root hub");
			break;
#undef sd
		default:
			/* default from usbroothub */
			return buflen;
		}
		break;

	/* Hub requests */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	/* Clear Port Feature request */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER): {
		const size_t cp = xhci_rhport2ctlrport(sc, bn, index);

		DPRINTFN(4, "UR_CLEAR_PORT_FEAT bp=%jd feat=%jd bus=%jd cp=%jd",
		    index, value, bn, cp);
		if (index < 1 || index > sc->sc_rhportcount[bn]) {
			return -1;
		}
		port = XHCI_PORTSC(cp);
		v = xhci_op_read_4(sc, port);
		DPRINTFN(4, "portsc=0x%08jx", v, 0, 0, 0);
		v &= ~XHCI_PS_CLEAR;
		switch (value) {
		case UHF_PORT_ENABLE:
			xhci_op_write_4(sc, port, v & ~XHCI_PS_PED);
			break;
		case UHF_PORT_SUSPEND:
			return -1;
		case UHF_PORT_POWER:
			break;
		case UHF_PORT_TEST:
		case UHF_PORT_INDICATOR:
			return -1;
		case UHF_C_PORT_CONNECTION:
			xhci_op_write_4(sc, port, v | XHCI_PS_CSC);
			break;
		case UHF_C_PORT_ENABLE:
		case UHF_C_PORT_SUSPEND:
		case UHF_C_PORT_OVER_CURRENT:
			return -1;
		case UHF_C_BH_PORT_RESET:
			xhci_op_write_4(sc, port, v | XHCI_PS_WRC);
			break;
		case UHF_C_PORT_RESET:
			xhci_op_write_4(sc, port, v | XHCI_PS_PRC);
			break;
		case UHF_C_PORT_LINK_STATE:
			xhci_op_write_4(sc, port, v | XHCI_PS_PLC);
			break;
		case UHF_C_PORT_CONFIG_ERROR:
			xhci_op_write_4(sc, port, v | XHCI_PS_CEC);
			break;
		default:
			return -1;
		}
		break;
	}
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if (len == 0)
			break;
		if ((value & 0xff) != 0) {
			return -1;
		}
		usb_hub_descriptor_t hubd;

		totlen = uimin(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);
		hubd.bNbrPorts = sc->sc_rhportcount[bn];
		USETW(hubd.wHubCharacteristics, UHD_PWR_NO_SWITCH);
		hubd.bPwrOn2PwrGood = 200;
		for (i = 0, l = sc->sc_rhportcount[bn]; l > 0; i++, l -= 8) {
			/* XXX can't find out? */
			hubd.DeviceRemovable[i++] = 0;
		}
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + i;
		totlen = uimin(totlen, hubd.bDescLength);
		memcpy(buf, &hubd, totlen);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			return -1;
		}
		memset(buf, 0, len); /* ? XXX */
		totlen = len;
		break;
	/* Get Port Status request */
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER): {
		const size_t cp = xhci_rhport2ctlrport(sc, bn, index);

		DPRINTFN(8, "get port status bn=%jd i=%jd cp=%ju",
		    bn, index, cp, 0);
		if (index < 1 || index > sc->sc_rhportcount[bn]) {
			DPRINTFN(5, "bad get port status: index=%jd bn=%jd "
				    "portcount=%jd",
			    index, bn, sc->sc_rhportcount[bn], 0);
			return -1;
		}
		if (len != 4) {
			DPRINTFN(5, "bad get port status: len %jd != 4",
			    len, 0, 0, 0);
			return -1;
		}
		v = xhci_op_read_4(sc, XHCI_PORTSC(cp));
		DPRINTFN(4, "getrhportsc %jd 0x%08jx", cp, v, 0, 0);
		i = xhci_xspeed2psspeed(XHCI_PS_SPEED_GET(v));
		if (v & XHCI_PS_CCS)	i |= UPS_CURRENT_CONNECT_STATUS;
		if (v & XHCI_PS_PED)	i |= UPS_PORT_ENABLED;
		if (v & XHCI_PS_OCA)	i |= UPS_OVERCURRENT_INDICATOR;
		//if (v & XHCI_PS_SUSP)	i |= UPS_SUSPEND;
		if (v & XHCI_PS_PR)	i |= UPS_RESET;
		if (v & XHCI_PS_PP) {
			if (i & UPS_OTHER_SPEED)
					i |= UPS_PORT_POWER_SS;
			else
					i |= UPS_PORT_POWER;
		}
		if (i & UPS_OTHER_SPEED)
			i |= UPS_PORT_LS_SET(XHCI_PS_PLS_GET(v));
		if (sc->sc_vendor_port_status)
			i = sc->sc_vendor_port_status(sc, v, i);
		USETW(ps.wPortStatus, i);
		i = 0;
		if (v & XHCI_PS_CSC)    i |= UPS_C_CONNECT_STATUS;
		if (v & XHCI_PS_PEC)    i |= UPS_C_PORT_ENABLED;
		if (v & XHCI_PS_OCC)    i |= UPS_C_OVERCURRENT_INDICATOR;
		if (v & XHCI_PS_PRC)	i |= UPS_C_PORT_RESET;
		if (v & XHCI_PS_WRC)	i |= UPS_C_BH_PORT_RESET;
		if (v & XHCI_PS_PLC)	i |= UPS_C_PORT_LINK_STATE;
		if (v & XHCI_PS_CEC)	i |= UPS_C_PORT_CONFIG_ERROR;
		USETW(ps.wPortChange, i);
		totlen = uimin(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		DPRINTFN(5, "get port status: wPortStatus %#jx wPortChange %#jx"
			    " totlen %jd",
		    UGETW(ps.wPortStatus), UGETW(ps.wPortChange), totlen, 0);
		break;
	}
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		return -1;
	case C(UR_SET_HUB_DEPTH, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	/* Set Port Feature request */
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER): {
		int optval = (index >> 8) & 0xff;
		index &= 0xff;
		if (index < 1 || index > sc->sc_rhportcount[bn]) {
			return -1;
		}

		const size_t cp = xhci_rhport2ctlrport(sc, bn, index);

		port = XHCI_PORTSC(cp);
		v = xhci_op_read_4(sc, port);
		DPRINTFN(4, "index %jd cp %jd portsc=0x%08jx", index, cp, v, 0);
		v &= ~XHCI_PS_CLEAR;
		switch (value) {
		case UHF_PORT_ENABLE:
			xhci_op_write_4(sc, port, v | XHCI_PS_PED);
			break;
		case UHF_PORT_SUSPEND:
			/* XXX suspend */
			break;
		case UHF_PORT_RESET:
			xhci_op_write_4(sc, port, v | XHCI_PS_PR);
			/* Wait for reset to complete. */
			for (i = 0; i < USB_PORT_ROOT_RESET_DELAY / 10; i++) {
				if (sc->sc_dying) {
					return -1;
				}
				v = xhci_op_read_4(sc, port);
				if ((v & XHCI_PS_PR) == 0) {
					break;
				}
				usb_delay_ms(&sc->sc_bus, 10);
			}
			break;
		case UHF_PORT_POWER:
			/* XXX power control */
			break;
		/* XXX more */
		case UHF_C_PORT_RESET:
			xhci_op_write_4(sc, port, v | XHCI_PS_PRC);
			break;
		case UHF_PORT_U1_TIMEOUT:
			if (XHCI_PS_SPEED_GET(v) < XHCI_PS_SPEED_SS) {
				return -1;
			}
			port = XHCI_PORTPMSC(cp);
			v = xhci_op_read_4(sc, port);
			DPRINTFN(4, "index %jd cp %jd portpmsc=0x%08jx",
			    index, cp, v, 0);
			v &= ~XHCI_PM3_U1TO_SET(0xff);
			v |= XHCI_PM3_U1TO_SET(optval);
			xhci_op_write_4(sc, port, v);
			break;
		case UHF_PORT_U2_TIMEOUT:
			if (XHCI_PS_SPEED_GET(v) < XHCI_PS_SPEED_SS) {
				return -1;
			}
			port = XHCI_PORTPMSC(cp);
			v = xhci_op_read_4(sc, port);
			DPRINTFN(4, "index %jd cp %jd portpmsc=0x%08jx",
			    index, cp, v, 0);
			v &= ~XHCI_PM3_U2TO_SET(0xff);
			v |= XHCI_PM3_U2TO_SET(optval);
			xhci_op_write_4(sc, port, v);
			break;
		default:
			return -1;
		}
	}
		break;
	case C(UR_CLEAR_TT_BUFFER, UT_WRITE_CLASS_OTHER):
	case C(UR_RESET_TT, UT_WRITE_CLASS_OTHER):
	case C(UR_GET_TT_STATE, UT_READ_CLASS_OTHER):
	case C(UR_STOP_TT, UT_WRITE_CLASS_OTHER):
		break;
	default:
		/* default from usbroothub */
		return buflen;
	}

	return totlen;
}

static int
xhci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct xhci_softc *sc = XHCI_BUS2SC(bus);
	int actlen;

	mutex_enter(&sc->sc_rhlock);
	actlen = xhci_roothub_ctrl_locked(bus, req, buf, buflen);
	mutex_exit(&sc->sc_rhlock);

	return actlen;
}

/* root hub interrupt */

static usbd_status
xhci_root_intr_transfer(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Pipe isn't running, start first */
	return xhci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

/* Wait for roothub port status/change */
static usbd_status
xhci_root_intr_start(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	const size_t bn = XHCI_XFER2BUS(xfer) == &sc->sc_bus ? 0 : 1;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(xhci_polling_p(sc) || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(sc->sc_intrxfer[bn] == NULL);
	sc->sc_intrxfer[bn] = xfer;
	xfer->ux_status = USBD_IN_PROGRESS;

	return USBD_IN_PROGRESS;
}

static void
xhci_root_intr_abort(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	const size_t bn = XHCI_XFER2BUS(xfer) == &sc->sc_bus ? 0 : 1;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(xfer->ux_pipe->up_intrxfer == xfer);

	/* If xfer has already completed, nothing to do here.  */
	if (sc->sc_intrxfer[bn] == NULL)
		return;

	/*
	 * Otherwise, sc->sc_intrxfer[bn] had better be this transfer.
	 * Cancel it.
	 */
	KASSERT(sc->sc_intrxfer[bn] == xfer);
	xfer->ux_status = USBD_CANCELLED;
	usb_transfer_complete(xfer);
}

static void
xhci_root_intr_close(struct usbd_pipe *pipe)
{
	struct xhci_softc * const sc __diagused = XHCI_PIPE2SC(pipe);
	const struct usbd_xfer *xfer __diagused = pipe->up_intrxfer;
	const size_t bn __diagused = XHCI_XFER2BUS(xfer) == &sc->sc_bus ? 0 : 1;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	/*
	 * Caller must guarantee the xfer has completed first, by
	 * closing the pipe only after normal completion or an abort.
	 */
	KASSERT(sc->sc_intrxfer[bn] == NULL);
}

static void
xhci_root_intr_done(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	const size_t bn = XHCI_XFER2BUS(xfer) == &sc->sc_bus ? 0 : 1;

	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	KASSERT(mutex_owned(&sc->sc_lock));

	/* Claim the xfer so it doesn't get completed again.  */
	KASSERT(sc->sc_intrxfer[bn] == xfer);
	KASSERT(xfer->ux_status != USBD_IN_PROGRESS);
	sc->sc_intrxfer[bn] = NULL;
}

/* -------------- */
/* device control */

static usbd_status
xhci_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Pipe isn't running, start first */
	return xhci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
xhci_device_ctrl_start(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];
	struct xhci_xfer * const xx = XHCI_XFER2XXFER(xfer);
	usb_device_request_t * const req = &xfer->ux_request;
	const bool isread = usbd_xfer_isread(xfer);
	const uint32_t len = UGETW(req->wLength);
	usb_dma_t * const dma = &xfer->ux_dmabuf;
	uint64_t parameter;
	uint32_t status;
	uint32_t control;
	u_int i;
	const bool polling = xhci_polling_p(sc);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("req: %04jx %04jx %04jx %04jx",
	    req->bmRequestType | (req->bRequest << 8), UGETW(req->wValue),
	    UGETW(req->wIndex), UGETW(req->wLength));

	KASSERT(polling || mutex_owned(&sc->sc_lock));

	/* we rely on the bottom bits for extra info */
	KASSERTMSG(((uintptr_t)xfer & 0x3) == 0x0, "xfer %zx",
	    (uintptr_t) xfer);

	KASSERT((xfer->ux_rqflags & URQ_REQUEST) != 0);

	if (tr->is_halted)
		goto out;

	i = 0;

	/* setup phase */
	parameter = le64dec(req); /* to keep USB endian after xhci_trb_put() */
	status = XHCI_TRB_2_IRQ_SET(0) | XHCI_TRB_2_BYTES_SET(sizeof(*req));
	control = ((len == 0) ? XHCI_TRB_3_TRT_NONE :
	     (isread ? XHCI_TRB_3_TRT_IN : XHCI_TRB_3_TRT_OUT)) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_SETUP_STAGE) |
	    XHCI_TRB_3_IDT_BIT;
	xhci_xfer_put_trb(xx, i++, parameter, status, control);

	if (len != 0) {
		/* data phase */
		parameter = DMAADDR(dma, 0);
		KASSERTMSG(len <= 0x10000, "len %d", len);
		status = XHCI_TRB_2_IRQ_SET(0) |
		    XHCI_TRB_2_TDSZ_SET(0) |
		    XHCI_TRB_2_BYTES_SET(len);
		control = (isread ? XHCI_TRB_3_DIR_IN : 0) |
		    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_DATA_STAGE) |
		    (isread ? XHCI_TRB_3_ISP_BIT : 0) |
		    XHCI_TRB_3_IOC_BIT;
		xhci_xfer_put_trb(xx, i++, parameter, status, control);

		usb_syncmem(dma, 0, len,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	}

	parameter = 0;
	status = XHCI_TRB_2_IRQ_SET(0);
	/* the status stage has inverted direction */
	control = ((isread && (len > 0)) ? 0 : XHCI_TRB_3_DIR_IN) |
	    XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_STATUS_STAGE) |
	    XHCI_TRB_3_IOC_BIT;
	xhci_xfer_put_trb(xx, i++, parameter, status, control);

	if (!polling)
		mutex_enter(&tr->xr_lock);
	xhci_ring_put_xfer(sc, tr, xx, i);
	if (!polling)
		mutex_exit(&tr->xr_lock);

	xhci_db_write_4(sc, XHCI_DOORBELL(xs->xs_idx), dci);

out:	if (xfer->ux_status == USBD_NOT_STARTED) {
		usbd_xfer_schedule_timeout(xfer);
		xfer->ux_status = USBD_IN_PROGRESS;
	} else {
		/*
		 * We must be coming from xhci_pipe_restart -- timeout
		 * already set up, nothing to do.
		 */
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	return USBD_IN_PROGRESS;
}

static void
xhci_device_ctrl_done(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();
	usb_device_request_t *req = &xfer->ux_request;
	int len = UGETW(req->wLength);
	int rd = req->bmRequestType & UT_READ;

	if (len)
		usb_syncmem(&xfer->ux_dmabuf, 0, len,
		    rd ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

static void
xhci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	usbd_xfer_abort(xfer);
}

static void
xhci_device_ctrl_close(struct usbd_pipe *pipe)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	xhci_close_pipe(pipe);
}

/* ------------------ */
/* device isochronous */

static usbd_status
xhci_device_isoc_transfer(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	return xhci_device_isoc_enter(xfer);
}

static usbd_status
xhci_device_isoc_enter(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];
	struct xhci_xfer * const xx = XHCI_XFER2XXFER(xfer);
	struct xhci_pipe * const xpipe = (struct xhci_pipe *)xfer->ux_pipe;
	uint32_t len = xfer->ux_length;
	usb_dma_t * const dma = &xfer->ux_dmabuf;
	uint64_t parameter;
	uint32_t status;
	uint32_t control;
	uint32_t mfindex;
	uint32_t offs;
	int i, ival;
	const bool polling = xhci_polling_p(sc);
	const uint16_t MPS = UGETW(xfer->ux_pipe->up_endpoint->ue_edesc->wMaxPacketSize);
	const uint16_t mps = UE_GET_SIZE(MPS);
	const uint8_t maxb = xpipe->xp_maxb;
	u_int tdpc, tbc, tlbpc;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	KASSERT(polling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT(xfer->ux_nframes != 0 && xfer->ux_frlengths);
	KASSERT((xfer->ux_rqflags & URQ_REQUEST) == 0);

	const bool isread = usbd_xfer_isread(xfer);
	if (xfer->ux_length)
		usb_syncmem(dma, 0, xfer->ux_length,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	ival = xfer->ux_pipe->up_endpoint->ue_edesc->bInterval;
	if (ival >= 1 && ival <= 16)
		ival = 1 << (ival - 1);
	else
		ival = 1; /* fake something up */

	if (xpipe->xp_isoc_next == -1) {
		mfindex = xhci_rt_read_4(sc, XHCI_MFINDEX);
		DPRINTF("mfindex %jx", (uintmax_t)mfindex, 0, 0, 0);
		mfindex = XHCI_MFINDEX_GET(mfindex + 1);
		mfindex /= USB_UFRAMES_PER_FRAME;
		mfindex += 7; /* 7 frames is max possible IST */
		xpipe->xp_isoc_next = roundup2(mfindex, ival);
	}

	offs = 0;
	for (i = 0; i < xfer->ux_nframes; i++) {
		len = xfer->ux_frlengths[i];

		tdpc = howmany(len, mps);
		tbc = howmany(tdpc, maxb) - 1;
		tlbpc = tdpc % maxb;
		tlbpc = tlbpc ? tlbpc - 1 : maxb - 1;

		KASSERTMSG(len <= 0x10000, "len %d", len);
		parameter = DMAADDR(dma, offs);
		status = XHCI_TRB_2_IRQ_SET(0) |
		    XHCI_TRB_2_TDSZ_SET(0) |
		    XHCI_TRB_2_BYTES_SET(len);
		control = XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_ISOCH) |
		    (isread ? XHCI_TRB_3_ISP_BIT : 0) |
		    XHCI_TRB_3_TBC_SET(tbc) |
		    XHCI_TRB_3_TLBPC_SET(tlbpc) |
		    XHCI_TRB_3_IOC_BIT;
		if (XHCI_HCC_CFC(sc->sc_hcc)) {
			control |= XHCI_TRB_3_FRID_SET(xpipe->xp_isoc_next);
#if 0
		} else if (xpipe->xp_isoc_next == -1) {
			control |= XHCI_TRB_3_FRID_SET(xpipe->xp_isoc_next);
#endif
		} else {
			control |= XHCI_TRB_3_ISO_SIA_BIT;
		}
#if 0
		if (i != xfer->ux_nframes - 1)
			control |= XHCI_TRB_3_BEI_BIT;
#endif
		xhci_xfer_put_trb(xx, i, parameter, status, control);

		xpipe->xp_isoc_next += ival;
		offs += len;
	}

	xx->xx_isoc_done = 0;

	if (!polling)
		mutex_enter(&tr->xr_lock);
	xhci_ring_put_xfer(sc, tr, xx, i);
	if (!polling)
		mutex_exit(&tr->xr_lock);

	xfer->ux_status = USBD_IN_PROGRESS;
	xhci_db_write_4(sc, XHCI_DOORBELL(xs->xs_idx), dci);
	usbd_xfer_schedule_timeout(xfer);

	return USBD_IN_PROGRESS;
}

static void
xhci_device_isoc_abort(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	usbd_xfer_abort(xfer);
}

static void
xhci_device_isoc_close(struct usbd_pipe *pipe)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	xhci_close_pipe(pipe);
}

static void
xhci_device_isoc_done(struct usbd_xfer *xfer)
{
#ifdef USB_DEBUG
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
#endif
	const bool isread = usbd_xfer_isread(xfer);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

/* ----------- */
/* device bulk */

static usbd_status
xhci_device_bulk_transfer(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Pipe isn't running, so start it first.  */
	return xhci_device_bulk_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
xhci_device_bulk_start(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];
	struct xhci_xfer * const xx = XHCI_XFER2XXFER(xfer);
	const uint32_t len = xfer->ux_length;
	usb_dma_t * const dma = &xfer->ux_dmabuf;
	uint64_t parameter;
	uint32_t status;
	uint32_t control;
	u_int i = 0;
	const bool polling = xhci_polling_p(sc);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	KASSERT(polling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	KASSERT((xfer->ux_rqflags & URQ_REQUEST) == 0);

	if (tr->is_halted)
		goto out;

	parameter = DMAADDR(dma, 0);
	const bool isread = usbd_xfer_isread(xfer);
	if (len)
		usb_syncmem(dma, 0, len,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	/*
	 * XXX: (dsl) The physical buffer must not cross a 64k boundary.
	 * If the user supplied buffer crosses such a boundary then 2
	 * (or more) TRB should be used.
	 * If multiple TRB are used the td_size field must be set correctly.
	 * For v1.0 devices (like ivy bridge) this is the number of usb data
	 * blocks needed to complete the transfer.
	 * Setting it to 1 in the last TRB causes an extra zero-length
	 * data block be sent.
	 * The earlier documentation differs, I don't know how it behaves.
	 */
	KASSERTMSG(len <= 0x10000, "len %d", len);
	status = XHCI_TRB_2_IRQ_SET(0) |
	    XHCI_TRB_2_TDSZ_SET(0) |
	    XHCI_TRB_2_BYTES_SET(len);
	control = XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_NORMAL) |
	    (isread ? XHCI_TRB_3_ISP_BIT : 0) |
	    XHCI_TRB_3_IOC_BIT;
	xhci_xfer_put_trb(xx, i++, parameter, status, control);

	if (!polling)
		mutex_enter(&tr->xr_lock);
	xhci_ring_put_xfer(sc, tr, xx, i);
	if (!polling)
		mutex_exit(&tr->xr_lock);

	xhci_db_write_4(sc, XHCI_DOORBELL(xs->xs_idx), dci);

out:	if (xfer->ux_status == USBD_NOT_STARTED) {
		xfer->ux_status = USBD_IN_PROGRESS;
		usbd_xfer_schedule_timeout(xfer);
	} else {
		/*
		 * We must be coming from xhci_pipe_restart -- timeout
		 * already set up, nothing to do.
		 */
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	return USBD_IN_PROGRESS;
}

static void
xhci_device_bulk_done(struct usbd_xfer *xfer)
{
#ifdef USB_DEBUG
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
#endif
	const bool isread = usbd_xfer_isread(xfer);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

static void
xhci_device_bulk_abort(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	usbd_xfer_abort(xfer);
}

static void
xhci_device_bulk_close(struct usbd_pipe *pipe)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	xhci_close_pipe(pipe);
}

/* ---------------- */
/* device interrupt */

static usbd_status
xhci_device_intr_transfer(struct usbd_xfer *xfer)
{
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Pipe isn't running, so start it first.  */
	return xhci_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
xhci_device_intr_start(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc = XHCI_XFER2SC(xfer);
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
	struct xhci_ring * const tr = xs->xs_xr[dci];
	struct xhci_xfer * const xx = XHCI_XFER2XXFER(xfer);
	const uint32_t len = xfer->ux_length;
	const bool polling = xhci_polling_p(sc);
	usb_dma_t * const dma = &xfer->ux_dmabuf;
	uint64_t parameter;
	uint32_t status;
	uint32_t control;
	u_int i = 0;

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	KASSERT(polling || mutex_owned(&sc->sc_lock));

	if (sc->sc_dying)
		return USBD_IOERROR;

	if (tr->is_halted)
		goto out;

	KASSERT((xfer->ux_rqflags & URQ_REQUEST) == 0);

	const bool isread = usbd_xfer_isread(xfer);
	if (len)
		usb_syncmem(dma, 0, len,
		    isread ? BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);

	parameter = DMAADDR(dma, 0);
	KASSERTMSG(len <= 0x10000, "len %d", len);
	status = XHCI_TRB_2_IRQ_SET(0) |
	    XHCI_TRB_2_TDSZ_SET(0) |
	    XHCI_TRB_2_BYTES_SET(len);
	control = XHCI_TRB_3_TYPE_SET(XHCI_TRB_TYPE_NORMAL) |
	    (isread ? XHCI_TRB_3_ISP_BIT : 0) | XHCI_TRB_3_IOC_BIT;
	xhci_xfer_put_trb(xx, i++, parameter, status, control);

	if (!polling)
		mutex_enter(&tr->xr_lock);
	xhci_ring_put_xfer(sc, tr, xx, i);
	if (!polling)
		mutex_exit(&tr->xr_lock);

	xhci_db_write_4(sc, XHCI_DOORBELL(xs->xs_idx), dci);

out:	if (xfer->ux_status == USBD_NOT_STARTED) {
		xfer->ux_status = USBD_IN_PROGRESS;
		usbd_xfer_schedule_timeout(xfer);
	} else {
		/*
		 * We must be coming from xhci_pipe_restart -- timeout
		 * already set up, nothing to do.
		 */
	}
	KASSERT(xfer->ux_status == USBD_IN_PROGRESS);

	return USBD_IN_PROGRESS;
}

static void
xhci_device_intr_done(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc __diagused = XHCI_XFER2SC(xfer);
#ifdef USB_DEBUG
	struct xhci_slot * const xs = xfer->ux_pipe->up_dev->ud_hcpriv;
	const u_int dci = xhci_ep_get_dci(xfer->ux_pipe->up_endpoint->ue_edesc);
#endif
	const bool isread = usbd_xfer_isread(xfer);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx slot %ju dci %ju",
	    (uintptr_t)xfer, xs->xs_idx, dci, 0);

	KASSERT(xhci_polling_p(sc) || mutex_owned(&sc->sc_lock));

	usb_syncmem(&xfer->ux_dmabuf, 0, xfer->ux_length,
	    isread ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
}

static void
xhci_device_intr_abort(struct usbd_xfer *xfer)
{
	struct xhci_softc * const sc __diagused = XHCI_XFER2SC(xfer);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx", (uintptr_t)xfer, 0, 0, 0);

	KASSERT(mutex_owned(&sc->sc_lock));
	usbd_xfer_abort(xfer);
}

static void
xhci_device_intr_close(struct usbd_pipe *pipe)
{
	//struct xhci_softc * const sc = XHCI_PIPE2SC(pipe);

	XHCIHIST_FUNC();
	XHCIHIST_CALLARGS("%#jx", (uintptr_t)pipe, 0, 0, 0);

	xhci_close_pipe(pipe);
}
