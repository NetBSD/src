/*	$NetBSD: octeon_usbc.c,v 1.1 2015/04/29 08:32:01 hikaru Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_usbc.c,v 1.1 2015/04/29 08:32:01 hikaru Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/kernel.h>
#include <sys/device.h>
#endif
#include <sys/rnd.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/cpu.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <mips/locore.h>
#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_corereg.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_usbnvar.h>
#include <mips/cavium/dev/octeon_usbcvar.h>
#include <mips/cavium/dev/octeon_usbcreg.h>
#include <mips/cavium/dev/octeon_gpioreg.h>
#include <mips/cavium/octeonvar.h>
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
#include <mips/cavium/octeon_model.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#if BYTE_ORDER == BIG_ENDIAN
#define htole32(x) (bswap32(x))
#define le32toh(x) (bswap32(x))
#else
#define htole32(x) (x)
#define le32toh(x) (x)
#endif
#endif

#ifdef USBC_DEBUG
int usbc_debug = 0;
#undef logprintf
#define logprintf(...) log(LOG_DEBUG, __VA_ARGS__)
/*#define USBC_SOF_DEBUG_PRINT |+ NOT recommend +|*/
#define DPRINTF(x)	do { if (usbc_debug) logprintf x; } while(0)
#define DPRINTFN(n,x)	do { if (usbc_debug>(n)) logprintf x; } while (0)
#define octeon_usbc_debug_reg_dump_N(n,x)\
	do { if (usbc_debug>(n)) octeon_usbc_debug_reg_dump x; } while (0)
#ifndef __NetBSD__
#define snprintb(b,l,f,q) snprintf((b), (l), "%b", (q), (f))
#endif
#define DCOUNTUP(v) do {\
	v++;\
	} while(0)
#define DCOUNTDOWN(v) do {\
	v--;\
	} while(0)
int usbc_pool_ed = 0;
int usbc_used_ed = 0;
int usbc_pool_td = 0;
int usbc_used_td = 0;
int usbc_pool_xfer = 0;
int usbc_used_xfer = 0;
int usbc_used_hc = 0;
int usbc_hc_cap = 0;
int usbc_skip_setnext = 0;
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
int usbc_recv_pkts = 0;
#endif
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#define DCOUNTUP(v)
#define DCOUNTDOWN(v)
#endif

#define USBC_CALC_OFFSET(base,n,d) ( (base) + ( (d) * (n) ) )
#define USBC_HPRT_WR1CMSK ~( USBC_HPRT_PRTOVRCURRCHNG | USBC_HPRT_PRTENCHNG \
	       	| USBC_HPRT_PRTENA | USBC_HPRT_PRTCONNDET )

#define USBC_HCCHARX_DELTA	(USBC_HCCHAR1_OFFSET - USBC_HCCHAR0_OFFSET)
#define USBC_HCSPLTX_DELTA	(USBC_HCSPLT1_OFFSET - USBC_HCSPLT0_OFFSET)
#define USBC_HCINTX_DELTA	(USBC_HCINT1_OFFSET - USBC_HCINT0_OFFSET)
#define USBC_HCINTMSKX_DELTA	(USBC_HCINTMSK1_OFFSET - USBC_HCINTMSK0_OFFSET)
#define USBC_HCTSIZX_DELTA	(USBC_HCTSIZ1_OFFSET - USBC_HCTSIZ0_OFFSET)

#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
#define USBC_NPTXDFIFOX_DELTA	(USBC_NPTXDFIFO1_OFFSET - USBC_NPTXDFIFO0_OFFSET)
#define USBC_NPTXDFIFOX_SIZE	(USBC_NPTXDFIFOX_DELTA)
#define USBC_SLAVE_MODE_NAK_INTERVAL (hz/5000)
#define USBC_SLAVE_MODE_NPTXEMP_INTERVAL (hz/10000)
#endif

#define USBC_DWC_HB_MULT(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))

#define USBC_INTR_ENDPT 1

#define USBC_RX_FIFO_SIZE 456
#define USBC_NPERIO_TX_FIFO_SIZE 912
#define USBC_PERIO_TX_FIFO_SIZE 456

#define USBC_IS_ABORTED_XFER(xfer)	((xfer)->status == USBD_CANCELLED || \
		(xfer)->status == USBD_TIMEOUT)

static octeon_usbc_soft_ed_t *octeon_usbc_alloc_soft_ed(octeon_usbc_softc_t *);
static void	octeon_usbc_free_soft_ed(octeon_usbc_softc_t *, octeon_usbc_soft_ed_t *);
static octeon_usbc_soft_td_t *octeon_usbc_alloc_soft_td(octeon_usbc_softc_t *);
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
static void	octeon_usbc_intr_nperio_txfifo_empty(octeon_usbc_softc_t *);
static void	octeon_usbc_intr_perio_txfifo_empty(octeon_usbc_softc_t *);
static void	octeon_usbc_intr_rxfifo_non_empty(octeon_usbc_softc_t *);
static void	octeon_usbc_intr_nptxfemp_enable(void *);
static void	octeon_usbc_intr_txfifo_empty_enable(octeon_usbc_softc_t *,
							octeon_usbc_hostch_t *);
static void	octeon_usbc_intr_do_nak_retry(void *);
static inline	usbd_status octeon_usbc_write_packet_into_host_channel(
				octeon_usbc_softc_t *, octeon_usbc_hostch_t *,
				u_int32_t, u_int32_t *);
static inline	usbd_status octeon_usbc_read_packet_from_host_channel(
				octeon_usbc_softc_t *, octeon_usbc_hostch_t *, u_int16_t);
#endif
static void	octeon_usbc_free_soft_td(octeon_usbc_softc_t *, octeon_usbc_soft_td_t *);
static octeon_usbc_soft_td_t * octeon_usbc_drop_soft_td(octeon_usbc_softc_t *,
				octeon_usbc_soft_td_t *, usbd_xfer_handle);

static void octeon_usbc_add_ed(octeon_usbc_soft_ed_t *, octeon_usbc_soft_ed_t *);
static void octeon_usbc_rem_ed(octeon_usbc_soft_ed_t *, octeon_usbc_soft_ed_t *);
static usbd_status	octeon_usbc_set_next_transact(octeon_usbc_softc_t *,
						octeon_usbc_soft_ed_t *);
static usbd_status	octeon_usbc_set_transact(octeon_usbc_softc_t *,
						octeon_usbc_hostch_t *);
void			octeon_usbc_abort_task(void *);
void			octeon_usbc_timeout_task(void *);
void			octeon_usbc_timeout(void *);

static usbd_status	octeon_usbc_init_core(octeon_usbc_softc_t *);
static void		octeon_usbc_reset_core(octeon_usbc_softc_t *);
static void		octeon_usbc_flush_fifo(octeon_usbc_softc_t *);
static inline void	octeon_usbc_wait_ahbidle(octeon_usbc_softc_t *);
static usbd_status	octeon_usbc_init_host(octeon_usbc_softc_t *);
static usbd_status	octeon_usbc_reinit_core(octeon_usbc_softc_t *);
static usbd_status	octeon_usbc_reinit_host(octeon_usbc_softc_t *);
static int		octeon_usbc_intr(void *);
static int		octeon_usbc_intr_common(octeon_usbc_softc_t *, u_int32_t);
static int		octeon_usbc_intr_host(octeon_usbc_softc_t *, u_int32_t);
static int		octeon_usbc_intr_host_sof(octeon_usbc_softc_t *);
static int		octeon_usbc_intr_host_port(octeon_usbc_softc_t *);
static void		octeon_usbc_intr_host_port_enable(void *);
static void		octeon_usbc_intr_host_channel(octeon_usbc_softc_t *);
static octeon_usbc_hostch_t* octeon_usbc_alloc_host_channel(octeon_usbc_softc_t *);
static inline int	octeon_usbc_setup_host_channel(octeon_usbc_softc_t *,
						octeon_usbc_hostch_t *);
static inline void	octeon_usbc_enable_host_channel(octeon_usbc_softc_t *,
							octeon_usbc_hostch_t *);
static int		octeon_usbc_halt_host_channel(octeon_usbc_softc_t *, octeon_usbc_hostch_t *);
static int		octeon_usbc_abort_host_channel(
				octeon_usbc_softc_t *, octeon_usbc_hostch_t *);
static int		octeon_usbc_free_host_channel(
				octeon_usbc_softc_t *, octeon_usbc_hostch_t *,
				usbd_status);


static usbd_status	octeon_usbc_open(usbd_pipe_handle);
static void		octeon_usbc_poll(struct usbd_bus *);
static void		octeon_usbc_softintr(void *);
static usbd_status	octeon_usbc_softintr_xfercompl(octeon_usbc_softc_t *,
							octeon_usbc_hostch_t *);
static inline int	octeon_usbc_calc_actlen_offset(int, int, u_int32_t);
static void		octeon_usbc_waitintr(octeon_usbc_softc_t *, usbd_xfer_handle);

static usbd_status	octeon_usbc_device_request(usbd_xfer_handle xfer);

static usbd_status	octeon_usbc_allocm(struct usbd_bus *, usb_dma_t *, u_int32_t);
static void		octeon_usbc_freem(struct usbd_bus *, usb_dma_t *);
static usbd_xfer_handle	octeon_usbc_allocx(struct usbd_bus *);
static void		octeon_usbc_freex(struct usbd_bus *, usbd_xfer_handle);

static usbd_status	octeon_usbc_root_ctrl_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_root_ctrl_start(usbd_xfer_handle);
static void		octeon_usbc_root_ctrl_abort(usbd_xfer_handle);
static void		octeon_usbc_root_ctrl_close(usbd_pipe_handle);
static void		octeon_usbc_root_ctrl_done(usbd_xfer_handle);

static usbd_status	octeon_usbc_root_intr_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_root_intr_start(usbd_xfer_handle);
static void		octeon_usbc_root_intr_abort(usbd_xfer_handle);
static void		octeon_usbc_root_intr_close(usbd_pipe_handle);
static void		octeon_usbc_root_intr_done(usbd_xfer_handle);

static usbd_status	octeon_usbc_device_ctrl_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_device_ctrl_start(usbd_xfer_handle);
static void		octeon_usbc_device_ctrl_abort(usbd_xfer_handle);
static void		octeon_usbc_device_ctrl_close(usbd_pipe_handle);
static void		octeon_usbc_device_ctrl_done(usbd_xfer_handle);

static usbd_status	octeon_usbc_device_intr_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_device_intr_start(usbd_xfer_handle);
static void		octeon_usbc_device_intr_abort(usbd_xfer_handle);
static void		octeon_usbc_device_intr_close(usbd_pipe_handle);
static void		octeon_usbc_device_intr_done(usbd_xfer_handle);

static usbd_status	octeon_usbc_device_bulk_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_device_bulk_start(usbd_xfer_handle);
static void		octeon_usbc_device_bulk_abort(usbd_xfer_handle);
static void		octeon_usbc_device_bulk_close(usbd_pipe_handle);
static void		octeon_usbc_device_bulk_done(usbd_xfer_handle);

static usbd_status	octeon_usbc_device_isoc_transfer(usbd_xfer_handle);
static usbd_status	octeon_usbc_device_isoc_start(usbd_xfer_handle);
static void		octeon_usbc_device_isoc_abort(usbd_xfer_handle);
static void		octeon_usbc_device_isoc_close(usbd_pipe_handle);
static void		octeon_usbc_device_isoc_done(usbd_xfer_handle);

static void		octeon_usbc_device_clear_toggle(usbd_pipe_handle);
static void		octeon_usbc_abort_xfer(usbd_xfer_handle, usbd_status);
static void		octeon_usbc_noop(usbd_pipe_handle);
static void		octeon_usbc_close_pipe(usbd_pipe_handle, octeon_usbc_soft_ed_t *);

static inline void	octeon_usbc_reg_assert(octeon_usbc_softc_t *, bus_size_t, u_int32_t);
static inline void	octeon_usbc_reg_assert_msked(octeon_usbc_softc_t *,
						bus_size_t, u_int32_t, u_int32_t);
static inline void	octeon_usbc_reg_deassert(octeon_usbc_softc_t *, bus_size_t, u_int32_t);
static inline void	octeon_usbc_reg_deassert_msked(octeon_usbc_softc_t *,
						bus_size_t, u_int32_t, u_int32_t);
static inline u_int32_t	octeon_usbc_reg_rd(octeon_usbc_softc_t *, bus_size_t);
static inline void	octeon_usbc_reg_wr(octeon_usbc_softc_t *, bus_size_t, u_int32_t);


#ifdef USBC_DEBUG
struct octeon_usbc_reg;
void			octeon_usbc_debug_reg_dump(octeon_usbc_softc_t *, bus_size_t);
void			octeon_usbc_debug_dumpregs(octeon_usbc_softc_t *);
void			octeon_usbc_debug_dumpreg(octeon_usbc_softc_t *, const struct octeon_usbc_reg *);
#endif

struct usbd_bus_methods octeon_usbc_bus_methods = {
	octeon_usbc_open,
	octeon_usbc_softintr,
	octeon_usbc_poll,
	octeon_usbc_allocm,
	octeon_usbc_freem,
	octeon_usbc_allocx,
	octeon_usbc_freex,
};

struct usbd_pipe_methods octeon_usbc_root_ctrl_methods = {
	octeon_usbc_root_ctrl_transfer,
	octeon_usbc_root_ctrl_start,
	octeon_usbc_root_ctrl_abort,
	octeon_usbc_root_ctrl_close,
	octeon_usbc_noop,
	octeon_usbc_root_ctrl_done,
};

struct usbd_pipe_methods octeon_usbc_root_intr_methods = {
	octeon_usbc_root_intr_transfer,
	octeon_usbc_root_intr_start,
	octeon_usbc_root_intr_abort,
	octeon_usbc_root_intr_close,
	octeon_usbc_noop,
	octeon_usbc_root_intr_done,
};


struct usbd_pipe_methods octeon_usbc_device_ctrl_methods = {
	octeon_usbc_device_ctrl_transfer,
	octeon_usbc_device_ctrl_start,
	octeon_usbc_device_ctrl_abort,
	octeon_usbc_device_ctrl_close,
	octeon_usbc_noop,
	octeon_usbc_device_ctrl_done,
};

struct usbd_pipe_methods octeon_usbc_device_intr_methods = {
	octeon_usbc_device_intr_transfer,
	octeon_usbc_device_intr_start,
	octeon_usbc_device_intr_abort,
	octeon_usbc_device_intr_close,
	octeon_usbc_device_clear_toggle,
	octeon_usbc_device_intr_done,
};

struct usbd_pipe_methods octeon_usbc_device_bulk_methods = {
	octeon_usbc_device_bulk_transfer,
	octeon_usbc_device_bulk_start,
	octeon_usbc_device_bulk_abort,
	octeon_usbc_device_bulk_close,
	octeon_usbc_device_clear_toggle,
	octeon_usbc_device_bulk_done,
};

struct usbd_pipe_methods octeon_usbc_device_isoc_methods = {
	octeon_usbc_device_isoc_transfer,
	octeon_usbc_device_isoc_start,
	octeon_usbc_device_isoc_abort,
	octeon_usbc_device_isoc_close,
	octeon_usbc_noop,
	octeon_usbc_device_isoc_done,
};


usbd_status
octeon_usbc_init(octeon_usbc_softc_t *sc)
{
	usbd_status err;
	u_int32_t ver;
	int status;

	DPRINTFN(5,("octeon_usbc_init: start\n"));

	ver = octeon_usbc_reg_rd(sc, USBC_GSNPSID_OFFSET);
	aprint_normal("%s: Core Release number %x\n", device_xname(sc->sc_dev),
	    ver);
	sc->sc_noport = 1;
	sc->sc_dma_enable = 0; /* autodetect while initialization */
	sc->sc_bus.usbrev = USBREV_2_0;
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
	if (MIPS_PRID_IMPL(mips_options.mips_cpu_id) == MIPS_CN31XX)
		aprint_normal("%s: enable CN31xx USBN DMA workaround\n",
				device_xname(sc->sc_dev));
		sc->sc_workaround_force_slave_mode = 1;
	} else {
		sc->sc_workaround_force_slave_mode = 0;
	}
#endif

	SIMPLEQ_INIT(&sc->sc_free_xfers);

	/* XXX bus_space window should be passed from the parent? */
#if 0
	status = bus_space_map(sc->sc_bust, USBC_BASE, USBC_SIZE,
	    0, &sc->sc_regh);
	if (status != 0)
		panic(": can't map i/o space");
#endif

	status = usb_setup_reserve(sc->sc_dev, &sc->sc_dma_reserve, sc->sc_bus.dmatag,
	    USB_MEM_RESERVE);
	if (status != 0)
		panic(": can't reserve dma");

	/* Allocate dummy ED that starts the control list. */
	sc->sc_ctrl_head = octeon_usbc_alloc_soft_ed(sc);
	if (sc->sc_ctrl_head == NULL) {
		err = USBD_NOMEM;
		goto bad1;
	}
	sc->sc_ctrl_head->status = USBC_ENDPT_STATUS_SKIP;

	/* Allocate dummy ED that starts the bulk list. */
	sc->sc_bulk_head = octeon_usbc_alloc_soft_ed(sc);
	if (sc->sc_bulk_head == NULL) {
		err = USBD_NOMEM;
		goto bad2;
	}
	sc->sc_bulk_head->status = USBC_ENDPT_STATUS_SKIP;

	/* Allocate dummy ED that starts the intr list. */
	sc->sc_intr_head = octeon_usbc_alloc_soft_ed(sc);
	if (sc->sc_intr_head == NULL) {
		err = USBD_NOMEM;
		goto bad3;
	}
	sc->sc_intr_head->status = USBC_ENDPT_STATUS_SKIP;

#if 0
	/* Allocate dummy ED that starts the isochronous list. */
	sc->sc_isoc_head = octeon_usbc_alloc_soft_ed(sc);
	if (sc->sc_isoc_head == NULL) {
		err = USBD_NOMEM;
		goto bad4;
	}
	sc->sc_isoc_head->status = USBC_ENDPT_STATUS_SKIP;
#else
	sc->sc_isoc_head = NULL;
#endif

#ifdef USBC_DEBUG
	/* show hardware configuration */
	if (usbc_debug > 15)
		octeon_usbc_debug_dumpregs(sc);
#endif

	/* Turn off global interrupt and port power while initializing */
	octeon_usbc_reg_deassert(sc, USBC_GAHBCFG_OFFSET, USBC_GAHBCFG_GLBLINTRMSK);
	octeon_usbc_reg_deassert_msked(sc,USBC_HPRT_OFFSET,
			USBC_HPRT_PRTPWR, USBC_HPRT_WR1CMSK);

	DPRINTFN(5,("octeon_usbc_init: core initialization\n"));
	octeon_usbc_init_core(sc);

	DPRINTFN(5,("octeon_usbc_init: interrupt establish\n"));
	/* XXX IPL_USB? */
	sc->sc_ih = octeon_intr_establish(ffs64(CIU_INTX_SUM0_USB) -1,
	    0, IPL_BIO, octeon_usbc_intr, sc);
	if (sc->sc_ih == NULL)
		panic("%s: can't establish common interrupt\n",
			device_xname(sc->sc_dev));

	callout_init(&sc->sc_tmo_hprt, 0);
#if OCTEON_USBN_CN31XX_DMA_WORKAROUND
	if (!sc->sc_dma_enable)
		callout_init(&sc->sc_tmo_nptxfemp, 0);
#endif

	    
	if (octeon_usbc_reg_rd(sc, USBC_GINTSTS_OFFSET) &
	    USBC_GINTSTS_CURMOD)
	{
		DPRINTFN(5,("octeon_usbc_init: host initialization\n"));
		/* host mode */	
		octeon_usbc_init_host(sc);
	}
	else
	{
		/* device mode */
		panic("device mode is not supported");
	}

	/* 
	 * Finaly, turn on global interrupt.
	 * But power is still off, because it will be turned on by uhub.
	 */
	octeon_usbc_reg_assert(sc, USBC_GAHBCFG_OFFSET, USBC_GAHBCFG_GLBLINTRMSK);


	return USBD_NORMAL_COMPLETION;

 bad3:
	octeon_usbc_free_soft_ed(sc, sc->sc_bulk_head);
 bad2:
	octeon_usbc_free_soft_ed(sc, sc->sc_ctrl_head);
 bad1:
/*        usb_freemem(&sc->sc_bus, &sc->sc_hccadma);*/
	return (err);
}

static void
octeon_usbc_reset_core(octeon_usbc_softc_t *sc)
{
	/* Reset core */
	int i = 0;
	u_int32_t grstctl = 0;
	u_int32_t grstctl_csftrst;

	octeon_usbc_wait_ahbidle(sc);
	
	i = 0;
	DPRINTFN(2,("octeon_usbc_reset_core: soft reset core\n"));
	grstctl |= USBC_GRSTCTL_CSFTRST;
	octeon_usbc_reg_wr(sc, USBC_GRSTCTL_OFFSET, grstctl);
	for (;;) {
		usb_delay_ms(&sc->sc_bus, 1);
		grstctl = octeon_usbc_reg_rd(sc, USBC_GRSTCTL_OFFSET);
		grstctl_csftrst = grstctl & USBC_GRSTCTL_CSFTRST;

		if (!grstctl_csftrst)
			break;
		if (++i > 10000) {
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_GRSTCTL_OFFSET);
#endif /* USBC_DEBUG */
			panic("Hangup when Soft Reset Core.\n");
		}
	}
	usb_delay_ms(&sc->sc_bus, 100); /* 100 msec wait for 3 PHY clocks */

	DPRINTFN(2,("octeon_usbc_reset_core: reseted.\n"));

	return;
}

static void
octeon_usbc_flush_fifo(octeon_usbc_softc_t *sc)
{
	u_int32_t grstctl = 0;
	u_int32_t grstctl_txfflsh = 0;
	u_int32_t grstctl_rxfflsh = 0;
	int i;

	grstctl |= 0x10 << USBC_GRSTCTL_TXFNUM_OFFSET;
	grstctl |= USBC_GRSTCTL_TXFFLSH;
	grstctl |= USBC_GRSTCTL_RXFFLSH;

	DPRINTFN(2,("octeon_usbc_flush_fifo: Flush FIFO\n"));
	octeon_usbc_reg_wr(sc, USBC_GRSTCTL_OFFSET, grstctl);
	for (i=0;;) {
		grstctl = octeon_usbc_reg_rd(sc, USBC_GRSTCTL_OFFSET);
		grstctl_txfflsh = grstctl & USBC_GRSTCTL_TXFFLSH;
		grstctl_rxfflsh = grstctl & USBC_GRSTCTL_RXFFLSH;

		if (!grstctl_txfflsh && !grstctl_rxfflsh)
			break;
		if (++i > 10000) {
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_GRSTCTL_OFFSET);
#endif /* USBC_DEBUG */
			panic("Hangup when Flushing FIFO.\n");
		}
	}
}

static inline void
octeon_usbc_wait_ahbidle(octeon_usbc_softc_t *sc)
{
	int i = 0;
	u_int32_t grstctl = 0;
	u_int32_t grstctl_ahbidle = 0;

	DPRINTFN(2,("%s: waiting for idel state\n", __func__));
	for (;;) {
		usb_delay_ms(&sc->sc_bus, 1);
		grstctl = octeon_usbc_reg_rd(sc, USBC_GRSTCTL_OFFSET);
		grstctl_ahbidle = grstctl & USBC_GRSTCTL_AHBIDLE;

		if (grstctl_ahbidle)
			break;
		if (++i > 100000) {
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_GRSTCTL_OFFSET);
#endif /* USBC_DEBUG */
			panic("Hangup when wait for AHB master IDLE state.\n");
		}
	}
}

static usbd_status
octeon_usbc_init_core(octeon_usbc_softc_t *sc)
{
	u_int32_t ahbcfg, usbcfg, gintmsk;
	u_int32_t hwcfg2;
#if 0
	u_int32_, hwcfg3, hwcfg4;
#endif
	u_int32_t hcfg = 0;

#if 0	/* NOT need, because we dont have ULPI PHY, but UTMI+ PHY */
	/* TODO: setup ULPI External VBUS, TS Dline conf into GUSBCFG ?? */

	/* TODO: and reset core ?? */
#endif


	/* read H/W configurations */
	hwcfg2 = octeon_usbc_reg_rd(sc, USBC_GHWCFG2_OFFSET);
#if 0
	hwcfg3 = octeon_usbc_reg_rd(sc, USBC_GHWCFG3_OFFSET);
	hwcfg4 = octeon_usbc_reg_rd(sc, USBC_GHWCFG4_OFFSET);
#endif

	sc->sc_hwcfg.numhstch
		= ((hwcfg2 & USBC_GHWCFG2_NUMHSTCHN1)
				>> USBC_GHWCFG2_NUMHSTCHN1_OFFSET) + 1;
#ifdef  USBC_DEBUG
	usbc_hc_cap = sc->sc_hwcfg.numhstch;
#endif

	sc->sc_hwcfg.nptxqdepth
		= (((hwcfg2 & USBC_GHWCFG2_NPTXQDEPTH)
				>> USBC_GHWCFG2_NPTXQDEPTH_OFFSET) << 2);
	if (!sc->sc_hwcfg.nptxqdepth)
		sc->sc_hwcfg.nptxqdepth = 2;

	sc->sc_hwcfg.ptxqdepth
		= (((hwcfg2 & USBC_GHWCFG2_PTXQDEPTH)
				>> USBC_GHWCFG2_PTXQDEPTH_OFFSET) << 2);
	if (!sc->sc_hwcfg.ptxqdepth)
		sc->sc_hwcfg.ptxqdepth = 2;

#ifdef USBC_USE_DEV_MODE
	sc->sc_hwcfg.num_dev_eps = (hwcfg2 & USBC_GHWCFG2_NUMDEVEPS)
			>> USBC_GHWCFG2_NUMDEVEPS_OFFSET;
	sc->sc_hwcfg.num_dev_perio_eps = (hwcfg4 & USBC_GHWCFG4_NUMDEVPERIOEPS)
			>> USBC_GHWCFG4_NUMDEVPERIOEPS_OFFSET;
#endif
	sc->sc_hwcfg.ptxqdepth = (hwcfg2 & USBC_GHWCFG2_PTXQDEPTH)
			>> USBC_GHWCFG2_PTXQDEPTH_OFFSET;


	hcfg = 0;
/*        hcfg |= USBC_HCFG_FSLSPCLKSEL_30_60_MHZ << USBC_HCFG_FSLSPCLKSEL_OFFSET;*/
/*        hcfg |= USBC_HCFG_FSLSPCLKSEL_48_MHZ << USBC_HCFG_FSLSPCLKSEL_OFFSET;*/
	hcfg |= USBC_HCFG_FSLSPCLKSEL_30_60_MHZ;
/*        hcfg |= USBC_HCFG_FSLSPCLKSEL_48_MHZ;*/
	octeon_usbc_reg_wr(sc, USBC_HCFG_OFFSET, hcfg);
	octeon_usbc_reset_core(sc);

	DPRINTFN(5,("octeon_usbc_init_core: initialize fifo size\n"));
	sc->sc_rx_fifo_size = USBC_RX_FIFO_SIZE;
	sc->sc_nperio_tx_fifo_siz = USBC_NPERIO_TX_FIFO_SIZE;
	sc->sc_perio_tx_fifo_siz = USBC_PERIO_TX_FIFO_SIZE;

	DPRINTF(("octeon_usbc_init_core:   rx_fifo_size=0x%x\n",
		sc->sc_rx_fifo_size));
	DPRINTF(("octeon_usbc_init_core:   nperio_tx_fifo_size=0x%x\n",
		sc->sc_nperio_tx_fifo_siz));
	DPRINTF(("octeon_usbc_init_core:   perio_tx_fifo_size=0x%x\n",
		sc->sc_perio_tx_fifo_siz));

	octeon_usbc_reg_wr(sc, USBC_GRXFSIZ_OFFSET, sc->sc_rx_fifo_size);
	octeon_usbc_reg_wr(sc, USBC_GNPTXFSIZ_OFFSET,
			sc->sc_nperio_tx_fifo_siz << USBC_GNPTXFSIZ_NPTXFEDP_OFFSET
			| sc->sc_rx_fifo_size );
	octeon_usbc_reg_wr(sc, USBC_HPTXFSIZ_OFFSET,
			sc->sc_perio_tx_fifo_siz << USBC_HPTXFSIZ_PTXFSTADDR_OFFSET
			| (sc->sc_rx_fifo_size + sc->sc_nperio_tx_fifo_siz) );
	octeon_usbc_flush_fifo(sc);

	/* init GAHBCFG */
	ahbcfg = octeon_usbc_reg_rd(sc, USBC_GAHBCFG_OFFSET);
	switch (((hwcfg2 & USBC_GHWCFG2_OTGARCH) >> USBC_GHWCFG2_OTGARCH_OFFSET)) {
	case USBC_GHWCFG2_OTGARCH_SLAVEONLY:
		DPRINTFN(2,("octeon_usbc_init_core: Slave Only Mode detected\n"));
		sc->sc_dma_enable = 0;
		break;
	case USBC_GHWCFG2_OTGARCH_INTERNALDMA:
		DPRINTFN(2,("octeon_usbc_init_core: Internal DMA Mode detected\n"));
		panic(": Internal DMA is not supported");
		break;
	case USBC_GHWCFG2_OTGARCH_EXTERNALDMA:
		/* I expect to choose this case ...*/
		DPRINTFN(2,("octeon_usbc_init_core: External DMA Mode detected\n"));
		sc->sc_dma_enable = 1;
		break;
	default:
		panic(": can't detecte OTGARCH.");
	}

#if OCTEON_USBN_CN31XX_DMA_WORKAROUND
	/* force using slave mode */
	if (sc->sc_workaround_force_slave_mode) {
		DPRINTFN(2,("octeon_usbc_init_core: force Slave Mode\n"));
		sc->sc_dma_enable = 0;
	}
#endif
	if (sc->sc_dma_enable) {
		/* DMA mode */
		ahbcfg |= USBC_DMA_BURST_SIZE << USBC_GAHBCFG_HBSTLEN_OFFSET; /* XXX */
		ahbcfg |= USBC_GAHBCFG_DMAEN;
		ahbcfg &= ~(USBC_GAHBCFG_NPTXFEMPLVL | USBC_GAHBCFG_PTXFEMPLVL);
/*                ahbcfg &= ~USBC_GAHBCFG_HBSTLEN; |+ XXX +|*/
	} else {
		ahbcfg &= ~USBC_GAHBCFG_DMAEN;
		/* the periodic/nonperiodic TxFIFO is half empty */
		ahbcfg |= USBC_GAHBCFG_PTXFEMPLVL | USBC_GAHBCFG_NPTXFEMPLVL;
	}
	octeon_usbc_reg_wr(sc, USBC_GAHBCFG_OFFSET, ahbcfg);
	
	/* init GUSBCFG */
	usbcfg = octeon_usbc_reg_rd(sc, USBC_GUSBCFG_OFFSET);
	usbcfg &= (usbcfg & (~USBC_GUSBCFG_TOUTCAL)) |
	    (0 << USBC_GUSBCFG_TOUTCAL_OFFSET);
	usbcfg &= ~USBC_GUSBCFG_DDRSEL;
	usbcfg &= (usbcfg & (~USBC_GUSBCFG_USBTRDTIM)) |
	    (0x5U << USBC_GUSBCFG_USBTRDTIM_OFFSET);
	usbcfg &= ~USBC_GUSBCFG_PHYLPWRCLKSEL;
	usbcfg &= ~USBC_GUSBCFG_PHYSEL;
	usbcfg &= ~USBC_GUSBCFG_ULPI_UTMI_SEL;
/*        usbcfg |= USBC_GUSBCFG_PHYIF; |+ XXX RO register ??? +|*/
	usbcfg &= ~(USBC_GUSBCFG_XXX_31_17 |  USBC_GUSBCFG_XXX_14);
	octeon_usbc_reg_wr(sc, USBC_GUSBCFG_OFFSET, usbcfg);


	gintmsk = 0;
	gintmsk |= 
		USBC_GINTMSK_OTGINTMSK |
		USBC_GINTMSK_MODEMISMSK |
		USBC_GINTMSK_CONIDSTSTCHNGMSK |
		USBC_GINTMSK_WKUPINTMSK |
		USBC_GINTMSK_DISCONNINTMSK |
		USBC_GINTMSK_USBSUSPMSK |
		USBC_GINTMSK_SESSREQINTMSK;
	if (!sc->sc_dma_enable) {
		gintmsk |= USBC_GINTMSK_RXFLVLMSK;
	}
	octeon_usbc_reg_wr(sc, USBC_GINTMSK_OFFSET, gintmsk);

	return USBD_NORMAL_COMPLETION;
}
static usbd_status
octeon_usbc_reinit_core(octeon_usbc_softc_t *sc)
{
	octeon_usbc_reg_deassert_msked(sc,USBC_HPRT_OFFSET,
			USBC_HPRT_PRTPWR, USBC_HPRT_WR1CMSK);

	octeon_intr_disestablish(sc->sc_ih);

	octeon_usbc_init_core(sc);

	sc->sc_ih = octeon_intr_establish(ffs64(CIU_INTX_SUM0_USB) -1,
	    0, IPL_BIO, octeon_usbc_intr, sc);
	if (sc->sc_ih == NULL)
		panic("%s: can't establish common interrupt\n",
			device_xname(sc->sc_dev));

	octeon_usbc_reinit_host(sc);

	octeon_usbc_reg_assert(sc, USBC_GAHBCFG_OFFSET, USBC_GAHBCFG_GLBLINTRMSK);
	octeon_usbc_reg_assert_msked(sc,USBC_HPRT_OFFSET,
			USBC_HPRT_PRTPWR, USBC_HPRT_WR1CMSK);
	sc->sc_connstat_change = 0;
	sc->sc_isreset = 0;
	sc->sc_isprtbbl = 0;
	log(LOG_CRIT, "%s: core reseted.\n", device_xname(sc->sc_dev));
	return USBD_NORMAL_COMPLETION;
}

inline static usbd_status
octeon_usbc_init_host_subr(octeon_usbc_softc_t *sc)
{
	u_int32_t gintmsk=0;
	u_int32_t i;

	for (i=0; i < sc->sc_hwcfg.numhstch; i++){
		sc->sc_hostch_list[i].hc_id = 0x1 << i;
		callout_init(&sc->sc_hostch_list[i].tmo_retry, 0);
		sc->sc_hostch_list[i].sc = sc;
		sc->sc_hostch_list[i].std_done_head = NULL;
		sc->sc_hostch_list[i].offset_usbc_hcchar =
			USBC_CALC_OFFSET(USBC_HCCHAR0_OFFSET, i, USBC_HCCHARX_DELTA);
		sc->sc_hostch_list[i].offset_usbc_hcsplt =
			USBC_CALC_OFFSET(USBC_HCSPLT0_OFFSET, i, USBC_HCSPLTX_DELTA);
		sc->sc_hostch_list[i].offset_usbc_hcint =
			USBC_CALC_OFFSET(USBC_HCINT0_OFFSET, i, USBC_HCINTX_DELTA);
		sc->sc_hostch_list[i].offset_usbc_hcintmsk =
			USBC_CALC_OFFSET(USBC_HCINTMSK0_OFFSET, i, USBC_HCINTMSKX_DELTA);
		sc->sc_hostch_list[i].offset_usbc_hctsiz =
			USBC_CALC_OFFSET(USBC_HCTSIZ0_OFFSET, i, USBC_HCTSIZX_DELTA);
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
		sc->sc_hostch_list[i].hc_num = i;
		sc->sc_hostch_list[i].offset_usbc_dfifo =
			USBC_CALC_OFFSET(USBC_NPTXDFIFO0_OFFSET, i, USBC_NPTXDFIFOX_DELTA);
#endif
	}

/*        octeon_usbc_reg_deassert(sc, USBC_HCFG_OFFSET,*/
/*            USBC_HCFG_FSLSSUPP | USBC_HCFG_FSLSPCLKSEL);*/

	sc->sc_bus.methods = &octeon_usbc_bus_methods;
	sc->sc_bus.pipe_size = sizeof(struct octeon_usbc_pipe);

/*        octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET, USBC_GINTMSK_HCHINTMSK);*/
/*                USBC_GINTMSK_HCHINTMSK | USBC_GINTMSK_RXFLVLMSK);*/
	gintmsk = octeon_usbc_reg_rd(sc, USBC_GINTMSK_OFFSET);
	gintmsk |=
		USBC_GINTMSK_PRTINTMSK |
		USBC_GINTMSK_SOFMSK |
		USBC_GINTMSK_HCHINTMSK;
	gintmsk &= ~( USBC_GINTMSK_PTXFEMPMSK |
			USBC_GINTMSK_NPTXFEMPMSK);
	octeon_usbc_reg_wr(sc, USBC_GINTMSK_OFFSET, gintmsk);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
octeon_usbc_init_host(octeon_usbc_softc_t *sc)
{
	DPRINTF(("octeon_usbc_init_core: host channel num=%d\n",
		sc->sc_hwcfg.numhstch)); /* maybe... we have 8 channels */

	sc->sc_hostch_list =
		(octeon_usbc_hostch_t *)malloc(sc->sc_hwcfg.numhstch * 
			sizeof(octeon_usbc_hostch_t), M_USB, M_NOWAIT);
	if (sc->sc_hostch_list != NULL)
		memset(sc->sc_hostch_list, 0,
			sc->sc_hwcfg.numhstch * sizeof(octeon_usbc_hostch_t));
	else 
		return USBD_NOMEM;

	octeon_usbc_init_host_subr(sc);

	return USBD_NORMAL_COMPLETION;
}
static usbd_status
octeon_usbc_reinit_host(octeon_usbc_softc_t *sc)
{
	if (sc->sc_hostch_list != NULL)
		memset(sc->sc_hostch_list, 0,
			sc->sc_hwcfg.numhstch * sizeof(octeon_usbc_hostch_t));

	octeon_usbc_init_host_subr(sc);

	return USBD_NORMAL_COMPLETION;
}

static int
octeon_usbc_intr(void *arg)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *) arg;

	int result = 0;
	u_int32_t intsts;

	sc->sc_bus.no_intrs++;

#ifdef USBC_DEBUG
#ifdef USBC_SOF_DEBUG_PRINT
	DPRINTFN(15,("\nocteon_usbc_intr entered\n"));
	if (usbc_debug > 15) {
		octeon_usbc_debug_reg_dump(sc, USBC_GINTSTS_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GINTMSK_OFFSET);
	}
#endif
#endif


#if 0 && defined(USBC_DEBUG)
	do {
		u_int32_t gptxsts = octeon_usbc_reg_rd(sc, USBC_GNPTXSTS_OFFSET);
		const int que_space_avail = (gptxsts & USBC_GNPTXSTS_NPTXQSPCAVAIL)
						>> USBC_GNPTXSTS_NPTXQSPCAVAIL_OFFSET; 
		const int fifo_space_avail = (gptxsts & USBC_GNPTXSTS_NPTXFSPCAVAIL)
						>> USBC_GNPTXSTS_NPTXFSPCAVAIL_OFFSET; 
		if(que_space_avail < 0x8 || fifo_space_avail < sc->sc_nperio_tx_fifo_siz){
			DPRINTF(("octeon_usbc_intr: working nptx queue: "
						"avail_que=0x%x, avail_fifo=0x%x\n",
						que_space_avail, fifo_space_avail));
		}
	} while(0);
#endif

	intsts = octeon_usbc_reg_rd(sc, USBC_GINTSTS_OFFSET);
	intsts &= octeon_usbc_reg_rd(sc, USBC_GINTMSK_OFFSET)
		| USBC_GINTSTS_CURMOD;
	/* clear interrupt status */
	octeon_usbc_reg_assert(sc, USBC_GINTSTS_OFFSET, intsts);

	result |= octeon_usbc_intr_common(sc, intsts);

	if (intsts & USBC_GINTSTS_CURMOD)
	{
#ifdef USBC_DEBUG
#ifdef USBC_SOF_DEBUG_PRINT
		DPRINTFN(15,("octeon_usbc_inir: host interrupt\n"));
		/* host mode */	
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET, intsts);
#endif
#endif
		result |= octeon_usbc_intr_host(sc, intsts);
	}
	else
	{
		/* device mode */
		panic("device mode is not supported");
	}

	return result;
}

static int
octeon_usbc_intr_common(octeon_usbc_softc_t *sc, u_int32_t intsts)
{
	int result = 0;

	if (intsts & USBC_GINTSTS_MODEMIS) {
		log(LOG_CRIT, "octeon_usbc_intr: mode is mismatch\n");
		/* No implementation */
		/* Clear intr */
	}
	if (intsts & USBC_GINTSTS_OTGINT) {
		/* OTG interrupt */
		/* No implementation */
		log(LOG_CRIT, "octeon_usbc_intr: OTG is not supported\n");
	}
	if (intsts & USBC_GINTSTS_CONIDSTSCHNG) {
		/* Connector ID status Change */
		/* No implementation */
#if 0
		result = octeon_usbc_intr_connector_id_change(sc); 
#endif
	}
	if (intsts & USBC_GINTSTS_DISCONNINT) {

		/* Disconnect detected interrupt */
		log(LOG_DEBUG, "%s: disconnect device\n", device_xname(sc->sc_dev));
		sc->sc_connstat_change |= USBC_PRT_STAT_DISCONNECTED;
		/* clear intr */
		octeon_usbc_reg_assert(sc, USBC_GINTSTS_OFFSET,
			USBC_GINTSTS_DISCONNINT);
#if 0
		/* reset the port */
		octeon_usbc_reg_assert(sc, USBC_HPRT_OFFSET,
			USBC_HPRT_PRTENA);
#endif
	}
	if (intsts & USBC_GINTSTS_SESSREQINT) {
		/* clear intr */
		octeon_usbc_reg_assert(sc, USBC_GINTSTS_OFFSET,
			USBC_GINTSTS_SESSREQINT);
	}
	return result;
}

static int
octeon_usbc_intr_host(octeon_usbc_softc_t *sc, u_int32_t intsts)
{
	usbd_status result = 0;

	if (intsts == 0) {
		return 0;
	}
#ifdef USBC_DEBUG
	if (usbc_debug > 15 && intsts & (USBC_GINTSTS_PRTINT | USBC_GINTSTS_HCHINT)){
		DPRINTFN(15,("octeon_usbc_intr_host: enter\n"));
		octeon_usbc_debug_reg_dump(sc, USBC_GAHBCFG_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GINTSTS_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GINTMSK_OFFSET);
	}
#endif
	if (intsts & USBC_GINTSTS_SOF) {
		/* start of (micro)frame */
		result |= octeon_usbc_intr_host_sof(sc);
		/* clear intr */
	}
	if (intsts & USBC_GINTSTS_PRTINT) {
		/* host port interrupt */
		result |= octeon_usbc_intr_host_port(sc);
		/* clear intr */

		/* do not allow host port interrupts > 1 per second */
		octeon_usbc_reg_deassert(sc, USBC_GINTMSK_OFFSET, USBC_GINTMSK_PRTINTMSK);

                callout_reset(&sc->sc_tmo_hprt, hz,
				octeon_usbc_intr_host_port_enable, sc);
	}
	if (intsts & USBC_GINTSTS_RXFLVL) {
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
		if (!sc->sc_dma_enable) {
			octeon_usbc_reg_deassert(sc, USBC_GINTMSK_OFFSET,
					USBC_GINTMSK_RXFLVLMSK);
			octeon_usbc_intr_rxfifo_non_empty(sc);
			octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET,
					USBC_GINTMSK_RXFLVLMSK);
		}
#endif

	}
	if (intsts & USBC_GINTSTS_PTXFEMP) {
		/* the periodic tx FiFO is half-empty */
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
		if (!sc->sc_dma_enable)
			octeon_usbc_intr_perio_txfifo_empty(sc);
#endif
	}
	if (intsts & USBC_GINTSTS_NPTXFEMP) {
		/* the non-periodic tx FIFO is half-empty */
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
		if (!sc->sc_dma_enable)
			octeon_usbc_intr_nperio_txfifo_empty(sc);
#endif
	}
	if (intsts & USBC_GINTSTS_HCHINT) {
		/* host channel interrupt */
		octeon_usbc_intr_host_channel(sc);
		/* clear intr */
		if (sc->sc_haint)
			usb_schedsoftintr(&sc->sc_bus);

	}
#if 0
	if (intsts & USBC_GINTSTS_I2CINT) {
		/* i2c interrupt */
		/* h/w manual say "this bit is always 0." */
	}
#endif 

	return USBD_NORMAL_COMPLETION;
}

static int
octeon_usbc_intr_host_sof(octeon_usbc_softc_t *sc)
{
	octeon_usbc_soft_ed_t *sed;
	u_int32_t hfnum = octeon_usbc_reg_rd(sc, USBC_HFNUM_OFFSET);
	u_int8_t next_is_odd = (hfnum & 0x1) ? 0 : 1;
	sc->sc_frame_number = hfnum & USBC_HFNUM_FRNUM;

#ifdef USBC_DEBUG
	if ( usbc_debug > 18 && sc->sc_frame_number == 0x3FFF )
		DPRINTF(("octeon_usbc_intr_host_sof: reset frame number\n"));
#endif
	for (sed = sc->sc_intr_head; sed; sed = sed->next){
		if(sed->status == USBC_ENDPT_STATUS_WAIT_NEXT_SOF){
			if(--sed->interval_count < 1) {
				sed->status = USBC_ENDPT_STATUS_IN_PROGRESS;
				if (next_is_odd)
					octeon_usbc_reg_assert(sc,
							sed->hostch->offset_usbc_hcchar,
							USBC_HCCHARX_ODDFRM);
				else
					octeon_usbc_reg_deassert(sc,
							sed->hostch->offset_usbc_hcchar,
							USBC_HCCHARX_ODDFRM);
					
				octeon_usbc_enable_host_channel(sc, sed->hostch);
			}
		}
	}

	return USBD_NORMAL_COMPLETION;
}

static int
octeon_usbc_intr_host_port(octeon_usbc_softc_t *sc)
{
	u_int32_t hfnum	= 0;
	u_int32_t hfir	= 0;
	u_int32_t frint	= 0;

	u_char *p;
	u_int32_t hprt = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
#if 1
	usbd_xfer_handle xfer = sc->sc_intrxfer;

	if (xfer == NULL) {
		/* ignore and clear intr */
/*                octeon_usbc_reg_wr(sc, USBC_HPRT_OFFSET, hprt);*/
		return USBD_NORMAL_COMPLETION;
	}
#endif

#ifdef USBC_DEBUG
	DPRINTFN(15,("octeon_usbc_intr_host_port: enter\n"));
	if(usbc_debug > 15)
		octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET);
	if (hprt & USBC_HPRT_PRTCONNDET) {
		DPRINTFN(3, ("%s: PRTCONNDET\n", __func__));
	}
#endif


	if (hprt & USBC_HPRT_PRTCONNDET) {
		sc->sc_connstat_change |= USBC_PRT_STAT_CONNECTED;
		octeon_usbc_reg_assert_msked(sc,USBC_HPRT_OFFSET,
				USBC_HPRT_PRTCONNDET,
				USBC_HPRT_WR1CMSK);
		DPRINTFN(3, ("%s: PRTCONNDET clear\n", __func__));
	}

	
	if (hprt & USBC_HPRT_PRTENCHNG) {
		DPRINTF(("octeon_usbc_intr_host_port: port status changing\n"));
		if (hprt & USBC_HPRT_PRTENA) {
			int timeout = 100000;
			sc->sc_port_speed
				= (hprt & USBC_HPRT_PRTSPD) >> USBC_HPRT_PRTSPD_OFFSET;
			DPRINTF(("octeon_usbc_intr_host_port: enumerated speed: 0x%02x\n",
						sc->sc_port_speed));
			do {
				hfnum = octeon_usbc_reg_rd(sc, USBC_HFNUM_OFFSET);
				if (timeout-- < 0) {
					hprt = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
					if (!(hprt & USBC_HPRT_PRTENA)) {
						DPRINTF(("octeon_usbc_intr_host_port: PRTENA down(%08x)\n", hprt));
						log(LOG_WARNING, "%s: waiting for frame number is expired,"
								" caused by unxpected port disabled.\n",
								device_xname(sc->sc_dev));
						return USBD_INVAL;
					}
					panic("octeon_usbc_intr_host_port: waiting for frame number is expired."
							" (hfnum=0x%08x, hprt=0x%08x)", hfnum, hprt);
				}
			} while ((hfnum & USBC_HFNUM_FRNUM) == 0x3FFF);

			if (((hprt & USBC_HPRT_PRTSPD) >> USBC_HPRT_PRTSPD_OFFSET)
					== USBC_HPRT_PRTSPD_HIGH)
				frint = sc->sc_frame_interval = 0x0EA6U; /* 3750 */
			else
				frint = sc->sc_frame_interval = 0x7530U; /* 30000 */


			hfir = octeon_usbc_reg_rd(sc, USBC_HFIR_OFFSET);
			hfir = (hfir & ~USBC_HFIR_FRINT) |
		    		(frint << USBC_HFIR_FRINT_OFFSET);
/*                        octeon_usbc_reg_wr(sc, USBC_HFIR_OFFSET, hfir);*/

			/* clear the interrupt and do not touch PRTENA */
			hprt &= ~USBC_HPRT_PRTENA;
			hprt |= USBC_HPRT_PRTENCHNG;
			octeon_usbc_reg_wr(sc, USBC_HPRT_OFFSET, hprt);
		} else if (hprt & USBC_HPRT_PRTOVRCURRACT) {
			log(LOG_CRIT, "%s: port overcurrent active\n",
					device_xname(sc->sc_dev));
			/* TODO what to do ? */
		} else if (hprt & USBC_HPRT_PRTCONNSTS && !sc->sc_isreset
				&& !(sc->sc_connstat_change & USBC_PRT_STAT_DISCONNECTED)){
			/* Port Error Condition (USB2.0 Spec. Sec. 11) */
			/* due to port babble ?? */
			log(LOG_CRIT, "%s: detect port babble\n",
					device_xname(sc->sc_dev));
			sc->sc_isprtbbl = 1;
		} 
	}

#ifdef USBC_DEBUG
	if (usbc_debug > 0) {
		octeon_usbc_debug_reg_dump(sc, USBC_GINTSTS_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GINTMSK_OFFSET);
	}
#endif
	DPRINTF(("octeon_usbc_intr_host_port: sc=%p xfer=%p\n", sc, xfer));

	p = KERNADDR(&xfer->dmabuf, 0);
	memset(p, 0, xfer->length);
	/* Pick out CHANGE bits from the status reg. */
	if (sc->sc_connstat_change
			|| hprt & (USBC_HPRT_PRTENCHNG | USBC_HPRT_PRTOVRCURRCHNG))
		p[0] |= 1 << 1; /* we have only one port */
	DPRINTF(("octeon_usbc_intr_host: change=0x%02x\n", *p));
	xfer->actlen = xfer->length;
	xfer->status = USBD_NORMAL_COMPLETION;

	usb_transfer_complete(xfer);

	return USBD_NORMAL_COMPLETION;
}
static void
octeon_usbc_intr_host_port_enable(void *_sc)
{
	octeon_usbc_softc_t *sc = _sc;
	int s;

	s = splhardusb();
	octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET, USBC_GINTMSK_PRTINTMSK);
	splx(s);
}

#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
static void
octeon_usbc_intr_txfifo_empty_enable(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	if (USBC_ENDPT_TYPE_IS_PERIO(hostch->sed->hcchar_eptype))
		octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET,
				USBC_GINTMSK_PTXFEMPMSK);
	else
		octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET,
				USBC_GINTMSK_NPTXFEMPMSK);
}
static void
octeon_usbc_intr_nptxfemp_enable(void *_sc)
{
	octeon_usbc_softc_t *sc = _sc;
	int s;

	s = splhardusb();
	octeon_usbc_reg_assert(sc, USBC_GINTMSK_OFFSET, USBC_GINTMSK_NPTXFEMPMSK);
	splx(s);
}
static void
octeon_usbc_intr_do_nak_retry(void *_hostch)
{
	octeon_usbc_hostch_t *hostch = _hostch;
	octeon_usbc_softc_t *sc = hostch->sc;
	int s;

	s = splhardusb();
	octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar,
			USBC_HCCHARX_CHENA);
	splx(s);
}


static void
octeon_usbc_intr_rxfifo_non_empty(octeon_usbc_softc_t *sc)
{
	u_int32_t grxstsph = octeon_usbc_reg_rd(sc, USBC_GRXSTSPH_OFFSET);
	u_int32_t ch_num = (grxstsph & USBC_GRXSTSPH_CHNUM)
				>> USBC_GRXSTSPH_CHNUM_OFFSET;
	u_int32_t bcnt = (grxstsph & USBC_GRXSTSPH_BCNT)
				>> USBC_GRXSTSPH_BCNT_OFFSET;
	u_int32_t pktsts = (grxstsph & USBC_GRXSTSPH_PKTSTS)
				>> USBC_GRXSTSPH_PKTSTS_OFFSET;
#ifdef USBC_DEBUG
	u_int32_t pid = (grxstsph & USBC_GRXSTSPH_DPID)
				>> USBC_GRXSTSPH_DPID_OFFSET;
#endif

	DPRINTFN(5,("%s: pktsts=0x%x, ch=%d, pid=0x%x bcnt=%d\n",
				__func__, pktsts, ch_num, pid, bcnt));

	/*
	 * See Synopsys DWC_otg document FIGURE 5-2 and 5-4.
	 */
	switch (pktsts) {
	case USBC_GRXSTSPH_PKTSTS_IN_DATA_RECV:
		DPRINTFN(15,("RECV  ch=%d, pid=0x%x, bcnt=%d | ", ch_num, pid, bcnt));
		if (bcnt > 0) {
			octeon_usbc_hostch_t *hostch;
			hostch = &sc->sc_hostch_list[ch_num];
	/*                hostch->sed->status = USBC_ENDPT_STATUS_IN_PROGRESS;*/

			KASSERT(!(hostch->std_cur->flags & USBC_STD_FLAG_SKIP));
			KASSERT(hostch->std_cur != NULL);
			KASSERT(hostch->std_cur->buff != NULL);
			KASSERT(hostch->sed != NULL);

			octeon_usbc_read_packet_from_host_channel(sc, hostch, bcnt);

			if (!USBC_ENDPT_TYPE_IS_PERIO(hostch->sed->hcchar_eptype))
				octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar,
						USBC_HCCHARX_CHENA);
		}
		break;
	case USBC_GRXSTSPH_PKTSTS_IN_XFER_COMPL:
		DPRINTFN(15,("COMPL  ch=%d, pid=0x%x, bcnt=%d | ", ch_num, pid, bcnt));
		break;
	case USBC_GRXSTSPH_PKTSTS_TGL_ERR:
		DPRINTFN(15,("TGLERR ch=%d, pid=0x%x, bcnt=%d | ", ch_num, pid, bcnt));
		break;
	case USBC_GRXSTSPH_PKTSTS_CH_HLTD:
		DPRINTFN(15,("CHHLTD ch=%d, pid=0x%x, bcnt=%d\n", ch_num, pid, bcnt));
		break;
	default:
		break;
	}
}

static int
octeon_usbc_intr_txfifo_empty_sub(octeon_usbc_softc_t *sc,
		octeon_usbc_soft_ed_t *head, u_int32_t *fifospace, u_int32_t *quespace)
{
	octeon_usbc_soft_ed_t *sed;
	u_int32_t actlen = 0;
	usbd_status err = USBD_NORMAL_COMPLETION;
	int remain_transact = 0;

/*        DPRINTFN(16,("%s: fifospace=%d, quespace=%d\n",*/
/*                                __func__, *fifospace, *quespace));*/
	for (sed = head; sed; sed = sed->next, actlen = 0){
		if (sed->status & USBC_ENDPT_STATUS_SKIP)
			continue;
		if (*quespace <= 0) {
#ifdef USBC_DEBUG
			if (usbc_debug > 0)
				log(LOG_DEBUG, "%s: Non periodic Request Queue is full "
						"(continue time)\n",
						device_xname(sc->sc_dev));
#endif
			break;
		}
		if (sed->status == USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY) {
			if (sed->hostch->std_cur->direction
					       	== USBC_ENDPT_DIRECTION_OUT) {
				sed->status = USBC_ENDPT_STATUS_IN_PROGRESS;
				err = octeon_usbc_write_packet_into_host_channel(sc,
						sed->hostch, *fifospace, &actlen);
				switch (err) {
				case USBD_IN_PROGRESS:
					sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
				case USBD_NORMAL_COMPLETION:
					*fifospace -= actlen;
					*quespace--;
					/* keep USBC_ENDPT_STATUS_IN_PROGRESS */
					break;
				default:
					sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
					break;
				}
			} else if (!USBC_ENDPT_TYPE_IS_PERIO(
						sed->hostch->sed->hcchar_eptype)) {
				octeon_usbc_reg_assert(sc, sed->hostch->offset_usbc_hcchar,
						USBC_HCCHARX_CHENA);
				*quespace--;
				/* keep soft endpoint status equaxs FWAIT_FIFO_EMPTY */
			}
			if (sed->status == USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY)
				remain_transact = 1;
		}
	}
	return remain_transact;
}

static void
octeon_usbc_intr_nperio_txfifo_empty(octeon_usbc_softc_t *sc)
{
	u_int32_t gnptxsts = octeon_usbc_reg_rd(sc, USBC_GNPTXSTS_OFFSET);
	u_int32_t quespace = (gnptxsts & USBC_GNPTXSTS_NPTXQSPCAVAIL)
			>> USBC_GNPTXSTS_NPTXQSPCAVAIL_OFFSET;
	u_int32_t fifospace = (gnptxsts & USBC_GNPTXSTS_NPTXFSPCAVAIL)
			>> USBC_GNPTXSTS_NPTXFSPCAVAIL_OFFSET;
	int remain = 0;

	remain |= octeon_usbc_intr_txfifo_empty_sub(sc,
			sc->sc_ctrl_head, &fifospace, &quespace);
	remain |= octeon_usbc_intr_txfifo_empty_sub(sc,
			sc->sc_bulk_head, &fifospace, &quespace);
	octeon_usbc_reg_deassert(sc, USBC_GINTMSK_OFFSET, USBC_GINTMSK_NPTXFEMPMSK);
	if (remain)
		callout_reset(&sc->sc_tmo_nptxfemp, USBC_SLAVE_MODE_NPTXEMP_INTERVAL,
				octeon_usbc_intr_nptxfemp_enable, sc);
}

static void
octeon_usbc_intr_perio_txfifo_empty(octeon_usbc_softc_t *sc)
{
	u_int32_t hptxsts = octeon_usbc_reg_rd(sc, USBC_HPTXSTS_OFFSET);
	u_int32_t quespace = (hptxsts & USBC_HPTXSTS_PTXQSPCAVAIL)
			>> USBC_HPTXSTS_PTXQSPCAVAIL_OFFSET;
	u_int32_t fifospace = (hptxsts & USBC_HPTXSTS_PTXFSPCAVAIL)
			>> USBC_HPTXSTS_PTXFSPCAVAIL_OFFSET;
	int remain = 0;

	remain = octeon_usbc_intr_txfifo_empty_sub(sc,
			sc->sc_intr_head, &fifospace, &quespace);
	if (!remain)
		octeon_usbc_reg_deassert(sc, USBC_GINTMSK_OFFSET,
				USBC_GINTMSK_PTXFEMPMSK);
}
#endif

static void
octeon_usbc_intr_host_channel(octeon_usbc_softc_t *sc)
{
	int ch;
	octeon_usbc_hostch_t *hostch;
	u_int32_t haint;
#ifdef USBC_DEBUG
	DPRINTFN(15, ("octeon_usbc_intr_host_channel: enter\n"));
	if (usbc_debug > 15) {
		octeon_usbc_debug_reg_dump(sc, USBC_GINTSTS_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GINTMSK_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_GRXSTSRH_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_HAINT_OFFSET);
		octeon_usbc_debug_reg_dump(sc, USBC_HAINTMSK_OFFSET);
	}
#endif
	haint = octeon_usbc_reg_rd(sc, USBC_HAINT_OFFSET);
	sc->sc_haint |= haint;
	sc->sc_haintmsk = octeon_usbc_reg_rd(sc, USBC_HAINTMSK_OFFSET);
	for (ch = 0; ch < sc->sc_hwcfg.numhstch; ch++) {
		if (haint & sc->sc_hostch_list[ch].hc_id) {
			hostch = &(sc->sc_hostch_list[ch]);
#ifdef USBC_DEBUG
/*                        if (usbc_debug > 15) {*/
			if (usbc_debug > 9 && !(hostch->hcint & USBC_HCINTX_NAK)) {
				octeon_usbc_debug_reg_dump(sc,
						hostch->offset_usbc_hcint);
				octeon_usbc_debug_reg_dump(sc,
						hostch->offset_usbc_hcintmsk);
			}
#endif
			hostch->hcint |= octeon_usbc_reg_rd(sc,
					hostch->offset_usbc_hcint);
			hostch->hcintmsk |= octeon_usbc_reg_rd(sc,
					hostch->offset_usbc_hcintmsk);
			hostch->hcchar = octeon_usbc_reg_rd(sc,
					hostch->offset_usbc_hcchar);
			hostch->hctsiz = octeon_usbc_reg_rd(sc,
					hostch->offset_usbc_hctsiz);
			hostch->hcsplt = octeon_usbc_reg_rd(sc,
					hostch->offset_usbc_hcsplt);

			/* clear intr */
			octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcint,
					hostch->hcint);

			if (sc->sc_dma_enable) {
				if ( hostch->hcint & (USBC_HCINTX_ACK | USBC_HCINTX_NYET |
							USBC_HCINTX_NAK | USBC_HCINTX_DATATGLERR))
					octeon_usbc_reg_deassert(sc, hostch->offset_usbc_hcintmsk,
							USBC_HCINTMSKX_ACKMSK |
							USBC_HCINTMSKX_NYETMSK |
							USBC_HCINTMSKX_NAKMSK |
							USBC_HCINTMSKX_DATATGLERRMSK);
#if 0
				/*
				 * Skip soft interrupt code during NYET response
				 * This provides high performace of HS devices
				 * and low performace of system.
				 */
				if ( hostch->hcint & USBC_HCINTX_NYET 
						&& !(hostch->hcint & USBC_HCINTX_XFERCOMPL)) {
					hostch->hcint &= ~(USBC_HCINTX_CHHLTD|USBC_HCINTX_NYET);
					sc->sc_haint &= ~hostch->hc_id;
					hostch->std_cur->offset = octeon_usbc_calc_actlen_offset(
							hostch->std_cur->len,
							hostch->sed->hcchar_mps,
							hostch->hctsiz);
					hostch->sed->ping_state = 1;
					octeon_usbn_set_phyaddr(sc, hostch->std_cur->buff,
							hostch->hc_num,
							hostch->std_cur->direction,
							hostch->std_cur->offset
							+ hostch->std_cur->base_offset);
					octeon_usbc_enable_host_channel(sc, hostch);
/*                                        octeon_usbc_set_transact(sc, hostch);*/
				}
#endif
			}
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
			else if (!(hostch->hcint & USBC_HCINTX_CHHLTD)) {
				if ((hostch->std_cur->direction == USBC_ENDPT_DIRECTION_IN)
						&& (hostch->hcint & USBC_HCINTX_NAK)
						&& (!USBC_ENDPT_TYPE_IS_PERIO(
								hostch->sed->hcchar_eptype))) {
					/*
					 * When Non periodic IN transaction return NAK state,
					 * retry it.
					 */ 
					hostch->hcint &= ~USBC_HCINTX_NAK;
					callout_reset(&hostch->tmo_retry,
							USBC_SLAVE_MODE_NAK_INTERVAL,
							octeon_usbc_intr_do_nak_retry,
							hostch);
				} else if (hostch->sed->status == USBC_ENDPT_STATUS_WAIT_PING_ACK
						&& (hostch->hcint & USBC_HCINTX_ACK)){
					DPRINTFN(15, ("%s:ping ACK\n", __func__));
					octeon_usbc_reg_deassert(sc, hostch->offset_usbc_hctsiz,
							USBC_HCTSIZX_DOPNG);
					octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar,
							USBC_HCCHARX_CHENA);
					hostch->sed->ping_state = 0;
					hostch->sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
					octeon_usbc_intr_txfifo_empty_enable(sc, hostch);
				} else if (hostch->sed->status == USBC_ENDPT_STATUS_WAIT_PING_ACK
						&& (hostch->hcint & USBC_HCINTX_NAK)){
					DPRINTFN(15, ("%s:ping NAK\n", __func__));
					octeon_usbc_reg_assert(sc, hostch->offset_usbc_hctsiz,
							USBC_HCTSIZX_DOPNG);
				} else if (hostch->hcint & (USBC_HCINTX_XFERCOMPL
							| USBC_HCINTX_DATATGLERR
							| USBC_HCINTX_STALL
							| USBC_HCINTX_NAK
							| USBC_HCINTX_NYET
							| USBC_HCINTX_FRMOVRUN
							| USBC_HCINTX_XACTERR
							| USBC_HCINTX_BBLERR)) {
					/*
					 * In slave mode, we have to halt host channel,
					 * if get above interrupt status.
					 * See Section 5.4.5.4.1 of Synopsys DWC_otg Document.
					 */
					octeon_usbc_abort_host_channel(sc, hostch);
					hostch->status &= ~USBC_HOST_CH_STATUS_WAIT_HLT;
				}
				sc->sc_haint &= ~hostch->hc_id;
			}
#endif
		}
	}
	OCTEON_SYNC;
}

static void
octeon_usbc_softintr(void *aux)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)aux;
	octeon_usbc_hostch_t *hostch;
/*        struct octeon_usbc_pipe *usbc_pipe;*/

	u_int32_t haint;
	u_int32_t hcint;
	int ch;
	int s;

	s = splhardusb();
	haint = sc->sc_haint;
	sc->sc_haint = 0;
	splx(s);

	for (ch = 0; ch < sc->sc_hwcfg.numhstch; ch++) {
		if (haint & sc->sc_hostch_list[ch].hc_id) {
			usbd_xfer_handle xfer;
			octeon_usbc_soft_td_t *std_cur;
			usbd_status error = USBD_INVAL;
			int is_in, is_hs;
			int is_intr = 0, is_nperio = 0;
			int do_retry = 0;

			hostch = &(sc->sc_hostch_list[ch]);
			if (!(hostch->status & USBC_HOST_CH_STATUS_RESERVED))
				continue;

			std_cur = hostch->std_cur;
			KASSERT(std_cur != NULL);
			KASSERT(std_cur->xfer != NULL);
			KASSERT(std_cur->sed != NULL);
			if (!std_cur || !std_cur->xfer || !std_cur->sed) {
				log(LOG_WARNING, "%s: Wrong host channel values. Skip it.\n",
						device_xname(sc->sc_dev));
				continue;
			}
			s = splhardusb();
			hcint = hostch->hcint;
			hostch->hcint = 0;
			splx(s);

			if (sc->sc_dma_enable && !(hcint & USBC_HCINTX_CHHLTD))
					continue;

			if ((hostch->status & USBC_HOST_CH_STATUS_WAIT_HLT) != 0x0
					&& (hcint & USBC_HCINTX_CHHLTD)) {
				DPRINTFN(5, ("%s: free hc by STATUS_WAIT_HLT, ch.%d xfer=%p\n",
						__func__, hostch->hc_num, std_cur->xfer));
				octeon_usbc_free_host_channel(sc, hostch, USBD_CANCELLED);
				continue;
			}

			is_in = std_cur->direction;
			is_hs = std_cur->xfer->pipe->device->speed == USB_SPEED_HIGH ? 1 : 0;
			xfer = std_cur->xfer;
			is_intr = (std_cur->sed->hcchar_eptype
					== USBC_ENDPT_TYPE_INTR);
			is_nperio = (std_cur->sed->hcchar_eptype
				       	== USBC_ENDPT_TYPE_BULK ||
					std_cur->sed->hcchar_eptype
					== USBC_ENDPT_TYPE_CTRL);

			if (sc->sc_dma_enable
			    && !(hcint & ~(USBC_HCINTX_CHHLTD | USBC_HCINTX_XXX_31_11))) {
				/*
				 * In DMA mode, i dont know why, but sometimes,
				 * channel halts without no Error Intr bit or
				 * channel aboting...
				 * For now, treat this interrupt as XACTERR.
				 */
				log(LOG_WARNING, "%s: Unexpected H/W interrupt"
						" that has only HALTED bit."
						" Treat as XACTERR. (ch.%d)\n", 
						device_xname(sc->sc_dev),
						ch);
				hcint |= USBC_HCINTX_XACTERR;
			}

			if (hcint & (USBC_HCINTX_ACK | USBC_HCINTX_NYET)) {
				std_cur->error_count = 0;
				s = splusb();
				DPRINTFN(10, ("octeon_usbc_softintr: ch.%d: "
						"PID=%x\n", ch,
						hostch->hctsiz & USBC_HCTSIZX_PID));
				if (std_cur->sed->hcchar_eptype
						== USBC_ENDPT_TYPE_ISOC) {
					/* TODO:isoc */
				} else if ((hostch->hctsiz & USBC_HCTSIZX_PID) == 0x0) {
					 std_cur->sed->toggle_state = USBC_DATA_TGL_DATA0;
				} else {
					 std_cur->sed->toggle_state = USBC_DATA_TGL_DATA1;
				}
				if (std_cur->pid != USBC_DATA_TGL_SETUP)
					std_cur->pid = std_cur->sed->toggle_state;
				splx(s);

				if ((hcint & USBC_HCINTX_NYET) && !is_in && is_hs)
					std_cur->sed->ping_state = 1;
				else
					std_cur->sed->ping_state = 0;
			}

			if (hcint & USBC_HCINTX_XFERCOMPL) {
				std_cur->error_count = 0;
				octeon_usbc_free_host_channel(sc, hostch,
						USBD_NORMAL_COMPLETION);
				error = octeon_usbc_softintr_xfercompl(
						sc, hostch);
				continue;
			}
			if (hcint & USBC_HCINTX_XACTERR) {
				log(LOG_ERR, "%s: Transaction error"
						" (ch.%d)\n",
						device_xname(sc->sc_dev),
						ch);
#ifdef USBC_DEBUG
				if(usbc_debug > 0) {
					octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET);
					octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hcchar);
					octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hctsiz);
					octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hcint);
					octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hcintmsk);
					octeon_usbc_debug_reg_dump(sc, USBC_HAINTMSK_OFFSET);
				}
#endif

				std_cur->error_count++;

				/*
				 * XXX If device is disconnected, no ACK and no others
				 * will come
				 */
				s = splhardusb();
				if (sc->sc_connstat_change & USBC_PRT_STAT_DISCONNECTED) {
					splx(s);
					continue;
				}
				if (std_cur->error_count >= 3)
					std_cur->sed->status
						= USBC_ENDPT_STATUS_READY;
				splx(s);

				if (std_cur->error_count >= 3) {
					s = splusb();
					callout_stop(&xfer->timeout_handle);
					std_cur->error_count = 0;
					error = xfer->status = USBD_IOERROR;
					usb_transfer_complete(xfer);
					octeon_usbc_free_host_channel(sc, hostch, error);
					splx(s);
					continue;
				} else {
					do_retry = 1;
					if (!is_in && is_hs)
						std_cur->sed->ping_state = 1;
					else
						std_cur->sed->ping_state = 0;
				}
			}
			if (hcint & USBC_HCINTX_DATATGLERR) {
				log(LOG_WARNING, "%s: Data toggle error"
						" (ch.%d)\n",
						device_xname(sc->sc_dev),
						ch);
				std_cur->error_count = 0;
				if (is_nperio && is_in) {
					do_retry = 1;
					std_cur->offset = octeon_usbc_calc_actlen_offset(
							std_cur->len,
							std_cur->sed->hcchar_mps,
							hostch->hctsiz);
				} else if (is_intr)
					do_retry = 1;
			}
			if (hcint & USBC_HCINTX_BBLERR) {
				log(LOG_WARNING, "%s: Channel babble error"
						" (ch.%d)\n",
						device_xname(sc->sc_dev),
						ch);
				std_cur->error_count = 0;
				do_retry = 1;
			}
			if (hcint & (USBC_HCINTX_NAK|USBC_HCINTX_NYET)) {
				std_cur->error_count = 0;
				if (is_nperio && !is_in) {
					do_retry = 1;
					std_cur->offset = octeon_usbc_calc_actlen_offset(
							std_cur->len,
							std_cur->sed->hcchar_mps,
							hostch->hctsiz);
					if (is_hs)
						std_cur->sed->ping_state = 1;
					else
						std_cur->sed->ping_state = 0;
				} else if (is_intr) {
#if 0
					log(LOG_DEBUG, "%s: NAK reply on ch.%d\n",
							device_xname(sc->sc_dev),
							ch);
#endif
					do_retry = 1;
				}
			}
			if (hcint & USBC_HCINTX_FRMOVRUN) {
#ifdef USBC_DEBUG
				log(LOG_DEBUG, "%s: Frame overrun ch.%d\n",
						device_xname(sc->sc_dev),
						ch);
#endif
				if (is_intr)
					do_retry = 1;
			}
			if (hcint & USBC_HCINTX_STALL) {
#ifdef USBC_DEBUG
				if (usbc_debug > 0)
					log(LOG_DEBUG, "%s: STALL ch.%d\n",
							device_xname(sc->sc_dev), ch);
#endif
				s = splusb();
				callout_stop(&xfer->timeout_handle);
				std_cur->error_count = 0;
				std_cur->sed->status = USBC_ENDPT_STATUS_READY;
				error = xfer->status = USBD_STALLED;
				usb_transfer_complete(xfer);
				octeon_usbc_free_host_channel(sc, hostch, error);
				splx(s);
				continue;
			}

			if (do_retry) {
				s = splusb();
				if (!USBC_IS_ABORTED_XFER(xfer))
					octeon_usbc_set_transact(sc, hostch);
				splx(s);
			}
		}
	}

#ifdef USB_USE_SOFTINTR
	if (sc->sc_softwake) {
		sc->sc_softwake = 0;
		wakeup(&sc->sc_softwake);
	}
#endif /* USB_USE_SOFTINTR */

	DPRINTFN(15,("octeon_usbc_softintr: done:\n"));
}

static usbd_status
octeon_usbc_softintr_xfercompl(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	octeon_usbc_soft_td_t *std, *std_next, *std_done_head = NULL;
	octeon_usbc_soft_ed_t *sed = NULL;
	usbd_xfer_handle xfer;
	int s;

	DPRINTFN(5,("octeon_usbc_softintr_xfercompl: "
		"hostch=%p, hostch->std_done_head=%p\n", hostch, hostch->std_done_head));

	KASSERT(hostch->std_done_head != NULL);

	s = splusb();
	for (std = hostch->std_done_head; std; std = std_next) {
		std_next = std->next;
		std->next = std_done_head;
		std_done_head = std;
	}

	for (std = std_done_head; std; std = std_next) {
		std_next = std->next;

		sed = std->sed;
		xfer = std->xfer;
		if (std->flags & USBC_STD_FLAG_SKIP)
			continue;
		KASSERT(!(std->flags & USBC_STD_FLAG_SKIP));
		KASSERT(xfer != NULL);

		DPRINTFN(5,("octeon_usbc_softintr_xfercompl: "
			"std = %p, sed = %p, sed->std_head=%p, xfer=%p\n",
					std, sed, sed->std_head, xfer));

		if(xfer == NULL)
			continue;
		if (USBC_IS_ABORTED_XFER(xfer)) {
			DPRINTFN(5,("octeon_usbc_softintr_xfercompl: cancel/timeout %p\n",
				 xfer));
			/* XXX Handled by abort routine. */
			continue;
		}
#if defined(USBC_DEBUG)
		if (usbc_debug > 5) {
			if (std->len > 0 && std->direction == USBC_ENDPT_DIRECTION_IN) {
				/* debugdump */
				int i;
				unsigned char *c = KERNADDR(std->buff, std->base_offset);
				DPRINTFN(5, ("%s: ADDR=%d:EP=%d(type=%d) : %s DATA(%4d/%4d)= ",
						device_xname(sc->sc_dev),
						hostch->std_cur->sed->hcchar_devaddr,
						std->sed->hcchar_epnum,
						std->sed->hcchar_eptype,
						(std->direction) ? "IN " : "OUT",
						std->actlen,
						std->len
					     ));
				for(i=0; i < std->actlen; i++)
					DPRINTFN(5, ("%02x ", c[i]));
				DPRINTFN(5, ("\n"));
			}
		}
#endif
		if (std->pid != USBC_DATA_TGL_SETUP)
			xfer->actlen += std->actlen;

		if (std->flags & USBC_STD_FLAG_CALL_DONE) {
#if defined(USBC_DEBUG)
			DPRINTFN(5, ("%s: ADDR=%d:EP=%d(type=%d,%s): xfer done. %d/%d\n",
					device_xname(sc->sc_dev),
					std->sed->hcchar_devaddr,
					std->sed->hcchar_epnum,
					std->sed->hcchar_eptype,
					(std->direction)?"IN ":"OUT",
					xfer->actlen,
					xfer->length));
#endif
			callout_stop(&xfer->timeout_handle);
			sed->status = USBC_ENDPT_STATUS_READY;
			xfer->status = USBD_NORMAL_COMPLETION;
/*                        s = splusb();*/
			usb_transfer_complete(xfer);
/*                        splx(s);*/
		} else {
			usbd_status err;
#if defined(USBC_DEBUG) & 1
			DPRINTFN(5, ("%s: ADDR=%d:EP=%d(type=%d,%s): xfer continue. %d/%d\n",
					device_xname(sc->sc_dev),
					std->sed->hcchar_devaddr,
					std->sed->hcchar_epnum,
					std->sed->hcchar_eptype,
					(std->direction)?"IN ":"OUT",
					xfer->actlen,
					xfer->length));
#endif
			xfer->hcpriv = sed->std_head;
			if (xfer->status == USBD_IN_PROGRESS) {
				err = octeon_usbc_set_next_transact(sc, sed);
			} else {
				err = xfer->status;
				usb_endpoint_descriptor_t *ed = xfer->pipe->endpoint->edesc;
				log(LOG_WARNING, "%s: The xfer is not in progress,"
						" break xfer continuing. (pipe=%d(%s), status=%s)\n",
						device_xname(sc->sc_dev),
						UE_GET_ADDR(ed->bEndpointAddress),
						UE_GET_DIR(ed->bEndpointAddress)
								== UE_DIR_IN ? "IN" : "OUT",
						usbd_errstr(err));
			}

			if (err != USBD_IN_PROGRESS) {
				DPRINTF(("%s: fault set_next_transact() (%s)\n",
						__func__, usbd_errstr(err)));
				callout_stop(&xfer->timeout_handle);
				sed->std_head = octeon_usbc_drop_soft_td(sc,
						sed->std_head, xfer);
				sed->status = USBC_ENDPT_STATUS_READY;
				xfer->status = err;
				usb_transfer_complete(xfer);
			}
		}

#if 0
		if (std->flags & USBC_STD_FLAG_CALL_DONE) {
			DPRINTF(("octeon_usbc_softintr_xfercompl: "
						"STD_FLAG_CALL_DONE\n"));
			xfer->status = USBD_NORMAL_COMPLETION;
			usb_transfer_complete(xfer);
		} else {
			DPRINTF(("octeon_usbc_softintr_xfercompl: "
						"mark ED as Ready for next TD\n"));
		       	sed->status = USBC_ENDPT_STATUS_READY;
		}
#endif
		DPRINTFN(5, ("%s: free std=%p, xfer=%p\n",__func__, std, xfer));
		octeon_usbc_free_soft_td(sc, std);
	}
	hostch->std_done_head = NULL;
	splx(s);
	return USBD_NORMAL_COMPLETION;

}

static inline int
octeon_usbc_calc_actlen_offset(int len, int mps, u_int32_t hctsiz_reg)
{
	/* At least one packet is needed, even if length is Zero. */
	int pktcnt = len ? (len + (mps - 1)) / mps : 1;
	int remain_pktcnt = (hctsiz_reg & USBC_HCTSIZX_PKTCNT)
				>> USBC_HCTSIZX_PKTCNT_OFFSET;
	return (pktcnt - remain_pktcnt) * mps;
}

static octeon_usbc_hostch_t *
octeon_usbc_alloc_host_channel(octeon_usbc_softc_t *sc)
{
	int i=0;
	octeon_usbc_hostch_t *hostch = NULL;

#ifdef USBC_DEBUG
	for (i=0; i < usbc_hc_cap; i++){
#else
	for (i=0; i < sc->sc_hwcfg.numhstch; i++){
#endif
		if (sc->sc_hostch_list[i].status
				== USBC_HOST_CH_STATUS_FREE){
			sc->sc_hostch_list[i].status
			       |= USBC_HOST_CH_STATUS_RESERVED;
			hostch = &(sc->sc_hostch_list[i]);
			DCOUNTUP(usbc_used_hc);
			break;
		}
	}

	if (hostch != NULL) {
		hostch->sed = NULL;
		hostch->std_cur = NULL;
		DPRINTFN(5,("octeon_usbc_alloc_host_channel: reserve ch.%d\n", i));
	} else
		log(LOG_CRIT, "%s: all host channels are busy.\n",
				device_xname(sc->sc_dev));

	return hostch;
}


static inline int
octeon_usbc_setup_host_channel(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t * hostch)
{
	octeon_usbc_soft_ed_t *sed = hostch->sed;

	hostch->hcintmsk = 0;
	if (sc->sc_dma_enable) {
		hostch->hcintmsk |= USBC_HCINTMSKX_CHHLTDMSK;
		hostch->hcintmsk |= USBC_HCINTMSKX_AHBERRMSK;
		if ( hostch->error_state &&
				sed->hcchar_eptype != USBC_ENDPT_TYPE_ISOC
				) {
			hostch->hcintmsk |= USBC_HCINTMSKX_ACKMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_NYETMSK;
			if ( hostch->std_cur->direction == USBC_ENDPT_DIRECTION_IN ) {
				hostch->hcintmsk |= USBC_HCINTMSKX_DATATGLERRMSK;
				if (sed->hcchar_eptype != USBC_ENDPT_TYPE_INTR) {
					hostch->hcintmsk |= USBC_HCINTMSKX_NAKMSK;
				}
			}
		}
	} else {
		switch (hostch->sed->hcchar_eptype) {
		case USBC_ENDPT_TYPE_CTRL:
		case USBC_ENDPT_TYPE_BULK:
			hostch->hcintmsk |= USBC_HCINTMSKX_XFERCOMPLMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_NAKMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_STALLMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_XACTERRMSK;
			if(hostch->std_cur->direction == USBC_ENDPT_DIRECTION_IN){
				hostch->hcintmsk |= USBC_HCINTMSKX_DATATGLERRMSK;
				hostch->hcintmsk |= USBC_HCINTMSKX_BBLERRMSK;
			} else {
				hostch->hcintmsk |= USBC_HCINTMSKX_NYETMSK;
				if (hostch->hctsiz & USBC_HCTSIZX_DOPNG) {
					hostch->hcintmsk |= USBC_HCINTMSKX_ACKMSK;
				}
			}
			if (hostch->error_state)
				hostch->hcintmsk |= USBC_HCINTMSKX_CHHLTDMSK;

			/* TODO if split */
			if (hostch->hctsiz & USBC_HCTSIZX_DOPNG) {
				hostch->hcintmsk |= USBC_HCINTMSKX_ACKMSK;
			}
			break;
		case USBC_ENDPT_TYPE_INTR:
			hostch->hcintmsk |= USBC_HCINTMSKX_XFERCOMPLMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_NAKMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_STALLMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_XACTERRMSK;
			hostch->hcintmsk |= USBC_HCINTMSKX_FRMOVRUNMSK;
			if (hostch->std_cur->direction == USBC_ENDPT_DIRECTION_IN) {
				hostch->hcintmsk |= USBC_HCINTMSKX_DATATGLERRMSK;
				hostch->hcintmsk |= USBC_HCINTMSKX_BBLERRMSK;
			}
			if (hostch->error_state) {
				hostch->hcintmsk |= USBC_HCINTMSKX_CHHLTDMSK;
				hostch->hcintmsk |= USBC_HCINTMSKX_ACKMSK;
			}
			/* TODO if split */
			break;
		case USBC_ENDPT_TYPE_ISOC:
			/* TODO isoc type */
			break;
		default:
			break;
		}
	}
	hostch->hcchar &= ~USBC_HCCHARX_CHENA;
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcchar, hostch->hcchar & ~USBC_HCCHARX_CHENA);
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hctsiz, hostch->hctsiz);
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcint, 0xFFFFFFFF & ~USBC_HCINTX_XXX_31_11);
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcintmsk, hostch->hcintmsk);

	return USBD_NORMAL_COMPLETION;
}

static inline void
octeon_usbc_enable_host_channel(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	DPRINTFN(10,("octeon_usbc_enable_host_channel: enable ch.%d\n", hostch->hc_num));
	KASSERT(hostch->sed->status == USBC_ENDPT_STATUS_IN_PROGRESS);
#ifdef USBC_DEBUG
	if (usbc_debug > 15) {
		octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET);
		octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hcchar);
		octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hctsiz);
	}
#endif
	octeon_usbc_reg_assert(sc, USBC_HAINTMSK_OFFSET, hostch->hc_id);
	if (sc->sc_dma_enable) {
		octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar, USBC_HCCHARX_CHENA);
	}
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
	else if (hostch->std_cur->direction == USBC_ENDPT_DIRECTION_OUT
			&& hostch->sed->ping_state) {
		/* wait for ping ack */
		octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar, USBC_HCCHARX_CHENA);
		hostch->sed->status = USBC_ENDPT_STATUS_WAIT_PING_ACK;
	}
	else {
		usbd_status err = USBD_NORMAL_COMPLETION;
		u_int32_t actlen = 0;
		int32_t quespace;
		u_int32_t fifospace;

		if (USBC_ENDPT_TYPE_IS_PERIO(hostch->sed->hcchar_eptype)) {
			u_int32_t hptxsts = octeon_usbc_reg_rd(sc, USBC_HPTXSTS_OFFSET);
			quespace = (hptxsts & USBC_HPTXSTS_PTXQSPCAVAIL)
					>> USBC_HPTXSTS_PTXQSPCAVAIL_OFFSET;
			fifospace = (hptxsts & USBC_HPTXSTS_PTXFSPCAVAIL)
					>> USBC_HPTXSTS_PTXFSPCAVAIL_OFFSET;
		} else {
			u_int32_t gnptxsts = octeon_usbc_reg_rd(sc, USBC_GNPTXSTS_OFFSET);
			quespace = (gnptxsts & USBC_GNPTXSTS_NPTXQSPCAVAIL)
					>> USBC_GNPTXSTS_NPTXQSPCAVAIL_OFFSET;
			fifospace = (gnptxsts & USBC_GNPTXSTS_NPTXFSPCAVAIL)
					>> USBC_GNPTXSTS_NPTXFSPCAVAIL_OFFSET;
		}

		if (quespace > 0) {
			octeon_usbc_reg_assert(sc, hostch->offset_usbc_hcchar,
					USBC_HCCHARX_CHENA);
			if (hostch->std_cur->direction == USBC_ENDPT_DIRECTION_OUT)
				err = octeon_usbc_write_packet_into_host_channel(sc,
						hostch, fifospace, &actlen);
			else
				err = USBD_NORMAL_COMPLETION;

			switch (err) {
			case USBD_IN_PROGRESS:
				hostch->sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
				break;
			case USBD_NORMAL_COMPLETION:
				/* USBC_ENDPT_STATUS_IN_PROGRESS */
				break;
			case USBD_INVAL:
			default:
				hostch->sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
				break;
			}
		} else {
#ifdef USBC_DEBUG
			if (usbc_debug > 0)
				log(LOG_DEBUG, "%s: Non periodic Request Queue "
						"is full (initial time)\n",
						device_xname(sc->sc_dev));
#endif
			hostch->sed->status = USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY;
		}
		if (hostch->sed->status == USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY)
			octeon_usbc_intr_txfifo_empty_enable(sc, hostch);
	}
#endif
}

static int
octeon_usbc_free_host_channel(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch, usbd_status status)
{
	KASSERT(hostch != NULL);
	DPRINTFN(15,("%s: enter release ch.%d\n", __func__, hostch->hc_num));
	if (hostch->status & USBC_HOST_CH_STATUS_RESERVED){
		octeon_usbc_soft_td_t *std;
		int s;
		int hctsizx_xfersize;

		s = splhardusb();
		octeon_usbc_reg_deassert(sc, USBC_HAINTMSK_OFFSET, hostch->hc_id);
		octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcintmsk, 0);
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hcchar);
#endif
		splx(s);

		hctsizx_xfersize = (hostch->hctsiz & USBC_HCTSIZX_XFERSIZE )
				>> USBC_HCTSIZX_XFERSIZE_OFFSET;

		DPRINTFN(5,("%s: release ch.%d\n", __func__, hostch->hc_num));
		s = splusb();
		std = hostch->std_cur;
		KASSERT(std != NULL);
		switch (status) {
		case USBD_NORMAL_COMPLETION:
#ifdef USBC_DEBUG
			if (usbc_debug > 15)
				octeon_usbc_debug_reg_dump(sc, hostch->offset_usbc_hctsiz);
#endif
			if (std->direction == USBC_ENDPT_DIRECTION_IN)
				std->actlen = (std->len > hctsizx_xfersize)
					? std->len - hctsizx_xfersize : 0;
			else
				std->actlen = std->len;

			std->next = hostch->std_done_head;
			hostch->std_done_head = std;
			DPRINTFN(5,("%s: std=%p done. status=%d,"
						" actlen=%d, xfersize=%d\n",
						__func__,
						hostch->std_done_head, status,
						std->actlen, hctsizx_xfersize));
			break;
		case USBD_CANCELLED:
		case USBD_STALLED:
		case USBD_TIMEOUT:
		case USBD_IOERROR:
		case USBD_NOMEM:
		default:
			DPRINTF(("%s: err=%s\n",__func__, usbd_errstr(status)));
			octeon_usbc_free_soft_td(sc, hostch->std_cur);

			/* error ? */
			DPRINTFN(5,("%s: remove std=%p from hostch. status=%d\n",
						__func__, std, status));
			break;
		}
		splx(s);

		DPRINTF(("%s: free ch.%d sed=%p std=%p xfer=%p\n", __func__,
				hostch->hc_num, hostch->sed,
				hostch->std_cur, hostch->std_cur->xfer));
		hostch->status = USBC_HOST_CH_STATUS_FREE;
		DCOUNTDOWN(usbc_used_hc);
		/*
		hostch->sed->hostch = NULL;
		hostch->sed = NULL;
		hostch->std_cur = NULL;
		*/

	}

	return 0;
}
static int
octeon_usbc_halt_host_channel(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	DPRINTFN(2, ("%s: halt ch.%d status=0x%04x sed->status=%d\n", __func__, hostch->hc_num,
				hostch->status,
				hostch->sed->status));
	u_int32_t hcint = 0;
	u_int32_t hcchar = 0;

	hcint = octeon_usbc_reg_rd(sc, hostch->offset_usbc_hcint);
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcint, hcint);

	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcintmsk,
			USBC_HCINTMSKX_CHHLTDMSK);

	hcchar = octeon_usbc_reg_rd(sc, hostch->offset_usbc_hcchar);
	hcchar |= USBC_HCCHARX_CHDIS;
	hcchar |= USBC_HCCHARX_CHENA;
	octeon_usbc_reg_wr(sc, hostch->offset_usbc_hcchar, hcchar);

	return 0;
}
static int
octeon_usbc_abort_host_channel(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	if (hostch->status & USBC_HOST_CH_STATUS_WAIT_HLT) {
		DPRINTFN(5,("octeon_usbc_abort_host_channel: already aborting ch.%d\n", hostch->hc_num));
		return 1;
	} else if (hostch->status & USBC_HOST_CH_STATUS_RESERVED){
		hostch->status |= USBC_HOST_CH_STATUS_WAIT_HLT;
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
		if (!sc->sc_dma_enable)
			callout_stop(&hostch->tmo_retry);
		if (hostch->sed->status == USBC_ENDPT_STATUS_IN_PROGRESS
			|| hostch->sed->status == USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY) {
			/* stop retry BULK IN transactions */
			hostch->sed->status = USBC_ENDPT_STATUS_IN_PROGRESS;
#else
		if (hostch->sed->status == USBC_ENDPT_STATUS_IN_PROGRESS) {
#endif

			/* clear host channel intr */
			octeon_usbc_halt_host_channel(sc, hostch);

		} else {
			hostch->sed->status = USBC_ENDPT_STATUS_READY;
			octeon_usbc_free_host_channel(sc, hostch, USBD_CANCELLED);
		}
		DPRINTF(("octeon_usbc_abort_host_channel: abort ch.%d status=%d\n",
					hostch->hc_num, hostch->sed->status));
	}

	return 0;
}


static usbd_status
octeon_usbc_open(usbd_pipe_handle pipe)
{
	usbd_device_handle dev = pipe->device;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)dev->bus;
	usb_endpoint_descriptor_t *ed = pipe->endpoint->edesc;
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_soft_ed_t *sed;
	octeon_usbc_soft_td_t *std;
	u_int8_t addr = dev->address;
	u_int8_t xfertype = ed->bmAttributes & UE_XFERTYPE;
	int s;

	usbd_status err;

	DPRINTF(("octeon_usbc_open: pipe=%p, addr=%x, endpt=%d (%d)\n",
		     pipe, addr, ed->bEndpointAddress, sc->sc_addr));

	if (addr == sc->sc_addr) {
		switch (ed->bEndpointAddress) {
		case USB_CONTROL_ENDPOINT:
			pipe->methods = &octeon_usbc_root_ctrl_methods;
			break;
		case UE_DIR_IN | USBC_INTR_ENDPT:
			pipe->methods = &octeon_usbc_root_intr_methods;
			break;
		default:
			return (USBD_INVAL);
		}
		return (USBD_NORMAL_COMPLETION);
	}

	sed = octeon_usbc_alloc_soft_ed(sc);
	if (sed == NULL)
		goto bad0;
	usbc_pipe->sed = sed;

	std = octeon_usbc_alloc_soft_td(sc);
	if (std == NULL)
		goto bad1;
	std->flags |= USBC_STD_FLAG_SKIP;
	usbc_pipe->tail.std = std;

	sed->hcchar_mps = UGETW(ed->wMaxPacketSize);
	sed->hcchar_epnum = UE_GET_ADDR(ed->bEndpointAddress);
	sed->hcchar_devaddr = addr;
	sed->toggle_state = USBC_DATA_TGL_DATA0;
	sed->status = USBC_ENDPT_STATUS_READY;
	sed->std_head = std;

	switch (xfertype) {
	case UE_CONTROL:
		DPRINTF(("%s: open control pipe at addr#%d endpoint#%d(%s).\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum,
				is_read ? "IN" : "OUT"));

		pipe->methods = &octeon_usbc_device_ctrl_methods;
		err = usb_allocmem(&sc->sc_bus,
			  sizeof(usb_device_request_t),
			  0, &usbc_pipe->u.ctl.req_setup_dma);

		/*
		 * The buffer is required for status stage?
		 * Cavium's Linux SDK code say, "This space acts a bit bucket."
		 */
		err = usb_allocmem(&sc->sc_bus,
			  USBC_CONTROL_STATUS_BUF_SIZE,
			  0, &usbc_pipe->u.ctl.req_stat_dma);
		if (err)
			goto bad;

		sed->hcchar_eptype = USBC_ENDPT_TYPE_CTRL;
		s = splusb();
		octeon_usbc_add_ed(sed, sc->sc_ctrl_head);
		splx(s);
		break;
	case UE_INTERRUPT:
		DPRINTF(("%s: open interrupt pipe at addr#%d endpoint#%d(%s).\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum,
				is_read ? "IN" : "OUT"));

		pipe->methods = &octeon_usbc_device_intr_methods;
		if (pipe->interval == USBD_DEFAULT_INTERVAL)
			sed->interval = dev->speed == USB_SPEED_HIGH ?
				1 << (ed->bInterval - 1) : ed->bInterval;
		else
			sed->interval = pipe->interval;
		sed->interval -= sed->interval > 1 ? 1 : 0; /* XXX */
		DPRINTFN(2, ("octeon_usbc_open: interval=%d\n", sed->interval));

		sed->hcchar_eptype = USBC_ENDPT_TYPE_INTR;
		s = splusb();
		octeon_usbc_add_ed(sed, sc->sc_intr_head);
		splx(s);
		return USBD_NORMAL_COMPLETION;
	case UE_BULK:
		DPRINTF(("%s: open bulk pipe at addr#%d endpoint#%d(%s).\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum,
				is_read ? "IN" : "OUT"));

		err = usb_allocmem(&sc->sc_bus,
			  USBC_BULK_TERMINATE_BUF_SIZE,
			  0, &usbc_pipe->u.bulk.term_dma);
		if (err)
			goto bad;

		pipe->methods = &octeon_usbc_device_bulk_methods;
		sed->hcchar_eptype = USBC_ENDPT_TYPE_BULK;
		s = splusb();
		octeon_usbc_add_ed(sed, sc->sc_bulk_head);
		splx(s);
		break;
	case UE_ISOCHRONOUS:
		DPRINTF(("%s: open isochronous pipe"
				" at addr#%d endpoint #%d(%s).\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum,
				is_read ? "IN" : "OUT"));

#if 0
		DPRINTF(("octeon_usbc_open: UE_ISOCHRONOUS\n"));
		pipe->methods = &octeon_usbc_device_isoc_methods;
		return (octeon_usbc_setup_isoc(pipe));
#else
		panic("octeon_usbc_open: UE_ISOCHRONOUS "
				"there is no implementation\n");
		break;
#endif
	default:
		panic("unknown endpt type"); /* XXX */
	}
	return (USBD_NORMAL_COMPLETION);

	/* TODO more code ? */
 bad:
	if (std != NULL)
		octeon_usbc_free_soft_td(sc, std);
 bad1:
	if (sed != NULL)
		octeon_usbc_free_soft_ed(sc, sed);
 bad0:
	return (USBD_NOMEM);
}

#ifdef USBC_DEBUG
static void
octeon_usbc_print_device_close_log(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	int is_read
		= UE_GET_DIR(pipe->endpoint->edesc->bEndpointAddress) == UE_DIR_IN;
	char *ep_type_name;
	
	switch (sed->hcchar_eptype) {
	case USBC_ENDPT_TYPE_CTRL:
		ep_type_name = "control";
		break;
	case USBC_ENDPT_TYPE_BULK:
		ep_type_name = "bulk";
		break;
	case USBC_ENDPT_TYPE_INTR:
		ep_type_name = "interrupt";
		break;
	case USBC_ENDPT_TYPE_ISOC:
		ep_type_name = "isochronous";
		break;
	default:
		ep_type_name = "unknown";
	}

	DPRINTF(("%s: close %s pipe at addr#%d endpoint#%d(%s).\n",
			device_xname(sc->sc_dev), ep_type_name,
			usbc_pipe->sed->hcchar_devaddr,
			usbc_pipe->sed->hcchar_epnum,
			is_read ? "IN" : "OUT"));
}
#endif

/*
 * Close a reqular pipe.
 * Assumes that there are no pending transactions.
 */
static void
octeon_usbc_close_pipe(usbd_pipe_handle pipe, octeon_usbc_soft_ed_t *head)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	int s;
#ifdef USBC_DEBUG
	if(usbc_debug)
		octeon_usbc_print_device_close_log(pipe);
#endif
	s = splusb();
#ifdef USBC_DEBUG
	if (usbc_debug > 5)
		usbd_dump_pipe(&usbc_pipe->pipe);
#endif
	octeon_usbc_rem_ed(sed, head);
	splx(s);
	octeon_usbc_free_soft_ed(sc, usbc_pipe->sed);
}

/*
 * Abort a device request.
 * If this routine is called at splusb() it guarantees that the request
 * will be removed from the hardware scheduling and that the callback
 * for it will be called with USBD_CANCELLED status.
 * It's impossible to guarantee that the requested transfer will not
 * have happened since the hardware runs concurrently.
 * If the transaction has already happened we rely on the ordinary
 * interrupt processing to process it.
 */
static void
octeon_usbc_abort_xfer(usbd_xfer_handle xfer, usbd_status status)
{
/*        DPRINTF(("octeon_usbc_abort_xfer: enter XXX\n"));*/
	/* XXX */

	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)xfer->pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)usbc_pipe->pipe.device->bus;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	octeon_usbc_soft_td_t *std;
	int s, sh;
	int wake;
	DPRINTFN(2,("octeon_usbc_abort_xfer: xfer=%p pipe=%p sed=%p\n",
				xfer, usbc_pipe, sed));

#ifdef USBC_DEBUG
#ifdef USBN_DEBUG
	if (usbc_debug > 15)
		octeon_usbn_debug_dumpregs(sc);
#endif
	if (usbc_debug > 15)
		octeon_usbc_debug_dumpregs(sc);
#endif

	if (sc->sc_dying) {
		/* If we're dying, just do the software part. */
		s = splusb();
		sed->status = USBC_ENDPT_STATUS_READY;
		xfer->status = status;	/* make software ignore it */
		callout_stop(&xfer->timeout_handle);
		usb_transfer_complete(xfer);
		splx(s);
		return;
	}

	if (cpu_intr_p() || cpu_softintr_p())
		panic("octeon_usbc_abort_xfer: not in process context");

	/*
	 * If an abort is already in progress then just wait for it to
	 * complete and return.
	 */
	if (xfer->hcflags & UXFER_ABORTING) {
		DPRINTFN(2, ("octeon_usbc_abort_xfer: already aborting\n"));
#ifdef DIAGNOSTIC
		if (status == USBD_TIMEOUT)
			aprint_error("octeon_usbc_abort_xfer: TIMEOUT while aborting\n");
#endif
		/* Override the status which might be USBD_TIMEOUT. */
		xfer->status = status;
		DPRINTFN(2, ("octeon_usbc_abort_xfer: waiting for abort to finish\n"));
		xfer->hcflags |= UXFER_ABORTWAIT;
		while (xfer->hcflags & UXFER_ABORTING)
			tsleep(&xfer->hcflags, PZERO, "usbcaw", 0);
		return;
	}
	xfer->hcflags |= UXFER_ABORTING;

	/*
	 * Step 1: Make interrupt routine and hardware ignore xfer.
	 */
	s = splusb();
	xfer->status = status;	/* make software ignore it */
	DPRINTFN(5,("octeon_usbc_abort_xfer: uncallout xfer=%p\n", xfer)); 
	callout_stop(&xfer->timeout_handle);
	DPRINTFN(5,("octeon_usbc_abort_xfer: stop ed=%p\n", sed));
	sh = splhardusb();
	if (sed->hostch)
		octeon_usbc_abort_host_channel(sc, sed->hostch);
	sed->status = USBC_ENDPT_STATUS_SKIP;
	splx(sh);
	sed->std_head = octeon_usbc_drop_soft_td(sc, sed->std_head, xfer);
	splx(s);

	/*
	 * Step 2: Wait until we know hardware has finished any possible
	 * use of the xfer.  Also make sure the soft interrupt routine
	 * has run.
	 */
#if 0
	usb_delay_ms(usbc_pipe->pipe.device->bus, 20); /* Hardware finishes in 1ms */
	s = splusb();
#ifdef USB_USE_SOFTINTR
	sc->sc_softwake = 1;
#endif /* USB_USE_SOFTINTR */
	usb_schedsoftintr(&sc->sc_bus);
#ifdef USB_USE_SOFTINTR
	tsleep(&sc->sc_softwake, PZERO, "usbcab", 0);
#endif /* USB_USE_SOFTINTR */
	splx(s);
#endif

	/*
	 * Step 3: Remove any vestiges of the xfer from the hardware.
	 * The complication here is that the hardware may have executed
	 * beyond the xfer we're trying to abort.  So as we're scanning
	 * the TDs of this xfer we check if the hardware points to
	 * any of them.
	 */
	s = splusb();		/* XXX why? */
	std = xfer->hcpriv;
#ifdef DIAGNOSTIC
	if (std == NULL) {
		xfer->hcflags &= ~UXFER_ABORTING; /* XXX */
		splx(s);
		aprint_error("octeon_usbc_abort_xfer: hcpriv is NULL\n");
		return;
	}
#endif

	if (sed->hostch)
		sed->hostch->std_done_head = octeon_usbc_drop_soft_td(sc,
				sed->hostch->std_done_head, xfer);

	/*
	 * Step 4: Turn on hardware again.
	 */
	sed->status = USBC_ENDPT_STATUS_READY;

	/*
	 * Step 5: Execute callback.
	 */
	wake = xfer->hcflags & UXFER_ABORTWAIT;
	xfer->hcflags &= ~(UXFER_ABORTING | UXFER_ABORTWAIT);
	usb_transfer_complete(xfer);
	if (wake)
		wakeup(&xfer->hcflags);

	splx(s);
}

/*
 * Wait here until controller claims to have an interrupt.
 * Then call octeon_usbc_intr and return.  Use timeout to avoid waiting
 * too long.
 */
static void
octeon_usbc_waitintr(octeon_usbc_softc_t *sc, usbd_xfer_handle xfer)
{
	int timo = xfer->timeout;
	int usecs;
	u_int32_t intrs;

	DPRINTFN(15, ("octeon_usbc_waitintr: enter\n"));

	xfer->status = USBD_IN_PROGRESS;
	for (usecs = timo * 1000000 / hz; usecs > 0; usecs -= 1000) {
		usb_delay_ms(&sc->sc_bus, 1);
		if (sc->sc_dying)
			break;
		intrs = octeon_usbc_reg_rd(sc, USBC_HAINT_OFFSET);
		DPRINTFN(15,("ohci_waitintr: 0x%04x\n", intrs));
#ifdef USBC_DEBUG
		if (usbc_debug > 15) {
			octeon_usbc_debug_reg_dump(sc, USBC_HAINT_OFFSET);
			octeon_usbc_debug_reg_dump(sc, USBC_HAINTMSK_OFFSET);
		}
#endif
		if (intrs) {
			octeon_usbc_intr(sc);
			if (xfer->status != USBD_IN_PROGRESS)
				return;
		}
	}

	/* Timeout */
	DPRINTFN(15,("octeon_usbc_waitintr: timeout\n"));
	xfer->status = USBD_TIMEOUT;
	usb_transfer_complete(xfer);
}

static void
octeon_usbc_poll(struct usbd_bus *bus)
{
	DPRINTFN(15, ("octeon_usbc_poll: enter, do nothing\n"));
}

void
octeon_usbc_abort_task(void *addr)
{
	usbd_xfer_handle xfer = addr;
	int s;

	DPRINTFN(10, ("octeon_usbc_abort_task: xfer=%p\n", xfer));

	s = splusb();
	octeon_usbc_abort_xfer(xfer, USBD_CANCELLED);
	splx(s);
}
void
octeon_usbc_timeout_task(void *addr)
{
	usbd_xfer_handle xfer = addr;
	int s;

	DPRINTFN(10, ("octeon_usbc_timeout_task: xfer=%p\n", xfer));

	s = splusb();
	octeon_usbc_abort_xfer(xfer, USBD_TIMEOUT);
	splx(s);
}

void
octeon_usbc_timeout(void *addr)
{
	struct octeon_usbc_xfer *usbc_xfer = addr;
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)usbc_xfer->xfer.pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)usbc_pipe->pipe.device->bus;

	DPRINTFN(5, ("octeon_usbc_timeout: usbc_xfer=%p\n", usbc_xfer));
#ifdef USBC_DEBUG
	if (usbc_debug > 5)
		usbd_dump_pipe(usbc_xfer->xfer.pipe);
#endif

	if (sc->sc_dying) {
		octeon_usbc_abort_xfer(&usbc_xfer->xfer, USBD_TIMEOUT);
		return;
	}

	/* Execute the abort in a process context. */
	usb_init_task(&usbc_xfer->abort_task, octeon_usbc_timeout_task, addr, 0);
	usb_add_task(usbc_xfer->xfer.pipe->device, &usbc_xfer->abort_task, USB_TASKQ_HC);
}

#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
static usbd_status
octeon_usbc_write_packet_into_host_channel(
		octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch, u_int32_t fifo_avail, u_int32_t *actlen)
{
	octeon_usbc_soft_td_t *std = hostch->std_cur;
	octeon_usbc_soft_ed_t *sed = hostch->sed;
	u_int32_t *buff = (u_int32_t *)KERNADDR(std->buff,
			std->base_offset + std->offset);
	bus_size_t fifo = hostch->offset_usbc_dfifo;
	u_int32_t remain_len;
	u_int32_t byte_cnt, dword_cnt;

	u_int32_t hctsiz = octeon_usbc_reg_rd(sc, hostch->offset_usbc_hctsiz);
	u_int32_t remain_pktcnt = (hctsiz  & USBC_HCTSIZX_PKTCNT)
				>> USBC_HCTSIZX_PKTCNT_OFFSET;
	int i;

	KASSERT(std->direction == USBC_ENDPT_DIRECTION_OUT);

	if (std->xfer->status != USBD_IN_PROGRESS) {
		log(LOG_WARNING, "%s: The xfer is not in progress. Discard it. (%s)\n",
				device_xname(sc->sc_dev),
				usbd_errstr(std->xfer->status));
		return USBD_INVAL;
	}

	remain_len = std->len - std->offset; /* XXX */
	byte_cnt = (remain_len < sed->hcchar_mps) ? remain_len : sed->hcchar_mps;
	dword_cnt = (byte_cnt + 3) / 4;

	if (byte_cnt > fifo_avail) {
		log(LOG_DEBUG, "%s:Tx FIFO has no space\n",
				device_xname(sc->sc_dev));
		return USBD_INVAL;
	}

	DPRINTFN(4,("%s: remain_len=%d, byte_cnt=%d, remain_pktcnt=%d, hc_num=%d\n",
				__func__, remain_len, byte_cnt, remain_pktcnt, hostch->hc_num));


	/* FIXME Who assure the buffer is aligned ? */
	KASSERT(ALIGNED_POINTER(buff, u_int32_t));

	for (i = 0; i < dword_cnt; i++, buff++)
		octeon_usbc_reg_wr(sc, fifo, *buff);

	*actlen = byte_cnt;
	std->offset += *actlen;
	OCTEON_SYNC;

	if (remain_pktcnt > 1) {
		return USBD_IN_PROGRESS;
	} else
		/* This is a Last Packet */
		return USBD_NORMAL_COMPLETION;
}

static usbd_status
octeon_usbc_read_packet_from_host_channel(octeon_usbc_softc_t *sc,
		octeon_usbc_hostch_t *hostch, u_int16_t byte_cnt)
{
	octeon_usbc_soft_td_t *std = hostch->std_cur;
	u_int32_t *buff = (u_int32_t *)KERNADDR(std->buff,
			std->base_offset + std->offset);
	bus_size_t fifo = hostch->offset_usbc_dfifo;
	u_int32_t dword_cnt;
	int i;

	KASSERT(std->direction == USBC_ENDPT_DIRECTION_IN);

	dword_cnt = (byte_cnt + 3) / 4;

	DPRINTFN(4,("%s: dword_cnt=%d, byte_cnt=%d\n",
				__func__, dword_cnt, byte_cnt));

	for (i = 0; i < dword_cnt; i++, buff++)
		*buff = octeon_usbc_reg_rd(sc, fifo);

	std->offset += byte_cnt;
	OCTEON_SYNC;

	DCOUNTUP(usbc_recv_pkts);

	return USBD_NORMAL_COMPLETION;
}
#endif

static inline int
octeon_usbc_is_root_port_availe(octeon_usbc_softc_t *sc)
{
	u_int32_t hprt = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
	u_int32_t flgs = USBC_HPRT_PRTCONNSTS | USBC_HPRT_PRTENA;

	return  (hprt & flgs) == flgs;
}

static usbd_status
octeon_usbc_set_next_transact(octeon_usbc_softc_t *sc, octeon_usbc_soft_ed_t *sed)
{
	octeon_usbc_hostch_t *hostch = NULL;
	octeon_usbc_soft_td_t *std;
	int error = USBD_NORMAL_COMPLETION;
	int i;
	int s;

	if (!octeon_usbc_is_root_port_availe(sc)) {
		log(LOG_DEBUG, "%s: There is no available device on root hub"
				"(addr#%d,ep#%d)\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum);
		error = USBD_IOERROR;
		goto err;
	}

	for (std = sed->std_head; std != NULL && std->flags & USBC_STD_FLAG_SKIP;
			std = std->next)
		DPRINTF(("%s: skip std=%p", __func__, std));
	if (std == NULL) {
		log(LOG_WARNING, "%s: There is no valid transaction on USB pipe."
				"(ADDR=%d, EP=%d)\n", device_xname(sc->sc_dev),
					sed->hcchar_devaddr, sed->hcchar_epnum);
		error = USBD_INVAL;
		goto err;
	}

	for (i=0; i < 10; i ++){
		s = splhardusb();
		hostch = octeon_usbc_alloc_host_channel(sc);
		if( hostch )
			break; /* XXX block hard intr during hostch setup */
		splx(s);
		usb_delay_ms(&sc->sc_bus, 5);
	}

	if (hostch == NULL) {
		error = USBD_NOMEM;
		splx(s);
		goto err;
	}
	DPRINTF(("%s: alloc ch.%d sed=%p std=%p xfer=%p\n", __func__,
			hostch->hc_num, sed, std, std->xfer));

	hostch->sed = sed;
	sed->hostch = hostch;

	hostch->std_cur = std; 
	sed->std_head = std->next;
	std->next = NULL;

	splx(s);

#ifdef USBC_DEBUG
	if (usbc_skip_setnext) {
		logprintf("%s: DEBUG: skip set_next_transact. (%d)\n", __func__, usbc_skip_setnext);
		usbc_skip_setnext--;
		return USBD_IN_PROGRESS;
	}
#endif

	error = octeon_usbc_set_transact(sc, hostch);
	if (error != USBD_NORMAL_COMPLETION)
		goto err;

	return USBD_IN_PROGRESS;

err:
	DPRINTF(("%s: fail setting next transaction.\n", __func__));
	if (hostch != NULL)
		octeon_usbc_free_host_channel(sc, hostch, USBD_NOMEM);
	return error;
}

static usbd_status
octeon_usbc_set_transact(octeon_usbc_softc_t *sc, octeon_usbc_hostch_t *hostch)
{
	octeon_usbc_soft_ed_t *sed;
	octeon_usbc_soft_td_t *std;
	u_int32_t pktcnt = 0;
	int xfersize;
	int ping_state;
	int s;
#if 0 
	usb_dma_t tmp_dma_buff;
#endif

	DPRINTFN(15, ("octeon_usbc_set_transact: enter\n"));

	sed = hostch->sed;
	std = hostch->std_cur;
	ping_state = sed->ping_state;
	xfersize = (std->len > std->offset) ? std->len - std->offset : 0;
	hostch->hctsiz = hostch->hcchar = 0;
	hostch->error_state = (std->error_count > 0) ? 1 : 0;

	DPRINTFN(15, ("octeon_usbc_set_transact: head=%p next=%p cur=%p\n",
			sed->std_head, sed->std_head->next, hostch->std_cur));

	if (xfersize > 0)//|| std->direction == USBC_ENDPT_DIRECTION_IN)
		pktcnt = (xfersize + (sed->hcchar_mps - 1)) / sed->hcchar_mps;
	else
		pktcnt = 1;

#if 0
	if(std->direction == USBC_ENDPT_DIRECTION_OUT){
		usb_allocmem(&sc->sc_bus, sizeof(usb_device_request_t), 0, &tmp_dma_buff);
		octeon_usbn_set_phyaddr(sc, &tmp_dma_buff, hostch->hc_num, std->direction);
	}else
#endif

	if (sc->sc_dma_enable)
		octeon_usbn_set_phyaddr(sc, std->buff, hostch->hc_num, std->direction,
				std->offset + std->base_offset);

	hostch->hcchar = 0;
	hostch->hcchar |= (1U << USBC_HCCHARX_EC_OFFSET); /* FIXME for periodic xfer */
	hostch->hcchar |= (sed->hcchar_devaddr << USBC_HCCHARX_DEVADDR_OFFSET)
			& USBC_HCCHARX_DEVADDR;
	hostch->hcchar |= (sed->hcchar_epnum << USBC_HCCHARX_EPNUM_OFFSET)
			& USBC_HCCHARX_EPNUM;
	switch (sed->hcchar_eptype) {
	case USBC_ENDPT_TYPE_CTRL:
		hostch->hcchar |= USBC_HCCHARX_EPTYPE_CONTROL
				<< USBC_HCCHARX_EPTYPE_OFFSET;
		if (std->direction == USBC_ENDPT_DIRECTION_IN
				|| std->pid == USBC_DATA_TGL_SETUP)
			ping_state = 0;
		break;
	case USBC_ENDPT_TYPE_ISOC:
		hostch->hcchar |= USBC_HCCHARX_EPTYPE_ISOCHRONOUS
				<< USBC_HCCHARX_EPTYPE_OFFSET;
		if ((sc->sc_frame_number & 0x1) ? 0 : 1)
			hostch->hcchar |= USBC_HCCHARX_ODDFRM;
			octeon_usbc_reg_assert(sc, sed->hostch->offset_usbc_hcchar,
					USBC_HCCHARX_ODDFRM);
		break;
	case USBC_ENDPT_TYPE_BULK:
		hostch->hcchar |= USBC_HCCHARX_EPTYPE_BULK
				<< USBC_HCCHARX_EPTYPE_OFFSET;
		break;
	case USBC_ENDPT_TYPE_INTR:
		hostch->hcchar |= USBC_HCCHARX_EPTYPE_INTERRUPT
				<< USBC_HCCHARX_EPTYPE_OFFSET;
		if ((sc->sc_frame_number & 0x1) ? 0 : 1)
			hostch->hcchar |= USBC_HCCHARX_ODDFRM;
			octeon_usbc_reg_assert(sc, sed->hostch->offset_usbc_hcchar,
					USBC_HCCHARX_ODDFRM);
		break;
	default:
		panic("unknown endpt type"); /* XXX */
		break;
	}
	if (std->direction == USBC_ENDPT_DIRECTION_IN)
		hostch->hcchar |= USBC_HCCHARX_EPDIR;
	hostch->hcchar |= (sed->hcchar_mps << USBC_HCCHARX_MPS_OFFSET)
			& USBC_HCCHARX_MPS;
	if (sc->sc_port_speed == USBC_HPRT_PRTSPD_LOW)
		hostch->hcchar |= USBC_HCCHARX_LSPDDEV;

	hostch->hctsiz = 0;
	if (ping_state && std->xfer->pipe->device->speed == USB_SPEED_HIGH )
			hostch->hctsiz |= USBC_HCTSIZX_DOPNG;

	if (std->pid == USBC_DATA_TGL_INHERIT)
		std->pid = sed->toggle_state;

	switch (std->pid) {
	case USBC_DATA_TGL_DATA0:
		hostch->hctsiz |= USBC_HCTSIZX_PID_DATA0
				<< USBC_HCTSIZX_PID_OFFSET;
		break;
	case USBC_DATA_TGL_DATA1:
		hostch->hctsiz |= USBC_HCTSIZX_PID_DATA1
				<< USBC_HCTSIZX_PID_OFFSET;
		break;
	case USBC_DATA_TGL_DATA2:
		hostch->hctsiz |= USBC_HCTSIZX_PID_DATA2
				<< USBC_HCTSIZX_PID_OFFSET;
		break;
	case USBC_DATA_TGL_MDATA:
	case USBC_DATA_TGL_SETUP:
		hostch->hctsiz |= USBC_HCTSIZX_PID_MDATA_SETUP
				<< USBC_HCTSIZX_PID_OFFSET;
		break;
	default:
		panic("unknown pid type"); /* XXX */
		break;
	}

	hostch->hctsiz |= (pktcnt << USBC_HCTSIZX_PKTCNT_OFFSET)
			& USBC_HCTSIZX_PKTCNT;
	hostch->hctsiz |= (xfersize << USBC_HCTSIZX_XFERSIZE_OFFSET)
			& USBC_HCTSIZX_XFERSIZE;

#if defined(USBC_DEBUG) && 1
	if (usbc_debug > 5){
		if (std->direction == USBC_ENDPT_DIRECTION_IN) {
			DPRINTFN(10, ("octeon_usbc_set_transact: is IN packet\n"));
		} else if (xfersize == 0) {
			DPRINTFN(5, ("octeon_usbc_set_transact: is zerosize packet\n"));
		} else {
			/* debugdump */
			int i;
#if 0
			unsigned char *c = KERNADDR(&tmp_dma_buff, 0);
			memcpy(KERNADDR(&tmp_dma_buff, 0),
					KERNADDR(std->buff, 0), xfersize);
#else
			unsigned char *c = KERNADDR(std->buff, std->base_offset);
#endif
			DPRINTFN(5, ("%s: ADDR=%d:EP=%d(type=%d)    : Send DATA= ",
					device_xname(sc->sc_dev),
					hostch->std_cur->sed->hcchar_devaddr,
					std->sed->hcchar_epnum,
					std->sed->hcchar_eptype));
			for(i=0; i < xfersize; i++)
				DPRINTFN(5, ("%02x ", c[i]));
			DPRINTFN(5, ("\n"));
		}
	}
#endif

	octeon_usbc_setup_host_channel(sc, sed->hostch);
	OCTEON_SYNC;

	s = splhardusb();
	if (sed->hcchar_eptype == USBC_ENDPT_TYPE_INTR
			&& sed->interval > 1) {
		sed->interval_count = sed->interval;
		sed->status = USBC_ENDPT_STATUS_WAIT_NEXT_SOF;
	}

	if (sed->status == USBC_ENDPT_STATUS_READY
			|| sed->status == USBC_ENDPT_STATUS_IN_PROGRESS) {
		if (std->xfer->status != USBD_IN_PROGRESS) {
			splx(s);
			return USBD_IOERROR; /* XXX */
		}
		sed->status = USBC_ENDPT_STATUS_IN_PROGRESS;
		octeon_usbc_enable_host_channel(sc, sed->hostch);
	}
	splx(s);

	return USBD_NORMAL_COMPLETION;
}

static usbd_status
octeon_usbc_device_request(usbd_xfer_handle xfer)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)xfer->pipe;
	usb_device_request_t *req = &xfer->request;
	usbd_device_handle dev = usbc_pipe->pipe.device;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)dev->bus;
	/* setup stage, status stage */
	octeon_usbc_soft_td_t *setup, *stat, *next, *tail, *data = NULL;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	int addr = dev->address;
	int is_read;
	int len;
	usbd_status err = 0;
	int s;

	len = UGETW(req->wLength);
	is_read = (len > 0) ? req->bmRequestType & UT_READ: 0;

	DPRINTFN(2,("octeon_usbc_device_request pipe=%p, type=0x%02x, request=0x%02x, "
		    "wValue=0x%04x, wIndex=0x%04x len=%d, addr=%d, endpt=%d\n",
		    usbc_pipe, req->bmRequestType, req->bRequest, UGETW(req->wValue),
		    UGETW(req->wIndex), len, addr,
		    usbc_pipe->pipe.endpoint->edesc->bEndpointAddress));

	sed = usbc_pipe->sed;
	sed->hcchar_devaddr = addr;
	sed->hcchar_mps = UGETW(usbc_pipe->pipe.endpoint->edesc->wMaxPacketSize);
	if(sed->hcchar_eptype != USBC_ENDPT_TYPE_CTRL)
		panic("sed->hcchar_eptype != USBC_ENDPT_TYPE_CTRL");

	/* Refuse new transfer during disconnecting */
	if (sed->status == USBC_ENDPT_STATUS_SKIP) {
		log(LOG_DEBUG, "%s: addr#%d ep#%d is unavailable\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum);
		err = USBD_IOERROR;
		goto bad1;
	}

	setup = usbc_pipe->tail.std;
	stat = octeon_usbc_alloc_soft_td(sc);
	if (stat == NULL) {
		err = USBD_NOMEM;
		goto bad1;
	}
	tail = octeon_usbc_alloc_soft_td(sc);
	if (tail == NULL) {
		err = USBD_NOMEM;
		goto bad2;
	}
	tail->xfer = NULL;

	usbc_pipe->u.ctl.length = len;

	next = stat;

	if (len > 0) {
		if (len > 512*1024) {
			DPRINTF(("octeon_usbc_device_request: data packet 512kb over!!\b"));
			goto bad3;
		} else {
			data = octeon_usbc_alloc_soft_td(sc);
			if (data == NULL) {
				err = USBD_NOMEM;
				goto bad3;
			}

			next = data;
			data->next = stat;
			data->direction = is_read ? USBC_ENDPT_DIRECTION_IN
					: USBC_ENDPT_DIRECTION_OUT;
			data->len = len;
			data->buff = &xfer->dmabuf;
			data->pid = USBC_DATA_TGL_DATA1;
			data->xfer = xfer;
			data->flags = 0;
			data->sed = sed;
		}
	}
#ifdef USBC_DEBUG
	else
		DPRINTFN(5, ("octeon_usbc_device_request: no data stage\n"));
#endif

	memcpy(KERNADDR(&usbc_pipe->u.ctl.req_setup_dma, 0), req, sizeof(*req));

	/* clear out memcpied data from Octeon write buffer. */
	OCTEON_SYNC;

	setup->next = next;
	setup->direction = USBC_ENDPT_DIRECTION_OUT;
	setup->len = 8;
	setup->buff = &usbc_pipe->u.ctl.req_setup_dma;
	setup->pid = USBC_DATA_TGL_SETUP;
	setup->xfer = xfer;
	setup->flags = 0;
	setup->sed = sed;

	stat->next = tail;
	stat->direction = is_read ? USBC_ENDPT_DIRECTION_OUT
				: USBC_ENDPT_DIRECTION_IN;
	stat->len = 0;
	stat->buff = &usbc_pipe->u.ctl.req_stat_dma;
	stat->pid = USBC_DATA_TGL_DATA1;
	stat->xfer = xfer;
	stat->flags = USBC_STD_FLAG_CALL_DONE;
	stat->sed = sed;

	xfer->hcpriv = setup;
	sed->std_head = setup;

	s = splusb();
	usbc_pipe->tail.std = tail;
	if (xfer->timeout && !sc->sc_bus.use_polling) {
		DPRINTFN(5,("octeon_usbc_device_request: callout xfer=%p\n", xfer)); 
                callout_reset(&xfer->timeout_handle, mstohz(xfer->timeout),
			    octeon_usbc_timeout, xfer);
	}
	xfer->status = USBD_IN_PROGRESS;


/*        DPRINTF(("octeon_usbc_device_request: "*/
/*                                "mark ED as Ready for next TD\n"));*/
	sed->status = USBC_ENDPT_STATUS_READY;
	err = octeon_usbc_set_next_transact(sc, sed);
	if (err != USBD_IN_PROGRESS) {
		DPRINTF(("%s: fault set_next_transact() (%s)\n",
				__func__, usbd_errstr(err)));
		callout_stop(&xfer->timeout_handle);
		sed->std_head = octeon_usbc_drop_soft_td(sc,
				sed->std_head, xfer);
		splx(s);
		goto bad1;
	}
	splx(s);

	return (USBD_NORMAL_COMPLETION);


 bad3:
	octeon_usbc_free_soft_td(sc, tail);
 bad2:
	octeon_usbc_free_soft_td(sc, stat);
 bad1:
	s = splusb();
	xfer->status = err;
	usb_transfer_complete(xfer);
	splx(s);
	return (err);
}

static usbd_status
octeon_usbc_allocm(struct usbd_bus *bus, usb_dma_t *dma, u_int32_t size)
{
#if defined(__NetBSD__) || defined(__OpenBSD__)
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)bus;
#endif
	usbd_status status;

/*        DPRINTFN(15, ("octeon_usbc_allocm: enter\n"));*/
	status = usb_allocmem(&sc->sc_bus, size, 0, dma);
#ifdef __NetBSD__
	if (status == USBD_NOMEM)
		status = usb_reserve_allocm(&sc->sc_dma_reserve, dma, size);
#endif
	return status;
}

static void
octeon_usbc_freem(struct usbd_bus *bus, usb_dma_t *dma)
{
#if defined(__NetBSD__) || defined(__OpenBSD__)
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)bus;
#endif
/*        DPRINTFN(15, ("octeon_usbc_freem: enter\n"));*/
#ifdef __NetBSD__
	if (dma->block->flags & USB_DMA_RESERVE) {
		usb_reserve_freem(&((octeon_usbc_softc_t *)bus)->sc_dma_reserve,
		    dma);
		return;
	}
#endif
	usb_freemem(&sc->sc_bus, dma);
}

static usbd_xfer_handle
octeon_usbc_allocx(struct usbd_bus *bus)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)bus;
	struct octeon_usbc_xfer *xfer;

	DPRINTFN(15, ("octeon_usbc_allocx: enter\n"));

	xfer = (struct octeon_usbc_xfer *)SIMPLEQ_FIRST(&sc->sc_free_xfers);
	if (xfer != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_free_xfers, next);
#if defined(DIAGNOSTIC)
		if (((usbd_xfer_handle)xfer)->busy_free != XFER_FREE) {
			printf("octeon_usbc_allocx: xfer=%p not free, 0x%08x\n", xfer,
			       ((usbd_xfer_handle)xfer)->busy_free);
		}
#endif
	} else {
		xfer = malloc(sizeof(struct octeon_usbc_xfer), M_USB, M_NOWAIT);
		DCOUNTUP(usbc_pool_xfer);
	}
	if (xfer != NULL) {
		/**
		 * Fortify Warnning
		 * Buffer Overflow (Input Validation and Representation, Semantic)
		 */
		memset(xfer, 0, sizeof (struct octeon_usbc_xfer));
#if defined(DIAGNOSTIC)
		((usbd_xfer_handle)xfer)->busy_free = XFER_BUSY;
#endif
	}
	DCOUNTUP(usbc_used_xfer);
	DPRINTFN(2, ("octeon_usbc_allocx: %p\n", xfer));

	return (usbd_xfer_handle)xfer;
}

static void
octeon_usbc_freex(struct usbd_bus *bus, usbd_xfer_handle xfer)
{
	struct octeon_usbc_softc *sc = (struct octeon_usbc_softc *)bus;

/*        DPRINTFN(15, ("octeon_usbc_freex: enter\n"));*/
	DPRINTFN(2, ("octeon_usbc_freex: %p\n", xfer));

#if defined(DIAGNOSTIC)
	if (xfer->busy_free != XFER_BUSY) {
		printf("octeon_usbc_freex: xfer=%p not busy, 0x%08x\n", xfer,
		       xfer->busy_free);
		return;
	}
	xfer->busy_free = XFER_FREE;
#endif
	SIMPLEQ_INSERT_HEAD(&sc->sc_free_xfers, xfer, next);
	DCOUNTDOWN(usbc_used_xfer);
}


/*
 * Data structures and routines to emulate the root hub.
 */
static usb_device_descriptor_t octeon_usbc_devd = {
	USB_DEVICE_DESCRIPTOR_SIZE,
	UDESC_DEVICE,		/* type */
	{0x00, 0x02},		/* USB version */
	UDCLASS_HUB,		/* class */
	UDSUBCLASS_HUB,		/* subclass */
	UDPROTO_HSHUBSTT,	/* protocol */
	64,			/* max packet */
	{0},{0},{0x00,0x01},	/* device id */
	1,2,0,			/* string indicies */
	1			/* # of configurations */
};

static usb_device_qualifier_t octeon_usbc_odevd = {
	USB_DEVICE_DESCRIPTOR_SIZE,
	UDESC_DEVICE_QUALIFIER,	/* type */
	{0x00, 0x02},		/* USB version */
	UDCLASS_HUB,		/* class */
	UDSUBCLASS_HUB,		/* subclass */
	UDPROTO_FSHUB,		/* protocol */
	64,			/* max packet */
	1,			/* # of configurations */
	0
};

static usb_config_descriptor_t octeon_usbc_confd = {
	USB_CONFIG_DESCRIPTOR_SIZE,
	UDESC_CONFIG,
	{USB_CONFIG_DESCRIPTOR_SIZE +
	 USB_INTERFACE_DESCRIPTOR_SIZE +
	 USB_ENDPOINT_DESCRIPTOR_SIZE},
	1,
	1,
	0,
	UC_SELF_POWERED,
	0			/* max power */
};

static usb_interface_descriptor_t octeon_usbc_ifcd = {
	USB_INTERFACE_DESCRIPTOR_SIZE,
	UDESC_INTERFACE,
	0,
	0,
	1,
	UICLASS_HUB,
	UISUBCLASS_HUB,
	UIPROTO_HSHUBSTT,
	0
};

static usb_endpoint_descriptor_t octeon_usbc_endpd = {
	USB_ENDPOINT_DESCRIPTOR_SIZE,
	UDESC_ENDPOINT,
	UE_DIR_IN | USBC_INTR_ENDPT,
	UE_INTERRUPT,
	{8, 0},			/* max packet */
	255
};

static usb_hub_descriptor_t octeon_usbc_hubd = {
	USB_HUB_DESCRIPTOR_SIZE,
	UDESC_HUB,
	0,
	{0,0},
	0,
	0,
	{0},
};

static int
octeon_usbc_str(usb_string_descriptor_t *p, int l, const char *s)
{
	int i;

	if (l == 0)
		return (0);
	p->bLength = 2 * strlen(s) + 2;
	if (l == 1)
		return (1);
	p->bDescriptorType = UDESC_STRING;
	l -= 2;
	for (i = 0; s[i] && l > 1; i++, l -= 2)
		USETW2(p->bString[i], 0, s[i]);
	return (2*i+2);
}

static usbd_status
octeon_usbc_root_ctrl_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	DPRINTFN(15, ("octeon_usbc_root_ctrl_transfer: enter\n"));

	/* Insert last in queue. */
	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (octeon_usbc_root_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static usbd_status
octeon_usbc_root_ctrl_start(usbd_xfer_handle xfer)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)xfer->pipe->device->bus;
	usb_device_request_t *req;
	void *buf = NULL;
	int port, i;
	int s, len, value, index, l, totlen = 0;
	usb_port_status_t ps;
	usb_hub_descriptor_t hubd;
	usbd_status err;
	u_int32_t v;


	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST))
		/* XXX panic */
		return (USBD_INVAL);
#endif
	req = &xfer->request;

	DPRINTFN(10,("octeon_usbc_root_ctrl_control type=0x%02x request=%02x, len=%d\n",
		    req->bmRequestType, req->bRequest, UGETW(req->wLength)));

	len = UGETW(req->wLength);
	value = UGETW(req->wValue);
	index = UGETW(req->wIndex);

	if (len != 0)
		buf = KERNADDR(&xfer->dmabuf, 0);

#define C(x,y) ((x) | ((y) << 8))
	switch(C(req->bRequest, req->bmRequestType)) {
	case C(UR_CLEAR_FEATURE, UT_WRITE_DEVICE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_INTERFACE):
	case C(UR_CLEAR_FEATURE, UT_WRITE_ENDPOINT):
		/*
		 * DEVICE_REMOTE_WAKEUP and ENDPOINT_HALT are no-ops
		 * for the integrated root hub.
		 */
		break;
	case C(UR_GET_CONFIG, UT_READ_DEVICE):
		if (len > 0) {
			*(u_int8_t *)buf = sc->sc_conf;
			totlen = 1;
		}
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_DEVICE):
		DPRINTFN(10,("octeon_usbc_root_ctrl_control wValue=0x%04x\n", value));
		switch(value >> 8) {
		case UDESC_DEVICE:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			USETW(octeon_usbc_devd.idVendor, sc->sc_id_vendor);
			memcpy(buf, &octeon_usbc_devd, l);
			break;
		/*
		 * We can't really operate at another speed, but the spec says
		 * we need this descriptor.
		 */
		case UDESC_DEVICE_QUALIFIER:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_DEVICE_DESCRIPTOR_SIZE);
			memcpy(buf, &octeon_usbc_odevd, l);
			break;
		/*
		 * We can't really operate at another speed, but the spec says
		 * we need this descriptor.
		 */
		case UDESC_OTHER_SPEED_CONFIGURATION:
		case UDESC_CONFIG:
			if ((value & 0xff) != 0) {
				err = USBD_IOERROR;
				goto ret;
			}
			totlen = l = min(len, USB_CONFIG_DESCRIPTOR_SIZE);
			memcpy(buf, &octeon_usbc_confd, l);
			((usb_config_descriptor_t *)buf)->bDescriptorType =
				value >> 8;
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_INTERFACE_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &octeon_usbc_ifcd, l);
			buf = (char *)buf + l;
			len -= l;
			l = min(len, USB_ENDPOINT_DESCRIPTOR_SIZE);
			totlen += l;
			memcpy(buf, &octeon_usbc_endpd, l);
			break;
		case UDESC_STRING:
			if (len == 0)
				break;
			*(u_int8_t *)buf = 0;
			totlen = 1;
			switch (value & 0xff) {
			case 0: /* Language table */
				totlen = octeon_usbc_str(buf, len, "\001");
				break;
			case 1: /* Vendor */
				totlen = octeon_usbc_str(buf, len, sc->sc_vendor);
				break;
			case 2: /* Product */
				totlen = octeon_usbc_str(buf, len, "octeon_usbc root hub");
				break;
			}
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
		break;
	case C(UR_GET_INTERFACE, UT_READ_INTERFACE):
		if (len > 0) {
			*(u_int8_t *)buf = 0;
			totlen = 1;
		}
		break;
	case C(UR_GET_STATUS, UT_READ_DEVICE):
		if (len > 1) {
			USETW(((usb_status_t *)buf)->wStatus,UDS_SELF_POWERED);
			totlen = 2;
		}
		break;
	case C(UR_GET_STATUS, UT_READ_INTERFACE):
	case C(UR_GET_STATUS, UT_READ_ENDPOINT):
		if (len > 1) {
			USETW(((usb_status_t *)buf)->wStatus, 0);
			totlen = 2;
		}
		break;
	case C(UR_SET_ADDRESS, UT_WRITE_DEVICE):
		if (value >= USB_MAX_DEVICES) {
			err = USBD_IOERROR;
			goto ret;
		}
		sc->sc_addr = value;
		break;
	case C(UR_SET_CONFIG, UT_WRITE_DEVICE):
		if (value != 0 && value != 1) {
			err = USBD_IOERROR;
			goto ret;
		}
		sc->sc_conf = value;
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_DEVICE):
	case C(UR_SET_FEATURE, UT_WRITE_INTERFACE):
	case C(UR_SET_FEATURE, UT_WRITE_ENDPOINT):
		err = USBD_IOERROR;
		goto ret;
	case C(UR_SET_INTERFACE, UT_WRITE_INTERFACE):
		break;
	case C(UR_SYNCH_FRAME, UT_WRITE_ENDPOINT):
		break;
	/* Hub requests */
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_CLEAR_FEATURE, UT_WRITE_CLASS_OTHER):
		DPRINTFN(10,("octeon_usbc_root_ctrl_control: UR_CLEAR_PORT_FEATURE "
			     "port=%d feature=%d\n",
			     index, value));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		port = USBC_HPRT_OFFSET; /* we have only one port */

		switch(value) {
		case UHF_PORT_ENABLE:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: clear enable\n"));
			octeon_usbc_reg_assert_msked(sc,USBC_HPRT_OFFSET,
					USBC_HPRT_PRTENA, USBC_HPRT_WR1CMSK);
			break;
		case UHF_PORT_SUSPEND:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: clear suspend\n"));
			octeon_usbc_reg_deassert_msked(sc,USBC_HPRT_OFFSET,
					USBC_HPRT_PRTSUSP, USBC_HPRT_WR1CMSK);
			break;
		case UHF_PORT_POWER:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: clear power\n"));
			s = splhardusb();
			octeon_usbc_reg_deassert_msked(sc,USBC_HPRT_OFFSET,
					USBC_HPRT_PRTPWR, USBC_HPRT_WR1CMSK);
			splx(s);
			break;
		case UHF_PORT_TEST:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: clear port test "
				    "%d\n", index));
			break;
		case UHF_PORT_INDICATOR:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: clear port ind "
				    "%d\n", index));
			/* XXX */
			break;
		case UHF_C_PORT_CONNECTION:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: port connected\n"));
			sc->sc_connstat_change = 0;
			break;
		case UHF_C_PORT_ENABLE:
			DPRINTFN(10,("octeon_usbc_root_ctrl_control: port enabled\n"));
			octeon_usbc_reg_assert_msked(sc,USBC_HPRT_OFFSET,
					USBC_HPRT_PRTENCHNG, USBC_HPRT_WR1CMSK);
			break;
		case UHF_C_PORT_SUSPEND:
			/* XXX */
			break;
		case UHF_C_PORT_OVER_CURRENT:
			octeon_usbc_reg_assert_msked(sc,USBC_HPRT_OFFSET,
					USBC_HPRT_PRTOVRCURRCHNG, USBC_HPRT_WR1CMSK);
			break;
		case UHF_C_PORT_RESET:
			log(LOG_CRIT, "%s: port 1 reseted\n", device_xname(sc->sc_dev));
			sc->sc_isreset = 0;
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET);
#endif
#if 0
		switch(value) {
		case UHF_C_PORT_ENABLE:
		case UHF_C_PORT_SUSPEND:
		case UHF_C_PORT_OVER_CURRENT:
		case UHF_C_PORT_RESET:
			/* Enable host port interrupt if condition is cleared. */
			if(sc->sc_isreset == 0);
				octeon_usbc_intr_host_port_enable(sc);
		case UHF_C_PORT_CONNECTION:
		default:
			break;
		}
#endif
		break;
	case C(UR_GET_DESCRIPTOR, UT_READ_CLASS_DEVICE):
		if ((value & 0xff) != 0) {
			err = USBD_IOERROR;
			goto ret;
		}
		v = octeon_usbc_reg_rd(sc, USBC_GHWCFG4_OFFSET);
		hubd = octeon_usbc_hubd;
		hubd.bNbrPorts = sc->sc_noport;
		/* XXX */
		USETW(hubd.wHubCharacteristics,
		      (v & USBC_GHWCFG4_ENABLEPWROPT ?
			 UHD_PWR_GANGED : UHD_PWR_NO_SWITCH));
		      /* XXX overcurrent */
		hubd.bPwrOn2PwrGood = 200; /* XXX */
		for (i = 0, l = sc->sc_noport; l > 0; i++, l -= 8, v >>= 8)
			hubd.DeviceRemovable[i++] = 0;
		hubd.bDescLength = USB_HUB_DESCRIPTOR_SIZE + i;
		l = min(len, hubd.bDescLength);
		totlen = l;
		memcpy(buf, &hubd, l);
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_DEVICE):
		if (len != 4) {
			err = USBD_IOERROR;
			goto ret;
		}
		memset(buf, 0, len); /* ? XXX */
		totlen = len;
		break;
	case C(UR_GET_STATUS, UT_READ_CLASS_OTHER):
		DPRINTFN(10,("octeon_usbc_root_ctrl_control: get port status i=%d\n",
			    index));
		if (index < 1 || index > sc->sc_noport) {
			err = USBD_IOERROR;
			goto ret;
		}
		if (len != 4) {
			err = USBD_IOERROR;
			goto ret;
		}
		v = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, USBC_HPRT_OFFSET);
#endif
		sc->sc_port_speed = (v & USBC_HPRT_PRTSPD) >> USBC_HPRT_PRTSPD_OFFSET;
		switch (sc->sc_port_speed) {
		case USBC_HPRT_PRTSPD_HIGH:
			i = UPS_HIGH_SPEED;
			break;
		case USBC_HPRT_PRTSPD_LOW:
			i = UPS_LOW_SPEED;
			break;
		case USBC_HPRT_PRTSPD_FULL:
		default:
			i = 0;
		}

		if (v & USBC_HPRT_PRTCONNSTS)
			i |= UPS_CURRENT_CONNECT_STATUS;
		if (v & USBC_HPRT_PRTENA)	i |= UPS_PORT_ENABLED;
		if (v & USBC_HPRT_PRTSUSP)	i |= UPS_SUSPEND;
		if (v & USBC_HPRT_PRTOVRCURRACT)
			i |= UPS_OVERCURRENT_INDICATOR;
		if (v & USBC_HPRT_PRTRST)	i |= UPS_RESET;
		if (v & USBC_HPRT_PRTPWR)	i |= UPS_PORT_POWER;
		USETW(ps.wPortStatus, i);
		i = 0;
		if (sc->sc_connstat_change)	i |= UPS_C_CONNECT_STATUS;
		if (v & USBC_HPRT_PRTENCHNG)	i |= UPS_C_PORT_ENABLED;
		if (v & USBC_HPRT_PRTOVRCURRCHNG)
			i |= UPS_C_OVERCURRENT_INDICATOR;
		if (sc->sc_isreset)		i |= UPS_C_PORT_RESET;
		USETW(ps.wPortChange, i);
		l = min(len, sizeof ps);
		memcpy(buf, &ps, l);
		totlen = l;
		break;
	case C(UR_SET_DESCRIPTOR, UT_WRITE_CLASS_DEVICE):
		err = USBD_IOERROR;
		goto ret;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_DEVICE):
		break;
	case C(UR_SET_FEATURE, UT_WRITE_CLASS_OTHER):
		if (index < 1 || index > sc->sc_noport) {
			/* only one port */
			err = USBD_IOERROR;
			goto ret;
		}
		port = USBC_HPRT_OFFSET;
		switch(value) {
		case UHF_PORT_ENABLE:
			DPRINTFN(2,("octeon_usbc_root_ctrl_transfer: enable port %d\n",
				    index));
			octeon_usbc_reg_assert(sc, port, USBC_HPRT_PRTENA);
			break;
		case UHF_PORT_SUSPEND:
			DPRINTFN(2,("octeon_usbc_root_ctrl_transfer: suspend port %d\n",
				    index));
			octeon_usbc_reg_assert(sc, port, USBC_HPRT_PRTSUSP);
			break;
		case UHF_PORT_RESET:
			DPRINTFN(2,("octeon_usbc_root_ctrl_transfer: reset port %d\n",
				    index));
			s = splhardusb();
			if (sc->sc_isprtbbl) {
				octeon_usbc_reinit_core(sc);
				sc->sc_isprtbbl = 0;
			}
			octeon_usbc_reg_assert_msked(sc, port,
					USBC_HPRT_PRTRST,USBC_HPRT_WR1CMSK);
			/* wait 250 msec ( > 10 msec )*/
			usb_delay_ms(&sc->sc_bus,
				     USB_PORT_ROOT_RESET_DELAY);

			if (sc->sc_dying) {
				err = USBD_IOERROR;
				splx(s);
				goto ret;
			}
			octeon_usbc_reg_deassert_msked(sc, port,
					USBC_HPRT_PRTRST,USBC_HPRT_WR1CMSK);
			sc->sc_isreset = 1;
			DPRINTFN(2,("octeon_usbc port %d reset,\n", index));
			splx(s);
			break;
		case UHF_PORT_POWER:
			DPRINTFN(2,("octeon_usbc_root_ctrl_transfer: set port power "
				    "%d\n", index));
			s = splhardusb();
			octeon_usbc_reg_assert(sc, port, USBC_HPRT_PRTPWR);
			splx(s);
			break;
		case UHF_PORT_TEST:
			DPRINTFN(2,("octeon_ubsc_root_ctrl_start: set port test "
				    "%d\n", index));
			break;
		default:
			err = USBD_IOERROR;
			goto ret;
		}
#ifdef USBC_DEBUG
		if (usbc_debug > 15)
			octeon_usbc_debug_reg_dump(sc, port);
#endif
		break;
	default:
		DPRINTFN(10,("octeon_usbc_root_ctrl_start: XXX DEFAULT?\n"));
		err = USBD_IOERROR;
		goto ret;
	}
	xfer->actlen = totlen;
	err = USBD_NORMAL_COMPLETION;
 ret:
	xfer->status = err;
	s = splusb();
	usb_transfer_complete(xfer);
	splx(s);
	return (USBD_IN_PROGRESS);
}

static void
octeon_usbc_root_ctrl_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(15,("octeon_usbc_root_ctrl_abort\n"));
	/* ohci.c comment say : Nothing to do, all transfers are synchronous. */
}

static void
octeon_usbc_root_ctrl_close(usbd_pipe_handle pipe)
{
	DPRINTFN(15,("octeon_usbc_root_ctrl_close\n"));
	/* Nothing to do. */
}

static void
octeon_usbc_root_ctrl_done(usbd_xfer_handle xfer)
{
	DPRINTFN(15,("octeon_usbc_root_ctrl_done\n"));
}


static usbd_status
octeon_usbc_root_intr_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;
	DPRINTFN(15, ("octeon_usbc_root_intr_transfer: enter\n"));

	/* Insert last in queue. */
	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	/* Pipe isn't running, start first */
	return (octeon_usbc_root_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static usbd_status
octeon_usbc_root_intr_start(usbd_xfer_handle xfer)
{
	usbd_pipe_handle pipe = xfer->pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(15, ("octeon_usbc_root_intr_start: enter\n"));

	if (sc->sc_dying)
		return (USBD_IOERROR);

	sc->sc_intrxfer = xfer;

	return (USBD_IN_PROGRESS);
}

static void
octeon_usbc_root_intr_abort(usbd_xfer_handle xfer)
{
	int s;
	DPRINTFN(15, ("octeon_usbc_root_intr_abort: enter\n"));

	if (xfer->pipe->intrxfer == xfer) {
		DPRINTFN(10,("octeon_usbc_root_intr_abort: remove\n"));
		xfer->pipe->intrxfer = NULL;
	}
	xfer->status = USBD_CANCELLED;
	s = splusb();
	usb_transfer_complete(xfer);
	splx(s);
}

static void
octeon_usbc_root_intr_close(usbd_pipe_handle pipe)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(15,("octeon_usbc_root_intr_close\n"));

	sc->sc_intrxfer = NULL;
}

static void
octeon_usbc_root_intr_done(usbd_xfer_handle xfer)
{
	DPRINTFN(15, ("octeon_usbc_root_intr_done: enter\n"));
	xfer->hcpriv = NULL;
}

static usbd_status
octeon_usbc_device_ctrl_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;

	DPRINTFN(15, ("octeon_usbc_device_ctrl_transfer: enter\n"));
	/* Insert last in queue. */
	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return (octeon_usbc_device_ctrl_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static usbd_status
octeon_usbc_device_ctrl_start(usbd_xfer_handle xfer)
{
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)xfer->pipe->device->bus;
	DPRINTFN(15, ("octeon_usbc_device_ctrl_start: enter\n"));
	usbd_status err;

	if (sc->sc_dying)
		return (USBD_IOERROR);

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST))
		panic("octeon_usbc_device_ctrl_transfer: not a request");
#endif

	err = octeon_usbc_device_request(xfer);
	if (err)
		return (err);

	if (sc->sc_bus.use_polling)
		octeon_usbc_waitintr(sc, xfer);
	return (USBD_IN_PROGRESS);
}

static void
octeon_usbc_device_ctrl_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(10,("octeon_usbc_device_ctrl_abort: xfer=%p\n", xfer));
	octeon_usbc_abort_xfer(xfer, USBD_CANCELLED);
}

static void
octeon_usbc_device_ctrl_close(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(10,("octeon_usbc_device_ctrl_close: pipe=%p\n", pipe));
	octeon_usbc_close_pipe(pipe, sc->sc_ctrl_head);
	octeon_usbc_free_soft_td(sc, usbc_pipe->tail.std);
}

static void
octeon_usbc_device_ctrl_done(usbd_xfer_handle xfer)
{
	DPRINTFN(10,("octeon_usbc_device_ctrl_done: xfer=%p\n", xfer));

#ifdef DIAGNOSTIC
	if (!(xfer->rqflags & URQ_REQUEST)) {
		panic("octeon_usbc_device_ctrl_done: not a request");
	}
#endif
}

static usbd_status
octeon_usbc_device_intr_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;
	DPRINTFN(15, ("octeon_usbc_device_intr_transfer: enter\n"));

	/* Insert last in queue. */
	err = usb_insert_transfer(xfer);
	if (err)
		return (err);

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return (octeon_usbc_device_intr_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static usbd_status
octeon_usbc_device_intr_start(usbd_xfer_handle xfer)
{
	DPRINTFN(15, ("octeon_usbc_device_intr_start: enter\n"));
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)xfer->pipe;
	usbd_device_handle dev = xfer->pipe->device;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)dev->bus;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	octeon_usbc_soft_td_t *data, *tail;
	usbd_status err = USBD_IN_PROGRESS;
	int len, is_read, endpt;
	int s;

	DPRINTFN(5, ("octeon_usbc_device_intr_start: xfer=%p len=%d flags=%d\n",
	    xfer, xfer->length, xfer->flags));

	if (sc->sc_dying)
		return (USBD_IOERROR);

	/* Refuse new transfer during disconnecting */
	if (sed->status == USBC_ENDPT_STATUS_SKIP) {
		log(LOG_DEBUG, "%s: addr#%d ep#%d is unavailable\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum);
		return (USBD_IOERROR);
	}

	data = usbc_pipe->tail.std;
	tail = octeon_usbc_alloc_soft_td(sc);
	if (tail == NULL)
		return (USBD_NOMEM);
	tail->xfer = NULL;

	len = xfer->length;
	endpt = usbc_pipe->pipe.endpoint->edesc->bEndpointAddress;
	is_read = UE_GET_DIR(endpt) == UE_DIR_IN;

	usbc_pipe->u.intr.length = len;

		data->next = tail;
		data->direction = is_read ? USBC_ENDPT_DIRECTION_IN
					: USBC_ENDPT_DIRECTION_OUT;
		data->buff = &xfer->dmabuf;
		data->len = len;
		data->pid = sed->toggle_state;
		data->xfer = xfer;
		data->flags = USBC_STD_FLAG_CALL_DONE;
		data->sed = sed;
	xfer->hcpriv = data;

	if (sc->sc_bus.use_polling)
		octeon_usbc_waitintr(sc, xfer);

	s = splusb();
	usbc_pipe->tail.std = tail;
	xfer->status = USBD_IN_PROGRESS;
	sed->status = USBC_ENDPT_STATUS_READY;
	err = octeon_usbc_set_next_transact(sc, sed);
	if (USBD_IN_PROGRESS != err) {
		DPRINTF(("%s: fault set_next_transact() (%s)\n",
				__func__, usbd_errstr(err)));
		callout_stop(&xfer->timeout_handle);
		sed->std_head = octeon_usbc_drop_soft_td(sc,
				sed->std_head, xfer);
		sed->status = USBC_ENDPT_STATUS_READY;
		xfer->status = err;
		usb_transfer_complete(xfer);
	}
	splx(s);

	return err;
}

static void
octeon_usbc_device_intr_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(1,("octeon_usbc_device_intr_abort: xfer=%p\n", xfer));

	/*
	 * Why do xHCI drivers set intrxfer to NULL?
	 * If do that, usbd_close_pipe() will NOT free intrxfer.
	 */
	/*
	if (xfer->pipe->intrxfer == xfer) {
		DPRINTFN(10,("octeon_usbc_device_intr_abort: remove\n"));
		xfer->pipe->intrxfer = NULL;
	}
	*/
	octeon_usbc_abort_xfer(xfer, USBD_CANCELLED);
}

static void
octeon_usbc_device_intr_close(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(1,("octeon_usbc_device_intr_close: pipe=%p\n", pipe));
	octeon_usbc_close_pipe(pipe, sc->sc_intr_head);
	octeon_usbc_free_soft_td(sc, usbc_pipe->tail.std);
}

static void
octeon_usbc_device_intr_done(usbd_xfer_handle xfer)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)xfer->pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)usbc_pipe->pipe.device->bus;
	octeon_usbc_soft_ed_t *sed;
	octeon_usbc_soft_td_t *data, *tail;
	usbd_status err;
	int len, endpt, is_read, s;

	DPRINTFN(5,("octeon_usbc_device_intr_done:"
				" xfer=%p, actlen=%d, repeat=%d\n",
				xfer, xfer->actlen, xfer->pipe->repeat));

	if (xfer->pipe->repeat) {

		len = usbc_pipe->u.intr.length;
		xfer->length = len;
		endpt = usbc_pipe->pipe.endpoint->edesc->bEndpointAddress;
		is_read = UE_GET_DIR(endpt) == UE_DIR_IN;
		sed = usbc_pipe->sed;

		data = usbc_pipe->tail.std;
		tail = octeon_usbc_alloc_soft_td(sc);
		if (tail == NULL) {
			xfer->status = USBD_NOMEM;
			return;
		}
		tail->xfer = NULL;
			data->next = tail;
			data->direction = is_read ? USBC_ENDPT_DIRECTION_IN
						: USBC_ENDPT_DIRECTION_OUT;
			data->buff = &xfer->dmabuf;
			data->len = xfer->length;
			data->pid = sed->toggle_state;
			data->xfer = xfer;
			data->flags = USBC_STD_FLAG_CALL_DONE;
			data->sed = sed;
		xfer->hcpriv = data;

		xfer->actlen = 0;

		if (sc->sc_bus.use_polling)
			octeon_usbc_waitintr(sc, xfer);

		s = splusb();
		usbc_pipe->tail.std = tail;
		xfer->status = USBD_IN_PROGRESS;
		sed->status = USBC_ENDPT_STATUS_READY;
		err = octeon_usbc_set_next_transact(sc, sed);
		if (err != USBD_IN_PROGRESS) {
			sed->std_head = octeon_usbc_drop_soft_td(sc,
					sed->std_head, xfer);
			DPRINTFN(-1, ("%s: set_next_transact() fail. (%s)\n",
					__func__, usbd_errstr(err)));
		}
		splx(s);
	}
}


static usbd_status
octeon_usbc_device_bulk_transfer(usbd_xfer_handle xfer)
{
	usbd_status err;
	DPRINTFN(15, ("octeon_usbc_device_bulk_transfer: enter\n"));

	/* Insert last in queue. */
	err = usb_insert_transfer(xfer);
	if (err) {
		DPRINTF(("%s: usb_insert_transfer(): %s, xfer=%p\n", __func__, usbd_errstr(err), xfer));
		return (err);
	}

	/*
	 * Pipe isn't running (otherwise err would be USBD_INPROG),
	 * so start it first.
	 */
	return (octeon_usbc_device_bulk_start(SIMPLEQ_FIRST(&xfer->pipe->queue)));
}

static usbd_status
octeon_usbc_device_bulk_start(usbd_xfer_handle xfer)
{
	DPRINTFN(15, ("octeon_usbc_device_bulk_start: enter\n"));
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)xfer->pipe;
	usbd_device_handle dev = xfer->pipe->device;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)dev->bus;
	octeon_usbc_soft_ed_t *sed = usbc_pipe->sed;
	octeon_usbc_soft_td_t *data, *tail;
	octeon_usbc_soft_td_t *term = NULL;
	usbd_status err = USBD_IN_PROGRESS;
	int len, is_read, endpt;
	int s;

	DPRINTFN(5, ("octeon_usbc_device_bulk_start: xfer=%p len=%d flags=%d\n",
	    xfer, xfer->length, xfer->flags));

	if (sc->sc_dying)
		return (USBD_IOERROR);

	/* Refuse new transfer during disconnecting */
	if (sed->status == USBC_ENDPT_STATUS_SKIP) {
		log(LOG_DEBUG, "%s: addr#%d ep#%d is unavailable\n",
				device_xname(sc->sc_dev),
				sed->hcchar_devaddr, sed->hcchar_epnum);
		return (USBD_IOERROR);
	}

	data = usbc_pipe->tail.std;
	tail = octeon_usbc_alloc_soft_td(sc);
	if (tail == NULL)
		return (USBD_NOMEM);
	tail->xfer = NULL;

	len = xfer->length;
	endpt = usbc_pipe->pipe.endpoint->edesc->bEndpointAddress;
	is_read = UE_GET_DIR(endpt) == UE_DIR_IN;

	usbc_pipe->u.bulk.length = len;

		data->next = tail;
		data->direction = is_read ? USBC_ENDPT_DIRECTION_IN
					: USBC_ENDPT_DIRECTION_OUT;
		data->buff = &xfer->dmabuf;
		data->len = len;
		data->pid = sed->toggle_state;
		data->xfer = xfer;
		data->flags = USBC_STD_FLAG_CALL_DONE;
		data->sed = sed;

	if (xfer->flags & USBD_FORCE_SHORT_XFER
			&& !is_read && len != 0
			&& len%sed->hcchar_mps == 0) {
		DPRINTFN(4,("%s: len=%d, mps=%d\n", __func__, len, sed->hcchar_mps));
		term = tail;
		tail = octeon_usbc_alloc_soft_td(sc);
		if (tail == NULL) {
			octeon_usbc_free_soft_td(sc, term);
			return (USBD_NOMEM);
		}
		tail->xfer = NULL;

		term->next = tail;
		term->direction = USBC_ENDPT_DIRECTION_OUT;
		term->len = 0;
		term->buff = &usbc_pipe->u.bulk.term_dma;
		term->pid = USBC_DATA_TGL_INHERIT;
		term->xfer = xfer;
		term->flags = USBC_STD_FLAG_CALL_DONE;
		term->sed = sed;
		data->flags = 0;
	}
	xfer->hcpriv = data;

	if (sc->sc_bus.use_polling)
		octeon_usbc_waitintr(sc, xfer);

	s = splusb();
	usbc_pipe->tail.std = tail;
	xfer->status = USBD_IN_PROGRESS;
	if (xfer->timeout && !sc->sc_bus.use_polling) {
                callout_reset(&xfer->timeout_handle, mstohz(xfer->timeout),
			    octeon_usbc_timeout, xfer);
	}
	sed->status = USBC_ENDPT_STATUS_READY;
	err = octeon_usbc_set_next_transact(sc, sed);
	if (USBD_IN_PROGRESS != err) {
		DPRINTF(("%s: fault set_next_transact() (%s)\n",
				__func__, usbd_errstr(err)));
		callout_stop(&xfer->timeout_handle);
		sed->std_head = octeon_usbc_drop_soft_td(sc,
				sed->std_head, xfer);
		sed->status = USBC_ENDPT_STATUS_READY;
		xfer->status = err;
		usb_transfer_complete(xfer);
	}
	splx(s);

	return err;
}

static void
octeon_usbc_device_bulk_abort(usbd_xfer_handle xfer)
{
	DPRINTFN(10,("octeon_usbc_device_bulk_abort: xfer=%p\n", xfer));
	octeon_usbc_abort_xfer(xfer, USBD_CANCELLED);
}

static void
octeon_usbc_device_bulk_close(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(1,("octeon_usbc_device_bulk_close: pipe=%p\n", pipe));
	octeon_usbc_close_pipe(pipe, sc->sc_bulk_head);
	octeon_usbc_free_soft_td(sc, usbc_pipe->tail.std);
}

static void
octeon_usbc_device_bulk_done(usbd_xfer_handle xfer)
{
	DPRINTFN(5,("octeon_usbc_device_bulk_done: xfer=%p, actlen=%d\n",
		     xfer, xfer->actlen));
}

static usbd_status
octeon_usbc_device_isoc_transfer(usbd_xfer_handle xfer)
{
	return (USBD_NORMAL_COMPLETION);
}

static usbd_status
octeon_usbc_device_isoc_start(usbd_xfer_handle xfer)
{
	return (USBD_NORMAL_COMPLETION);
}

static void
octeon_usbc_device_isoc_abort(usbd_xfer_handle xfer)
{
}
static void
octeon_usbc_device_isoc_close(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	octeon_usbc_softc_t *sc = (octeon_usbc_softc_t *)pipe->device->bus;

	DPRINTFN(1,("octeon_usbc_device_isoc_close: pipe=%p\n", pipe));
	octeon_usbc_close_pipe(pipe, sc->sc_isoc_head);
	octeon_usbc_free_soft_td(sc, usbc_pipe->tail.std);
}

static void
octeon_usbc_device_isoc_done(usbd_xfer_handle xfer)
{
}

static void
octeon_usbc_device_clear_toggle(usbd_pipe_handle pipe)
{
	struct octeon_usbc_pipe *usbc_pipe = (struct octeon_usbc_pipe *)pipe;
	usbc_pipe->sed->toggle_state = USBC_DATA_TGL_DATA0;
	/* TODO consider MDATA/DATA2 about IOSC/split
	   xfer on HIGHSPEED */
	DPRINTFN(15, ("octeon_usbc_device_clear_toggle: enter\n"));
}

static void
octeon_usbc_noop(usbd_pipe_handle pipe)
{
	DPRINTFN(15, ("octeon_usbc_noop: enter\n"));
}


void
octeon_usbc_add_ed(octeon_usbc_soft_ed_t *sed, octeon_usbc_soft_ed_t *head)
{
	DPRINTFN(8,("octeon_usbc_add_ed: sed=%p head=%p\n", sed, head));

	sed->next = head->next;
	head->next = sed;
}

void
octeon_usbc_rem_ed(octeon_usbc_soft_ed_t *sed, octeon_usbc_soft_ed_t *head)
{
	octeon_usbc_soft_ed_t *p;

	/* XXX */
	for (p = head; p != NULL && p->next != sed; p = p->next)
		;
	if (p == NULL)
		panic("octeon_usbc_rem_ed: ED not found");
	p->next = sed->next;
}


static octeon_usbc_soft_ed_t *
octeon_usbc_alloc_soft_ed (octeon_usbc_softc_t *sc)
{
	octeon_usbc_soft_ed_t *sed;

	if (sc->sc_freeseds == NULL) {
		DPRINTFN(10,("octeon_usbc_alloc_soft_ed: allocating chunk\n"));
		sed = malloc(USBC_SED_SIZE, M_USB, M_NOWAIT);
		if (sed == NULL)
			return NULL;
		sed->next = sc->sc_freeseds;
		sc->sc_freeseds = sed;
		DCOUNTUP(usbc_pool_ed);
	}
	sed = sc->sc_freeseds;
	sc->sc_freeseds = sed->next;
	sed->next = NULL;
	sed->hostch = NULL;
	sed->std_head = NULL;
	sed->status = USBC_ENDPT_STATUS_READY;
	sed->ping_state = 0;
	sed->toggle_state = USBC_DATA_TGL_DATA0;
	DPRINTFN(3,("octeon_usbc_alloc_soft_ed: alloc %p\n", sed));
	DCOUNTUP(usbc_used_ed);
	return (sed);
}
void
octeon_usbc_free_soft_ed(octeon_usbc_softc_t *sc, octeon_usbc_soft_ed_t *sed)
{
	DPRINTFN(3,("octeon_usbc_free_soft_ed: free %p\n", sed));
	sed->hostch = NULL;
	sed->std_head = NULL;
	sed->status = USBC_ENDPT_STATUS_SKIP;
	sed->next = sc->sc_freeseds;
	sc->sc_freeseds = sed;
	DCOUNTDOWN(usbc_used_ed);
}

octeon_usbc_soft_td_t *
octeon_usbc_alloc_soft_td (octeon_usbc_softc_t *sc)
{
	octeon_usbc_soft_td_t *std;
	int s;

	s = splusb();
	if (sc->sc_freestds == NULL) {
		DPRINTFN(10,("octeon_usbc_alloc_soft_td: allocating chunk\n"));
		std = malloc(USBC_STD_SIZE, M_USB, M_NOWAIT);
		if (std == NULL)
			goto fail;
		std->next = sc->sc_freestds;
		sc->sc_freestds = std;
		DCOUNTUP(usbc_pool_td);
	}
	std = sc->sc_freestds;
	sc->sc_freestds = std->next;
	std->next = NULL;
	std->sed = NULL;
	std->buff = NULL;
	std->xfer = NULL;
	std->flags = USBC_STD_FLAG_SKIP;
	std->pid = USBC_DATA_TGL_INHERIT;
	std->error_count = 0;
	std->len = std->actlen = 0;
	std->offset = 0;
	std->base_offset = 0;
	std->is_used = 1;
	DPRINTFN(2, ("%s: alloc std=%p\n", __func__, std));
	DCOUNTUP(usbc_used_td);
fail:
	splx(s);
	return (std);
}
void
octeon_usbc_free_soft_td(octeon_usbc_softc_t *sc, octeon_usbc_soft_td_t *std)
{
	int s;

	s = splusb();
	std->next = sc->sc_freestds;
	sc->sc_freestds = std;
	DPRINTFN(2, ("%s: free std=%p\n", __func__, std));
	if (!std->is_used) {
#ifdef DIAGNOSTIC
		printf("%s: t=%d, n=%d, d=%d, p=%d, l=%d,"
				" x=%p, f=0x%x, s=0x%x, s(h)=0x%x\n", __func__
					, std->sed->hcchar_eptype
					, std->sed->hcchar_epnum
					, std->direction
					, std->pid
					, std->len
					, std->xfer
					, std->flags
					, std->sed->status
					, std->sed->hostch->status
					);
		panic("%s: soft TD (%p) is already free\n",__func__,std);
#else
		log(LOG_WARNING, "%s: the soft TD is already free.\n",
				device_xname(sc->sc_dev));
		goto fail;
#endif
	}
	std->is_used = 0;
	DCOUNTDOWN(usbc_used_td);
#ifndef DIAGNOSTIC
 fail:
#endif
	splx(s);
}
static octeon_usbc_soft_td_t *
octeon_usbc_drop_soft_td(octeon_usbc_softc_t *sc,
		octeon_usbc_soft_td_t *std_head, usbd_xfer_handle xfer)
{
	octeon_usbc_soft_td_t *std_work = NULL, *std_next = NULL, *std_prev = NULL;

	for (std_work = std_head; std_work; std_work = std_next) {
		std_next = std_work->next;
		if(std_work->xfer == xfer) {
			DPRINTFN(5,("%s: remove from done_list std=%p, xfer=%p\n",
					__func__, std_work, std_work->xfer));
			if(std_prev)
				std_prev->next = std_work->next;
			else
				std_head = std_work->next;
#ifdef USBC_DEBUG
			if (usbc_debug > 0)
				log(LOG_DEBUG, "%s: free std=%p, xfer=%p\n",__func__,
						std_work, xfer);
#endif
			octeon_usbc_free_soft_td(sc, std_work);
		} else {
			std_prev = std_work;
		}
	}
	return std_head;
}

static inline void
octeon_usbc_reg_assert(octeon_usbc_softc_t *sc, bus_size_t offset, u_int32_t bits)
{
	u_int32_t value;
	
	value = octeon_usbc_reg_rd(sc, offset);
	value |= bits;
	octeon_usbc_reg_wr(sc, offset, value);
}
static inline void
octeon_usbc_reg_assert_msked(octeon_usbc_softc_t *sc, bus_size_t offset,
	       u_int32_t bits, u_int32_t msk)
{
	u_int32_t value;
	/* XXX(hsuenaga): why USBC_HPRT_OFFSET instaed of "offset" ??? */
	value = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
	value &= msk;
	value |= bits;
	octeon_usbc_reg_wr(sc, USBC_HPRT_OFFSET, value);
}

static inline void
octeon_usbc_reg_deassert(octeon_usbc_softc_t *sc, bus_size_t offset, u_int32_t bits)
{
	u_int32_t value;
	
	value = octeon_usbc_reg_rd(sc, offset);
	value &= ~bits;
	octeon_usbc_reg_wr(sc, offset, value);
}
static inline void
octeon_usbc_reg_deassert_msked(octeon_usbc_softc_t *sc, bus_size_t offset,
		u_int32_t bits, u_int32_t msk)
{
	u_int32_t value;
	/* XXX(hsuenaga): why USBC_HPRT_OFFSET instead of "offset" ??? */
	value = octeon_usbc_reg_rd(sc, USBC_HPRT_OFFSET);
	value &= msk;
	value &= ~bits;
	octeon_usbc_reg_wr(sc, USBC_HPRT_OFFSET, value);
}

static inline u_int32_t
octeon_usbc_reg_rd(octeon_usbc_softc_t *sc, bus_size_t offset)
{

	/* in the Cavium document, CSR addresses are written in little-endian
	   format. so the address should be swapped on the core running in
	   big-endian */
	return mips3_lw_a64(MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, ((USBC_BASE + offset) ^ 4)));
#if 0
	/* XXX currently, bus_space does not support 8, 16, 32 bit alignment */
	return bus_space_read_4(sc->sc_bust, sc->sc_regh, offset);
#endif
}

static inline void
octeon_usbc_reg_wr(octeon_usbc_softc_t *sc, bus_size_t offset, u_int32_t value)
{
	/* in the Cavium document, the CSR addresses are written in
	   little-endian format. so the address should be swapped on the core
	   which is running in big-endian */
	mips3_sw_a64(MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, ((USBC_BASE + offset) ^ 4)), value);
#if 0
	/* XXX currently, bus_space does not support 8, 16, 32 bit alignment */
	bus_space_write_4(sc->sc_bust, sc->sc_regh, offset, value);
#endif
}

#ifdef USBC_DEBUG
struct octeon_usbc_reg
{
	const char	*name;
	bus_size_t	offset;
	const char	*format;
};

static const struct octeon_usbc_reg octeon_usbc_regs[] = {
#define _ENTRY(n)	{#n, n##_OFFSET, n##_BITS}
	_ENTRY(USBC_GUSBCFG),
	_ENTRY(USBC_GRSTCTL),
	_ENTRY(USBC_GAHBCFG),
	_ENTRY(USBC_GINTSTS),
	_ENTRY(USBC_GINTMSK),
	_ENTRY(USBC_GRXSTSRH),
/*        _ENTRY(USBC_GRXSTSPH),*/
	_ENTRY(USBC_GRXFSIZ),
	_ENTRY(USBC_GNPTXFSIZ),
	_ENTRY(USBC_GNPTXSTS),
	_ENTRY(USBC_GHWCFG1),
	_ENTRY(USBC_GHWCFG2),
	_ENTRY(USBC_GHWCFG3),
	_ENTRY(USBC_GHWCFG4),
	_ENTRY(USBC_HCFG),
	_ENTRY(USBC_HPRT),
	_ENTRY(USBC_HFIR),
	_ENTRY(USBC_HAINT),
	_ENTRY(USBC_HAINTMSK),
	_ENTRY(USBC_HCINT0),
	_ENTRY(USBC_HCINT1),
	_ENTRY(USBC_HCINT2),
	_ENTRY(USBC_HCINT3),
	_ENTRY(USBC_HCINT4),
	_ENTRY(USBC_HCINT5),
	_ENTRY(USBC_HCINT6),
	_ENTRY(USBC_HCINT7),
	_ENTRY(USBC_HCINTMSK0),
	_ENTRY(USBC_HCINTMSK1),
	_ENTRY(USBC_HCINTMSK2),
	_ENTRY(USBC_HCINTMSK3),
	_ENTRY(USBC_HCINTMSK4),
	_ENTRY(USBC_HCINTMSK5),
	_ENTRY(USBC_HCINTMSK6),
	_ENTRY(USBC_HCINTMSK7),
	_ENTRY(USBC_HCCHAR0),
	_ENTRY(USBC_HCCHAR1),
	_ENTRY(USBC_HCCHAR2),
	_ENTRY(USBC_HCCHAR3),
	_ENTRY(USBC_HCCHAR4),
	_ENTRY(USBC_HCCHAR5),
	_ENTRY(USBC_HCCHAR6),
	_ENTRY(USBC_HCCHAR7),
	_ENTRY(USBC_HCTSIZ0),
	_ENTRY(USBC_HCTSIZ1),
	_ENTRY(USBC_HCTSIZ2),
	_ENTRY(USBC_HCTSIZ3),
	_ENTRY(USBC_HCTSIZ4),
	_ENTRY(USBC_HCTSIZ5),
	_ENTRY(USBC_HCTSIZ6),
	_ENTRY(USBC_HCTSIZ7),
#undef _ENTRY
};

void
octeon_usbc_debug_reg_dump(octeon_usbc_softc_t *sc, bus_size_t offset)
{
	int i;
	const struct octeon_usbc_reg *reg;

	reg = NULL;
	for (i = 0; i < (int)__arraycount(octeon_usbc_regs); i++)
		if (octeon_usbc_regs[i].offset == offset) {
			reg = &octeon_usbc_regs[i];
			break;
		}
	KASSERT(reg != NULL);

	octeon_usbc_debug_dumpreg(sc, reg);
}

void
octeon_usbc_debug_dumpregs(octeon_usbc_softc_t *sc)
{
	int i;

	for (i = 0; i < (int)__arraycount(octeon_usbc_regs); i++)
		octeon_usbc_debug_dumpreg(sc, &octeon_usbc_regs[i]);
}

void
octeon_usbc_debug_dumpreg(octeon_usbc_softc_t *sc,
    const struct octeon_usbc_reg *reg)
{
	uint64_t value;
	char buf[512];

	value = octeon_usbc_reg_rd(sc, reg->offset);
	snprintb(buf, sizeof(buf), reg->format, value);
	logprintf("\t%-24s: %s\n", reg->name, buf);
}

#endif
