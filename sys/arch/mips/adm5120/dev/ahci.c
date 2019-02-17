/*	$NetBSD: ahci.c,v 1.17 2019/02/17 04:17:52 rin Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tetsuya Isaki.
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
 * !! HIGHLY EXPERIMENTAL CODE !!
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ahci.c,v 1.17 2019/02/17 04:17:52 rin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbroothub.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_obiovar.h>

#include <mips/adm5120/dev/ahcireg.h>
#include <mips/adm5120/dev/ahcivar.h>

static usbd_status	ahci_open(struct usbd_pipe *);
static void		ahci_softintr(void *);
static void		ahci_poll(struct usbd_bus *);
static void		ahci_poll_hub(void *);
static void		ahci_poll_device(void *arg);
static struct usbd_xfer *
			ahci_allocx(struct usbd_bus *, unsigned int);
static void		ahci_freex(struct usbd_bus *, struct usbd_xfer *);

static void		ahci_get_lock(struct usbd_bus *, kmutex_t **);
static int		ahci_roothub_ctrl(struct usbd_bus *, usb_device_request_t *,
    void *, int);

static usbd_status	ahci_root_intr_transfer(struct usbd_xfer *);
static usbd_status	ahci_root_intr_start(struct usbd_xfer *);
static void		ahci_root_intr_abort(struct usbd_xfer *);
static void		ahci_root_intr_close(struct usbd_pipe *);
static void		ahci_root_intr_done(struct usbd_xfer *);

static usbd_status	ahci_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status	ahci_device_ctrl_start(struct usbd_xfer *);
static void		ahci_device_ctrl_abort(struct usbd_xfer *);
static void		ahci_device_ctrl_close(struct usbd_pipe *);
static void		ahci_device_ctrl_done(struct usbd_xfer *);

static usbd_status	ahci_device_intr_transfer(struct usbd_xfer *);
static usbd_status	ahci_device_intr_start(struct usbd_xfer *);
static void		ahci_device_intr_abort(struct usbd_xfer *);
static void		ahci_device_intr_close(struct usbd_pipe *);
static void		ahci_device_intr_done(struct usbd_xfer *);

static usbd_status	ahci_device_isoc_transfer(struct usbd_xfer *);
static usbd_status	ahci_device_isoc_start(struct usbd_xfer *);
static void		ahci_device_isoc_abort(struct usbd_xfer *);
static void		ahci_device_isoc_close(struct usbd_pipe *);
static void		ahci_device_isoc_done(struct usbd_xfer *);

static usbd_status	ahci_device_bulk_transfer(struct usbd_xfer *);
static usbd_status	ahci_device_bulk_start(struct usbd_xfer *);
static void		ahci_device_bulk_abort(struct usbd_xfer *);
static void		ahci_device_bulk_close(struct usbd_pipe *);
static void		ahci_device_bulk_done(struct usbd_xfer *);

static int		ahci_transaction(struct ahci_softc *,
	struct usbd_pipe *, uint8_t, int, u_char *, uint8_t);
static void		ahci_noop(struct usbd_pipe *);
static void		ahci_abort_xfer(struct usbd_xfer *, usbd_status);
static void		ahci_device_clear_toggle(struct usbd_pipe *);

extern int usbdebug;
extern int uhubdebug;
extern int umassdebug;
int ahci_dummy;

#define AHCI_DEBUG

#ifdef AHCI_DEBUG
#define D_TRACE	(0x0001)	/* function trace */
#define D_MSG	(0x0002)	/* debug messages */
#define D_XFER	(0x0004)	/* transfer messages (noisy!) */
#define D_MEM	(0x0008)	/* memory allocation */

int ahci_debug = 0;
#define DPRINTF(z,x)	if((ahci_debug&(z))!=0)printf x
void		print_req(usb_device_request_t *);
void		print_req_hub(usb_device_request_t *);
void		print_dumpreg(struct ahci_softc *);
void		print_xfer(struct usbd_xfer *);
#else
#define DPRINTF(z,x)
#endif


struct usbd_bus_methods ahci_bus_methods = {
	.ubm_open = ahci_open,
	.ubm_softint = ahci_softintr,
	.ubm_dopoll = ahci_poll,
	.ubm_allocx = ahci_allocx,
	.ubm_freex = ahci_freex,
	.ubm_getlock = ahci_get_lock,
	.ubm_rhctrl = ahci_roothub_ctrl,
};

struct usbd_pipe_methods ahci_root_intr_methods = {
	.upm_transfer = ahci_root_intr_transfer,
	.upm_start = ahci_root_intr_start,
	.upm_abort = ahci_root_intr_abort,
	.upm_close = ahci_root_intr_close,
	.upm_cleartoggle = ahci_noop,
	.upm_done = ahci_root_intr_done,
};

struct usbd_pipe_methods ahci_device_ctrl_methods = {
	.upm_transfer = ahci_device_ctrl_transfer,
	.upm_start = ahci_device_ctrl_start,
	.upm_abort = ahci_device_ctrl_abort,
	.upm_close = ahci_device_ctrl_close,
	.upm_cleartoggle = ahci_noop,
	.upm_done = ahci_device_ctrl_done,
};

struct usbd_pipe_methods ahci_device_intr_methods = {
	.upm_transfer = ahci_device_intr_transfer,
	.upm_start = ahci_device_intr_start,
	.upm_abort = ahci_device_intr_abort,
	.upm_close = ahci_device_intr_close,
	.upm_cleartoggle = ahci_device_clear_toggle,
	.upm_done = ahci_device_intr_done,
};

struct usbd_pipe_methods ahci_device_isoc_methods = {
	.upm_transfer = ahci_device_isoc_transfer,
	.upm_start = ahci_device_isoc_start,
	.upm_abort = ahci_device_isoc_abort,
	.upm_close = ahci_device_isoc_close,
	.upm_cleartoggle = ahci_noop,
	.upm_done = ahci_device_isoc_done,
};

struct usbd_pipe_methods ahci_device_bulk_methods = {
	.upm_transfer = ahci_device_bulk_transfer,
	.upm_start = ahci_device_bulk_start,
	.upm_abort = ahci_device_bulk_abort,
	.upm_close = ahci_device_bulk_close,
	.upm_cleartoggle = ahci_device_clear_toggle,
	.upm_done = ahci_device_bulk_done,
};

struct ahci_pipe {
	struct usbd_pipe pipe;
	uint32_t toggle;
};

static int	ahci_match(device_t, cfdata_t, void *);
static void	ahci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ahci, sizeof(struct ahci_softc),
    ahci_match, ahci_attach, NULL, NULL);

static int
ahci_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *aa = aux;

	if (strcmp(aa->oba_name, cf->cf_name) == 0)
		return 1;

	return 0;
}

#define	REG_READ(o)	bus_space_read_4(sc->sc_st, sc->sc_ioh, (o))
#define	REG_WRITE(o,v)	bus_space_write_4(sc->sc_st, sc->sc_ioh, (o),(v))

/*
 * Attach SL11H/SL811HS. Return 0 if success.
 */
void
ahci_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args *aa = aux;
	struct ahci_softc *sc = device_private(self);

	printf("\n");
	sc->sc_dmat = aa->oba_dt;
	sc->sc_st = aa->oba_st;

	/* Initialize sc */
	sc->sc_bus.ub_revision = USBREV_1_1;
	sc->sc_bus.ub_methods = &ahci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct ahci_pipe);
	sc->sc_bus.ub_dmatag = sc->sc_dmat;
	sc->sc_bus.ub_usedma = true;

	/* Map the device. */
	if (bus_space_map(sc->sc_st, aa->oba_addr,
	    512, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	/* Hook up the interrupt handler. */
	sc->sc_ih = adm5120_intr_establish(aa->oba_irq, INTR_IRQ, ahci_intr, sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self,
		    "unable to register interrupt handler\n");
		return;
	}

	SIMPLEQ_INIT(&sc->sc_free_xfers);

	callout_init(&sc->sc_poll_handle, 0);

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED /* XXXNH */);

	REG_WRITE(ADMHCD_REG_INTENABLE, 0); /* disable interrupts */
	REG_WRITE(ADMHCD_REG_CONTROL, ADMHCD_SW_RESET); /* reset */
	delay_ms(10);
	while (REG_READ(ADMHCD_REG_CONTROL) & ADMHCD_SW_RESET)
		delay_ms(1);

	REG_WRITE(ADMHCD_REG_CONTROL, ADMHCD_HOST_EN);
	REG_WRITE(ADMHCD_REG_HOSTHEAD, 0x00000000);
	REG_WRITE(ADMHCD_REG_FMINTERVAL, 0x20002edf);
	REG_WRITE(ADMHCD_REG_LSTHRESH, 0x628);
	REG_WRITE(ADMHCD_REG_RHDESCR, ADMHCD_NPS | ADMHCD_LPSC);
	REG_WRITE(ADMHCD_REG_HOSTCONTROL, ADMHCD_STATE_OP);

	REG_WRITE(ADMHCD_REG_INTENABLE, 0); /* XXX: enable interrupts */

#ifdef USB_DEBUG
	/* usbdebug = 0x7f;
	uhubdebug = 0x7f;
	umassdebug = 0xffffffff; */
#endif

	/* Attach USB devices */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);

}

int
ahci_intr(void *arg)
{
#if 0
	struct ahci_softc *sc = arg;
	uint8_t r;
#ifdef AHCI_DEBUG
	char bitbuf[256];
#endif

	r = sl11read(sc, SL11_ISR);

	sl11write(sc, SL11_ISR, SL11_ISR_DATA | SL11_ISR_SOFTIMER);

	if ((r & SL11_ISR_RESET)) {
		sc->sc_flags |= AHCDF_RESET;
		sl11write(sc, SL11_ISR, SL11_ISR_RESET);
	}
	if ((r & SL11_ISR_INSERT)) {
		sc->sc_flags |= AHCDF_INSERT;
		sl11write(sc, SL11_ISR, SL11_ISR_INSERT);
	}

#ifdef AHCI_DEBUG
	snprintb(bitbuf, sizeof(bitbuf),
	    ((sl11read(sc, SL11_CTRL) & SL11_CTRL_SUSPEND)
	    ? "\20\x8""D+\7RESUME\6INSERT\5SOF\4res\3""BABBLE\2USBB\1USBA"
	    : "\20\x8""D+\7RESET\6INSERT\5SOF\4res\3""BABBLE\2USBB\1USBA"),
	    r);

	DPRINTF(D_XFER, ("I=%s ", bitbuf));
#endif /* AHCI_DEBUG */
#endif

	return 0;
}

usbd_status
ahci_open(struct usbd_pipe *pipe)
{
	struct usbd_device *dev = pipe->up_dev;
	struct ahci_pipe *apipe = (struct ahci_pipe *)pipe;
	usb_endpoint_descriptor_t *ed = pipe->up_endpoint->ue_edesc;
	uint8_t rhaddr = dev->ud_bus->ub_rhaddr;

	DPRINTF(D_TRACE, ("ahci_open(addr=%d,ep=%d,scaddr=%d)",
		dev->ud_addr, ed->bEndpointAddress, rhaddr));

	apipe->toggle=0;

	if (dev->ud_addr == rhaddr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->up_methods = &roothub_ctrl_methods;
			break;
		case UE_DIR_IN | USBROOTHUB_INTR_ENDPT:
			pipe->up_methods = &ahci_root_intr_methods;
			break;
		default:
			printf("open:endpointErr!\n");
			return USBD_INVAL;
		}
	} else {
		switch (ed->bmAttributes & UE_XFERTYPE) {
		case UE_CONTROL:
			DPRINTF(D_MSG, ("control "));
			pipe->up_methods = &ahci_device_ctrl_methods;
			break;
		case UE_INTERRUPT:
			DPRINTF(D_MSG, ("interrupt "));
			pipe->up_methods = &ahci_device_intr_methods;
			break;
		case UE_ISOCHRONOUS:
			DPRINTF(D_MSG, ("isochronous "));
			pipe->up_methods = &ahci_device_isoc_methods;
			break;
		case UE_BULK:
			DPRINTF(D_MSG, ("bluk "));
			pipe->up_methods = &ahci_device_bulk_methods;
			break;
		}
	}
	return USBD_NORMAL_COMPLETION;
}

void
ahci_softintr(void *arg)
{
	DPRINTF(D_TRACE, ("%s()", __func__));
}

void
ahci_poll(struct usbd_bus *bus)
{
	DPRINTF(D_TRACE, ("%s()", __func__));
}

#define AHCI_BUS2SC(bus)	((bus)->ub_hcpriv)
#define AHCI_PIPE2SC(pipe)	AHCI_BUS2SC((pipe)->up_dev->ud_bus)
#define AHCI_XFER2SC(xfer)	AHCI_BUS2SC((xfer)->ux_bus)
#define AHCI_APIPE2SC(ap)	AHCI_BUS2SC((d)->pipe.up_dev->ud_bus)

/*
 * Emulation of interrupt transfer for status change endpoint
 * of root hub.
 */
void
ahci_poll_hub(void *arg)
{
	struct usbd_xfer *xfer = arg;
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	u_char *p;
	static int p0_state=0;
	static int p1_state=0;

	callout_reset(&sc->sc_poll_handle, sc->sc_interval, ahci_poll_hub, xfer);

	/* USB spec 11.13.3 (p.260) */
	p = KERNADDR(&xfer->ux_dmabuf, 0);
	p[0] = 0;
	if ((REG_READ(ADMHCD_REG_PORTSTATUS0) & ADMHCD_CCS) != p0_state) {
		p[0] = 2;
		DPRINTF(D_TRACE, ("!"));
		p0_state=(REG_READ(ADMHCD_REG_PORTSTATUS0) & ADMHCD_CCS);
	};
	if ((REG_READ(ADMHCD_REG_PORTSTATUS1) & ADMHCD_CCS) != p1_state) {
		p[0] = 2;
		DPRINTF(D_TRACE, ("@"));
		p1_state=(REG_READ(ADMHCD_REG_PORTSTATUS1) & ADMHCD_CCS);
	};

	/* no change, return NAK */
	if (p[0] == 0)
		return;

	xfer->ux_actlen = 1;
	xfer->ux_status = USBD_NORMAL_COMPLETION;
	mutex_enter(&sc->sc_lock);
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
}

struct usbd_xfer *
ahci_allocx(struct usbd_bus *bus, unsigned int nframes)
{
	struct ahci_softc *sc = AHCI_BUS2SC(bus);
	struct usbd_xfer *xfer;

	DPRINTF(D_MEM, ("SLallocx"));

	xfer = SIMPLEQ_FIRST(&sc->sc_free_xfers);
	if (xfer) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, ux_next);
#ifdef DIAGNOSTIC
		if (xfer->ux_state != XFER_FREE) {
			printf("ahci_allocx: xfer=%p not free, 0x%08x\n",
				xfer, xfer->ux_state);
		}
#endif
	} else {
		xfer = kmem_alloc(sizeof(*xfer), KM_SLEEP);
	}

	memset(xfer, 0, sizeof(*xfer));
#ifdef DIAGNOSTIC
	xfer->ux_state = XFER_BUSY;
#endif

	return xfer;
}

void
ahci_freex(struct usbd_bus *bus, struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_BUS2SC(bus);

	DPRINTF(D_MEM, ("SLfreex"));

#ifdef DIAGNOSTIC
	if (xfer->ux_state != XFER_BUSY &&
	    xfer->ux_status != USBD_NOT_STARTED) {
		printf("ahci_freex: xfer=%p not busy, 0x%08x\n",
			xfer, xfer->ux_state);
		return;
	}
	xfer->ux_state = XFER_FREE;
#endif
	SIMPLEQ_INSERT_HEAD(&sc->sc_free_xfers, xfer, ux_next);
}

static void
ahci_get_lock(struct usbd_bus *bus, kmutex_t **lock)
{
	struct ahci_softc *sc = AHCI_BUS2SC(bus);

	*lock = &sc->sc_lock;
}

void
ahci_noop(struct usbd_pipe *pipe)
{
	DPRINTF(D_TRACE, ("%s()", __func__));
}

/*
 * Data structures and routines to emulate the root hub.
 */

static int
ahci_roothub_ctrl(struct usbd_bus *bus, usb_device_request_t *req,
    void *buf, int buflen)
{
	struct ahci_softc *sc = AHCI_BUS2SC(bus);
	uint16_t len, value, index;
	usb_port_status_t ps;
	int totlen = 0;
	int status;

	DPRINTF(D_TRACE, ("SLRCstart "));

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

#define C(x,y) ((x) | ((y) << 8))
	switch (C(req->bRequest, req->bmRequestType)) {
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		switch (value) {
#define sd ((usb_string_descriptor_t *)buf)
		case C(2, UDESC_STRING):
			/* Product */
			totlen = usb_makestrdesc(sd, len, "ADM5120 root hub");
			break;
		default:
			printf("unknownGetDescriptor=%x", value);
			/* FALLTHROUGH */
		case C(0, UDESC_DEVICE):
		case C(1, UDESC_STRING):
			/* default from usbroothub */
			return buflen;
		}
		break;
	/*
	 * Hub specific requests
	 */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		/* Clear Hub Feature, 11.16.2.1, not supported */
		DPRINTF(D_MSG, ("ClearHubFeature not supported\n"));
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):

#define WPS(x) REG_WRITE(ADMHCD_REG_PORTSTATUS0+(index-1)*4, (x))
		/* Clear Port Feature, 11.16.2.2 */
		if (index != 1 && index != 2 ) {
			return -1;
		}
		switch (value) {
		case UHF_PORT_POWER:
			DPRINTF(D_MSG, ("POWER_OFF "));
			WPS(ADMHCD_LSDA);
			break;
		case UHF_PORT_SUSPEND:
			DPRINTF(D_MSG, ("SUSPEND "));
			WPS(ADMHCD_POCI);
			break;
		case UHF_PORT_ENABLE:
			DPRINTF(D_MSG, ("ENABLE "));
			WPS(ADMHCD_CCS);
			break;
		case UHF_C_PORT_CONNECTION:
			WPS(ADMHCD_CSC);
			break;
		case UHF_C_PORT_RESET:
			WPS(ADMHCD_PRSC);
			break;
		case UHF_C_PORT_SUSPEND:
			WPS(ADMHCD_PSSC);
			break;
		case UHF_C_PORT_ENABLE:
			WPS(ADMHCD_PESC);
			break;
		case UHF_C_PORT_OVER_CURRENT:
			WPS(ADMHCD_OCIC);
			break;
		default:
			printf("ClrPortFeatERR:value=0x%x ", value);
			return -1;
		}
		//DPRINTF(D_XFER, ("CH=%04x ", sc->sc_change));
#undef WPS
		break;
	case C(UR_GET_BUS_STATE, UT_READ_CLASS_OTHER):
		/* Get Bus State, 11.16.2.3, not supported */
		/* shall return a STALL... */
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		/* Get Hub Descriptor, 11.16.2.4 */
		DPRINTF(D_MSG, ("UR_GET_DESCRIPTOR RCD"));
		if ((value&0xff) != 0) {
			return -1;
		}
		usb_hub_descriptor_t hubd;

		totlen = uimin(buflen, sizeof(hubd));
		memcpy(&hubd, buf, totlen);
		hubd.bNbrPorts = 2;
		USETW(hubd.wHubCharacteristics, 0);
		hubd.bPwrOn2PwrGood = 0;
		memcpy(buf, &hubd, totlen);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		/* Get Hub Status, 11.16.2.5 */
		DPRINTF(D_MSG, ("UR_GET_STATUS RCD"));
		if (len != 4) {
			return -1;
		}
		memset(buf, 0, len);
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		/* Get Port Status, 11.16.2.6 */
		if ((index != 1 && index != 2)  || len != 4) {
			printf("index=%d,len=%d ", index, len);
			return -1;
		}
		status = REG_READ(ADMHCD_REG_PORTSTATUS0+(index-1)*4);
		DPRINTF(D_MSG, ("UR_GET_STATUS RCO=%x ", status));

		//DPRINTF(D_XFER, ("ST=%04x,CH=%04x ", status, sc->sc_change));
		USETW(ps.wPortStatus, status  & (UPS_CURRENT_CONNECT_STATUS|UPS_PORT_ENABLED|UPS_SUSPEND|UPS_OVERCURRENT_INDICATOR|UPS_RESET|UPS_PORT_POWER|UPS_LOW_SPEED));
		USETW(ps.wPortChange, (status>>16) & (UPS_C_CONNECT_STATUS|UPS_C_PORT_ENABLED|UPS_C_SUSPEND|UPS_C_OVERCURRENT_INDICATOR|UPS_C_PORT_RESET));
		totlen = uimin(len, sizeof(ps));
		memcpy(buf, &ps, totlen);
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		/* Set Hub Descriptor, 11.16.2.7, not supported */
		/* STALL ? */
		return -1;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		/* Set Hub Feature, 11.16.2.8, not supported */
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
#define WPS(x) REG_WRITE(ADMHCD_REG_PORTSTATUS0+(index-1)*4, (x))
		/* Set Port Feature, 11.16.2.9 */
		if ((index != 1) && (index !=2)) {
			printf("index=%d ", index);
			return -1;
		}
		switch (value) {
		case UHF_PORT_RESET:
			DPRINTF(D_MSG, ("PORT_RESET "));
			WPS(ADMHCD_PRS);
			break;
		case UHF_PORT_POWER:
			DPRINTF(D_MSG, ("PORT_POWER "));
			WPS(ADMHCD_PPS);
			break;
		case UHF_PORT_ENABLE:
			DPRINTF(D_MSG, ("PORT_ENABLE "));
			WPS(ADMHCD_PES);
			break;
		default:
			printf("SetPortFeatERR=0x%x ", value);
			return -1;
		}
#undef WPS
		break;
	default:
		DPRINTF(D_MSG, ("ioerr(UR=%02x,UT=%02x) ",
			req->bRequest, req->bmRequestType));
		/* default from usbroothub */
		return buflen;
	}

	return totlen;
}

static usbd_status
ahci_root_intr_transfer(struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	usbd_status error;

	DPRINTF(D_TRACE, ("SLRItransfer "));

	/* Insert last in queue */
	mutex_enter(&sc->sc_lock);
	error = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (error)
		return error;

	/*
	 * Pipe isn't running (otherwise error would be USBD_INPROG),
	 * start first.
	 */
	return ahci_root_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
ahci_root_intr_start(struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);

	DPRINTF(D_TRACE, ("SLRIstart "));

	sc->sc_interval = MS_TO_TICKS(xfer->ux_pipe->up_endpoint->ue_edesc->bInterval);
	callout_reset(&sc->sc_poll_handle, sc->sc_interval, ahci_poll_hub, xfer);
	sc->sc_intr_xfer = xfer;
	return USBD_IN_PROGRESS;
}

static void
ahci_root_intr_abort(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("SLRIabort "));
}

static void
ahci_root_intr_close(struct usbd_pipe *pipe)
{
	struct ahci_softc *sc = AHCI_PIPE2SC(pipe);

	DPRINTF(D_TRACE, ("SLRIclose "));

	callout_stop(&sc->sc_poll_handle);
	sc->sc_intr_xfer = NULL;
}

static void
ahci_root_intr_done(struct usbd_xfer *xfer)
{
	//DPRINTF(D_XFER, ("RIdn "));
}

static usbd_status
ahci_device_ctrl_transfer(struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	usbd_status error;

	DPRINTF(D_TRACE, ("C"));

	mutex_enter(&sc->sc_lock);
	error = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (error)
		return error;

	return ahci_device_ctrl_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
ahci_device_ctrl_start(struct usbd_xfer *xfer)
{
	usbd_status status =  USBD_NORMAL_COMPLETION;
	int s, err;
	static struct admhcd_ed ep_v __attribute__((aligned(16))), *ep;
	static struct admhcd_td td_v[4] __attribute__((aligned(16))), *td, *td1, *td2, *td3;
	static usb_dma_t reqdma;
	struct usbd_pipe *pipe = xfer->ux_pipe;
	usb_device_request_t *req = &xfer->ux_request;
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	int len, isread;


#if 0
	struct ahci_pipe *apipe = (struct ahci_pipe *)xfer->ux_pipe;
#endif
	mutex_enter(&sc->sc_lock);
/*	printf("ctrl_start>>>\n"); */

#ifdef DIAGNOSTIC
	if (!(xfer->ux_rqflags & URQ_REQUEST)) {
		/* XXX panic */
		printf("ahci_device_ctrl_transfer: not a request\n");
		return USBD_INVAL;
	}
#endif

#define KSEG1ADDR(x) (0xa0000000 | (((uint32_t)x) & 0x1fffffff))
	DPRINTF(D_TRACE, ("st "));
	if (!ep) {
		ep = (struct admhcd_ed *)KSEG1ADDR(&ep_v);
		td = (struct admhcd_td *)KSEG1ADDR(&td_v[0]);
		td1 = (struct admhcd_td *)KSEG1ADDR(&td_v[1]);
		td2 = (struct admhcd_td *)KSEG1ADDR(&td_v[2]);
		td3 = (struct admhcd_td *)KSEG1ADDR(&td_v[3]);
		err = usb_allocmem(&sc->sc_bus,
			sizeof(usb_device_request_t),
			0, &reqdma);
		if (err)
			return USBD_NOMEM;

		/* printf("ep: %p\n",ep); */
	};

	ep->control =  pipe->up_dev->ud_addr | \
		((pipe->up_dev->ud_speed==USB_SPEED_FULL)?ADMHCD_ED_SPEED:0) | \
		((UGETW(pipe->up_endpoint->ue_edesc->wMaxPacketSize))<<ADMHCD_ED_MAXSHIFT);
	memcpy(KERNADDR(&reqdma, 0), req, sizeof *req);
/* 	printf("status: %x\n",REG_READ(ADMHCD_REG_PORTSTATUS0));
	printf("ep_control: %x\n",ep->control);
	printf("speed: %x\n",pipe->up_dev->ud_speed);
	printf("req: %p\n",req);
	printf("dmabuf: %p\n",xfer->ux_dmabuf.block); */

	isread = req->bmRequestType & UT_READ;
	len = UGETW(req->wLength);

	ep->next = ep;

	td->buffer = DMAADDR(&reqdma,0) | 0xa0000000;
	td->buflen=sizeof(*req);
	td->control=ADMHCD_TD_SETUP | ADMHCD_TD_DATA0 | ADMHCD_TD_OWN;

	if (len) {
		td->next = td1;

		td1->buffer = DMAADDR(&xfer->ux_dmabuf,0) | 0xa0000000;
		td1->buflen = len;
		td1->next = td2;
		td1->control= (isread?ADMHCD_TD_IN:ADMHCD_TD_OUT) | ADMHCD_TD_DATA1 | ADMHCD_TD_R | ADMHCD_TD_OWN;
	} else {
		td1->control = 0;
		td->next = td2;
	};

	td2->buffer = 0;
	td2->buflen= 0;
	td2->next = td3;
	td2->control = (isread?ADMHCD_TD_OUT:ADMHCD_TD_IN) | ADMHCD_TD_DATA1 | ADMHCD_TD_OWN;

	td3->buffer = 0;
	td3->buflen= 0;
	td3->next = 0;
	td3->control = 0;

	ep->head = td;
	ep->tail = td3;
/*
	printf("ep: %p\n",ep);
	printf("ep->next: %p\n",ep->next);
	printf("ep->head: %p\n",ep->head);
	printf("ep->tail: %p\n",ep->tail);
	printf("td: %p\n",td);
	printf("td->next: %p\n",td->next);
	printf("td->buffer: %x\n",td->buffer);
	printf("td->buflen: %x\n",td->buflen);
	printf("td1: %p\n",td1);
	printf("td1->next: %p\n",td1->next);
	printf("td2: %p\n",td2);
	printf("td2->next: %p\n",td2->next);
	printf("td3: %p\n",td3);
	printf("td3->next: %p\n",td3->next);
*/

	REG_WRITE(ADMHCD_REG_HOSTHEAD, (uint32_t)ep);
	REG_WRITE(ADMHCD_REG_HOSTCONTROL, ADMHCD_STATE_OP | ADMHCD_DMA_EN);
/*	printf("1: %x %x %x %x\n", ep->control, td->control, td1->control, td2->control); */
	s=100;
	while (s--) {
		delay_ms(10);
/*                printf("%x %x %x %x\n", ep->control, td->control, td1->control, td2->control);*/
		status = USBD_TIMEOUT;
		if (td->control & ADMHCD_TD_OWN) continue;

		err = (td->control & ADMHCD_TD_ERRMASK)>>ADMHCD_TD_ERRSHIFT;
		if (err) {
			status = USBD_IOERROR;
			break;
		};

		status = USBD_TIMEOUT;
		if (td1->control & ADMHCD_TD_OWN) continue;
		err = (td1->control & ADMHCD_TD_ERRMASK)>>ADMHCD_TD_ERRSHIFT;
		if (err) {
			status = USBD_IOERROR;
			break;
		};

		status = USBD_TIMEOUT;
		if (td2->control & ADMHCD_TD_OWN) continue;
		err = (td2->control & ADMHCD_TD_ERRMASK)>>ADMHCD_TD_ERRSHIFT;
		if (err) {
			status = USBD_IOERROR;
		};
		status = USBD_NORMAL_COMPLETION;
		break;

	};
	REG_WRITE(ADMHCD_REG_HOSTCONTROL, ADMHCD_STATE_OP);

	xfer->ux_actlen = len;
	xfer->ux_status = status;

/* 	printf("ctrl_start<<<\n"); */

	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
	return USBD_NORMAL_COMPLETION;
}

static void
ahci_device_ctrl_abort(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Cab "));
	ahci_abort_xfer(xfer, USBD_CANCELLED);
}

static void
ahci_device_ctrl_close(struct usbd_pipe *pipe)
{
	DPRINTF(D_TRACE, ("Ccl "));
}

static void
ahci_device_ctrl_done(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Cdn "));
}

static usbd_status
ahci_device_intr_transfer(struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	usbd_status error;

	DPRINTF(D_TRACE, ("INTRtrans "));

	mutex_enter(&sc->sc_lock);
	error = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (error)
		return error;

	return ahci_device_intr_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
ahci_device_intr_start(struct usbd_xfer *xfer)
{
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct ahci_xfer *sx;

	DPRINTF(D_TRACE, ("INTRstart "));

	sx = kmem_intr_alloc(sizeof(*sx), KM_NOSLEEP);
	if (sx == NULL)
		goto reterr;
	memset(sx, 0, sizeof(*sx));
	sx->sx_xfer = xfer;
	xfer->ux_hcpriv = sx;

	/* initialize callout */
	callout_init(&sx->sx_callout_t, 0);
	callout_reset(&sx->sx_callout_t,
		MS_TO_TICKS(pipe->up_endpoint->ue_edesc->bInterval),
		ahci_poll_device, sx);

	/* ACK */
	return USBD_IN_PROGRESS;

 reterr:
	return USBD_IOERROR;
}

static void
ahci_poll_device(void *arg)
{
	struct ahci_xfer *sx = (struct ahci_xfer *)arg;
	struct usbd_xfer *xfer = sx->sx_xfer;
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	void *buf;
	int pid;
	int r;

	DPRINTF(D_TRACE, ("pldev"));

	callout_reset(&sx->sx_callout_t,
		MS_TO_TICKS(pipe->up_endpoint->ue_edesc->bInterval),
		ahci_poll_device, sx);

	/* interrupt transfer */
	pid = (UE_GET_DIR(pipe->up_endpoint->ue_edesc->bEndpointAddress) == UE_DIR_IN)
	    ? ADMHCD_TD_IN : ADMHCD_TD_OUT;
	buf = KERNADDR(&xfer->ux_dmabuf, 0);

	r = ahci_transaction(sc, pipe, pid, xfer->ux_length, buf, 0/*toggle*/);
	if (r < 0) {
		DPRINTF(D_MSG, ("%s error", __func__));
		return;
	}
	/* no change, return NAK */
	if (r == 0)
		return;

	xfer->ux_status = USBD_NORMAL_COMPLETION;
	mutex_enter(&sc->sc_lock);
	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);
}

static void
ahci_device_intr_abort(struct usbd_xfer *xfer)
{
	struct ahci_xfer *sx;

	DPRINTF(D_TRACE, ("INTRabort "));

	sx = xfer->ux_hcpriv;
	if (sx) {
		callout_stop(&sx->sx_callout_t);
		kmem_intr_free(sx, sizeof(*sx));
		xfer->ux_hcpriv = NULL;
	} else {
		printf("%s: sx == NULL!\n", __func__);
	}
	ahci_abort_xfer(xfer, USBD_CANCELLED);
}

static void
ahci_device_intr_close(struct usbd_pipe *pipe)
{
	DPRINTF(D_TRACE, ("INTRclose "));
}

static void
ahci_device_intr_done(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("INTRdone "));
}

static usbd_status
ahci_device_isoc_transfer(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("S"));
	return USBD_NORMAL_COMPLETION;
}

static usbd_status
ahci_device_isoc_start(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("st "));
	return USBD_NORMAL_COMPLETION;
}

static void
ahci_device_isoc_abort(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Sab "));
}

static void
ahci_device_isoc_close(struct usbd_pipe *pipe)
{
	DPRINTF(D_TRACE, ("Scl "));
}

static void
ahci_device_isoc_done(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Sdn "));
}

static usbd_status
ahci_device_bulk_transfer(struct usbd_xfer *xfer)
{
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	usbd_status error;

	DPRINTF(D_TRACE, ("B"));

	mutex_enter(&sc->sc_lock);
	error = usb_insert_transfer(xfer);
	mutex_exit(&sc->sc_lock);
	if (error)
		return error;

	return ahci_device_bulk_start(SIMPLEQ_FIRST(&xfer->ux_pipe->up_queue));
}

static usbd_status
ahci_device_bulk_start(struct usbd_xfer *xfer)
{
#define NBULK_TDS 32
	static volatile int level = 0;
	usbd_status status =  USBD_NORMAL_COMPLETION;
	int s, err;
	static struct admhcd_ed ep_v __attribute__((aligned(16))), *ep;
	static struct admhcd_td td_v[NBULK_TDS] __attribute__((aligned(16))), *td[NBULK_TDS];
	struct usbd_pipe *pipe = xfer->ux_pipe;
	struct ahci_softc *sc = AHCI_XFER2SC(xfer);
	int endpt, i, len, tlen, segs, offset, isread, toggle, short_ok;
	struct ahci_pipe *apipe = (struct ahci_pipe *)xfer->ux_pipe;

#define KSEG1ADDR(x) (0xa0000000 | (((uint32_t)x) & 0x1fffffff))
	DPRINTF(D_TRACE, ("st "));

#ifdef DIAGNOSTIC
	if (xfer->ux_rqflags & URQ_REQUEST) {
		/* XXX panic */
		printf("ohci_device_bulk_start: a request\n");
		return USBD_INVAL;
	}
#endif

	mutex_enter(&sc->sc_lock);
	level++;
/* 	printf("bulk_start>>>\n"); */

	if (!ep) {
		ep = (struct admhcd_ed *)KSEG1ADDR(&ep_v);
		for (i=0; i<NBULK_TDS; i++) {
			td[i] = (struct admhcd_td *)KSEG1ADDR(&td_v[i]);
		};
/*		printf("ep: %p\n",ep);*/
	};
	if (apipe->toggle == 0) {
		toggle = ADMHCD_TD_DATA0;
	} else {
		toggle = apipe->toggle;
	};

	endpt = pipe->up_endpoint->ue_edesc->bEndpointAddress;
	ep->control = pipe->up_dev->ud_addr | ((endpt & 0xf) << ADMHCD_ED_EPSHIFT)|\
		((pipe->up_dev->ud_speed==USB_SPEED_FULL)?ADMHCD_ED_SPEED:0) | \
		((UGETW(pipe->up_endpoint->ue_edesc->wMaxPacketSize))<<ADMHCD_ED_MAXSHIFT);

	short_ok = xfer->ux_flags & USBD_SHORT_XFER_OK?ADMHCD_TD_R:0;
/*	printf("level: %d\n",level);
	printf("short_xfer: %x\n",short_ok);
	printf("ep_control: %x\n",ep->control);
	printf("speed: %x\n",pipe->up_dev->ud_speed);
	printf("dmabuf: %p\n",xfer->ux_dmabuf.block); */

	isread = UE_GET_DIR(endpt) == UE_DIR_IN;
	len = xfer->ux_length;

	ep->next = ep;

	i = 0;
	offset = 0;
	while ((len>0) || (i==0)) {
		tlen = uimin(len,4096);
		td[i]->buffer = DMAADDR(&xfer->ux_dmabuf,offset) | 0xa0000000;
		td[i]->buflen=tlen;
		td[i]->control=(isread?ADMHCD_TD_IN:ADMHCD_TD_OUT) | toggle | ADMHCD_TD_OWN | short_ok;
		td[i]->len=tlen;
		toggle = ADMHCD_TD_TOGGLE;
		len -= tlen;
		offset += tlen;
		td[i]->next = td[i+1];
		i++;
	};

	td[i]->buffer = 0;
	td[i]->buflen = 0;
	td[i]->control = 0;
	td[i]->next = 0;

	ep->head = td[0];
	ep->tail = td[i];
	segs = i;
	len = 0;

/*	printf("segs: %d\n",segs);
	printf("ep: %p\n",ep);
	printf("ep->control: %x\n",ep->control);
	printf("ep->next: %p\n",ep->next);
	printf("ep->head: %p\n",ep->head);
	printf("ep->tail: %p\n",ep->tail);
	for (i=0; i<segs; i++) {
		printf("td[%d]: %p\n",i,td[i]);
		printf("td[%d]->control: %x\n",i,td[i]->control);
		printf("td[%d]->next: %p\n",i,td[i]->next);
		printf("td[%d]->buffer: %x\n",i,td[i]->buffer);
		printf("td[%d]->buflen: %x\n",i,td[i]->buflen);
	}; */

	REG_WRITE(ADMHCD_REG_HOSTHEAD, (uint32_t)ep);
	REG_WRITE(ADMHCD_REG_HOSTCONTROL, ADMHCD_STATE_OP | ADMHCD_DMA_EN);
	i = 0;
/*	printf("1: %x %d %x %x\n", ep->control, i, td[i]->control, td[i]->buflen); */
	s=100;
	err = 0;
	while (s--) {
/*                printf("%x %d %x %x\n", ep->control, i, td[i]->control, td[i]->buflen); */
		status = USBD_TIMEOUT;
		if (td[i]->control & ADMHCD_TD_OWN) {
			delay_ms(3);
			continue;
		};

		len += td[i]->len - td[i]->buflen;

		err = (td[i]->control & ADMHCD_TD_ERRMASK)>>ADMHCD_TD_ERRSHIFT;
		if (err) {
			status = USBD_IOERROR;
			break;
		};

		i++;
		if (i==segs) {
			status = USBD_NORMAL_COMPLETION;
			break;
		};

	};
	REG_WRITE(ADMHCD_REG_HOSTCONTROL, ADMHCD_STATE_OP);

	apipe->toggle = ((uint32_t)ep->head & 2)?ADMHCD_TD_DATA1:ADMHCD_TD_DATA0;
/*	printf("bulk_transfer_done: status: %x, err: %x, len: %x, toggle: %x\n", status,err,len,apipe->toggle); */

	if (short_ok && (err == 0x9 || err == 0xd)) {
/*		printf("bulk_transfer_done: short_transfer fix\n"); */
		status = USBD_NORMAL_COMPLETION;
	};
	xfer->ux_actlen = len;
	xfer->ux_status = status;

	level--;
/*	printf("bulk_start<<<\n"); */

	usb_transfer_complete(xfer);
	mutex_exit(&sc->sc_lock);

	return USBD_NORMAL_COMPLETION;
}

static void
ahci_device_bulk_abort(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Bab "));
	ahci_abort_xfer(xfer, USBD_CANCELLED);
}

static void
ahci_device_bulk_close(struct usbd_pipe *pipe)
{
	DPRINTF(D_TRACE, ("Bcl "));
}

static void
ahci_device_bulk_done(struct usbd_xfer *xfer)
{
	DPRINTF(D_TRACE, ("Bdn "));
}

#define DATA0_RD	(0x03)
#define DATA0_WR	(0x07)
#define AHCI_TIMEOUT	(5000)

/*
 * Do a transaction.
 * return 1 if ACK, 0 if NAK, -1 if error.
 */
static int
ahci_transaction(struct ahci_softc *sc, struct usbd_pipe *pipe,
	uint8_t pid, int len, u_char *buf, uint8_t toggle)
{
	return -1;
#if 0
#ifdef AHCI_DEBUG
	char str[64];
	int i;
#endif
	int timeout;
	int ls_via_hub = 0;
	int pl;
	uint8_t isr;
	uint8_t result = 0;
	uint8_t devaddr = pipe->up_dev->ud_addr;
	uint8_t endpointaddr = pipe->up_endpoint->ue_edesc->bEndpointAddress;
	uint8_t endpoint;
	uint8_t cmd = DATA0_RD;

	endpoint = UE_GET_ADDR(endpointaddr);
	DPRINTF(D_XFER, ("\n(%x,%d%s%d,%d) ",
		pid, len, (pid == SL11_PID_IN) ? "<-" : "->", devaddr, endpoint));

	/* Set registers */
	sl11write(sc, SL11_E0ADDR, 0x40);
	sl11write(sc, SL11_E0LEN,  len);
	sl11write(sc, SL11_E0PID,  (pid << 4) + endpoint);
	sl11write(sc, SL11_E0DEV,  devaddr);

	/* Set buffer unless PID_IN */
	if (pid != SL11_PID_IN) {
		if (len > 0)
			sl11write_region(sc, 0x40, buf, len);
		cmd = DATA0_WR;
	}

	/* timing ? */
	pl = (len >> 3) + 3;

	/* Low speed device via HUB */
	/* XXX does not work... */
	if ((sc->sc_fullspeed) && pipe->up_dev->ud_speed == USB_SPEED_LOW) {
		pl = len + 16;
		cmd |= SL11_EPCTRL_PREAMBLE;

		/*
		 * SL811HS/T rev 1.2 has a bug, when it got PID_IN
		 * from LowSpeed device via HUB.
		 */
		if (sc->sc_sltype == SLTYPE_SL811HS_R12 && pid == SL11_PID_IN) {
			ls_via_hub = 1;
			DPRINTF(D_MSG, ("LSvH "));
		}
	}

	/* timing ? */
	if (sl11read(sc, SL811_CSOF) <= (uint8_t)pl)
		cmd |= SL11_EPCTRL_SOF;

	/* Transfer */
	sl11write(sc, SL11_ISR, 0xff);
	sl11write(sc, SL11_E0CTRL, cmd | toggle);

	/* Polling */
	for (timeout = AHCI_TIMEOUT; timeout; timeout--) {
		isr = sl11read(sc, SL11_ISR);
		if ((isr & SL11_ISR_USBA))
			break;
	}

	/* Check result status */
	result = sl11read(sc, SL11_E0STAT);
	if (!(result & SL11_EPSTAT_NAK) && ls_via_hub) {
		/* Resend PID_IN within 20usec */
		sl11write(sc, SL11_ISR, 0xff);
		sl11write(sc, SL11_E0CTRL, SL11_EPCTRL_ARM);
	}

	sl11write(sc, SL11_ISR, 0xff);

	DPRINTF(D_XFER, ("t=%d i=%x ", AHCI_TIMEOUT - timeout, isr));
#if AHCI_DEBUG
	snprintb(str, sizeof(str),
	    "\20\x8STALL\7NAK\6OV\5SETUP\4DATA1\3TIMEOUT\2ERR\1ACK", result);
	DPRINTF(D_XFER, ("STAT=%s ", str));
#endif

	if ((result & SL11_EPSTAT_ERROR))
		return -1;

	if ((result & SL11_EPSTAT_NAK))
		return 0;

	/* Read buffer if PID_IN */
	if (pid == SL11_PID_IN && len > 0) {
		sl11read_region(sc, buf, 0x40, len);
#if AHCI_DEBUG
		for (i = 0; i < len; i++)
			DPRINTF(D_XFER, ("%02X ", buf[i]));
#endif
	}

	return 1;
#endif
}

void
ahci_abort_xfer(struct usbd_xfer *xfer, usbd_status status)
{
	xfer->ux_status = status;
	usb_transfer_complete(xfer);
}

void
ahci_device_clear_toggle(struct usbd_pipe *pipe)
{
	struct ahci_pipe *apipe = (struct ahci_pipe *)pipe;
	apipe->toggle = 0;
}

#ifdef AHCI_DEBUG
void
print_req(usb_device_request_t *r)
{
	const char *xmes[]={
		"GETSTAT",
		"CLRFEAT",
		"res",
		"SETFEAT",
		"res",
		"SETADDR",
		"GETDESC",
		"SETDESC",
		"GETCONF",
		"SETCONF",
		"GETIN/F",
		"SETIN/F",
		"SYNC_FR"
	};
	int req, type, value, index, len;

	req   = r->bRequest;
	type  = r->bmRequestType;
	value = UGETW(r->wValue);
	index = UGETW(r->wIndex);
	len   = UGETW(r->wLength);

	printf("%x,%s,v=%d,i=%d,l=%d ",
		type, xmes[req], value, index, len);
}

void
print_req_hub(usb_device_request_t *r)
{
	struct {
		int req;
		int type;
		const char *str;
	} conf[] = {
		{ 1, 0x20, "ClrHubFeat"  },
		{ 1, 0x23, "ClrPortFeat" },
		{ 2, 0xa3, "GetBusState" },
		{ 6, 0xa0, "GetHubDesc"  },
		{ 0, 0xa0, "GetHubStat"  },
		{ 0, 0xa3, "GetPortStat" },
		{ 7, 0x20, "SetHubDesc"  },
		{ 3, 0x20, "SetHubFeat"  },
		{ 3, 0x23, "SetPortFeat" },
		{-1, 0, NULL},
	};
	int i;
	int value, index, len;

	value = UGETW(r->wValue);
	index = UGETW(r->wIndex);
	len   = UGETW(r->wLength);
	for (i = 0; ; i++) {
		if (conf[i].req == -1 )
			return print_req(r);
		if (r->bmRequestType == conf[i].type && r->bRequest == conf[i].req) {
			printf("%s", conf[i].str);
			break;
		}
	}
	printf(",v=%d,i=%d,l=%d ", value, index, len);
}

void
print_dumpreg(struct ahci_softc *sc)
{
#if 0
	printf("00=%02x,01=%02x,02=%02x,03=%02x,04=%02x,"
	       "08=%02x,09=%02x,0A=%02x,0B=%02x,0C=%02x,",
		sl11read(sc, 0),  sl11read(sc, 1),
		sl11read(sc, 2),  sl11read(sc, 3),
		sl11read(sc, 4),  sl11read(sc, 8),
		sl11read(sc, 9),  sl11read(sc, 10),
		sl11read(sc, 11), sl11read(sc, 12)
	);
	printf("CR1=%02x,IER=%02x,0D=%02x,0E=%02x,0F=%02x ",
		sl11read(sc, 5), sl11read(sc, 6),
		sl11read(sc, 13), sl11read(sc, 14), sl11read(sc, 15)
	);
#endif
}

void
print_xfer(struct usbd_xfer *xfer)
{
	printf("xfer: length=%d, actlen=%d, flags=%x, timeout=%d,",
		xfer->ux_length, xfer->ux_actlen, xfer->ux_flags, xfer->ux_timeout);
	printf("request{ ");
	print_req_hub(&xfer->ux_request);
	printf("} ");
}
#endif /* AHCI_DEBUG */
