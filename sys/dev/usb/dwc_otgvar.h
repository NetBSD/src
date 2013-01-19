/*	$NetBSD: dwc_otgvar.h,v 1.6 2013/01/19 14:07:37 skrll Exp $ */

/* $FreeBSD: src/sys/dev/usb/controller/dwc_otg.h,v 1.12 2012/09/27 15:23:38 hselasky Exp $ */
/*-
 * Copyright (c) 2012 Hans Petter Selasky. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_DWC_OTGVAR_H_
#define	_DWC_OTGVAR_H_

#include <sys/pool.h>
#include <sys/workqueue.h>

#define	DWC_OTG_MAX_DEVICES MIN(USB_MAX_DEVICES, 32)
#define	DWC_OTG_FRAME_MASK 0x7FF
#define	DWC_OTG_MAX_TXP 4
#define	DWC_OTG_MAX_TXN (0x200 * DWC_OTG_MAX_TXP)
#define	DWC_OTG_MAX_CHANNELS 16
#define	DWC_OTG_MAX_ENDPOINTS 16
#define	DWC_OTG_HOST_TIMER_RATE 10 /* ms */

#define DOTG_COUNTERS
#ifdef DOTG_COUNTERS
/*
 * curmode (bit 0)is a mode indication bit 0 = device, 1 = host
 */
#define	DWC_OTG_INTRBITF	1
#define	DWC_OTG_INTRBITL	31
#define	DWC_OTG_NINTRBITS	32
#endif

struct dwc_otg_td;
struct dwc_otg_softc;

typedef uint8_t (dwc_otg_cmd_t)(struct dwc_otg_td *td);

typedef struct dwc_otg_td {
	struct dwc_otg_td *obj_next;
	dwc_otg_cmd_t *func;
	usbd_xfer_handle xfer;
 	void *buf;
	uint32_t actlen;
	uint32_t tx_bytes;
	uint32_t offset;
	uint32_t remainder;
	uint32_t hcchar;		/* HOST CFG */
	uint32_t hcsplt;		/* HOST CFG */
	uint16_t max_packet_size;	/* packet_size */
	uint16_t npkt;
	uint8_t errcnt;
	uint8_t retrycnt;
	uint8_t tmr_res;
	uint8_t tmr_val;
	uint8_t curr_frame;
	uint8_t	ep_no;
	uint8_t channel;
	uint8_t state;
#define	DWC_CHAN_ST_START 0
#define	DWC_CHAN_ST_WAIT_ANE 1
#define	DWC_CHAN_ST_WAIT_S_ANE 2
#define	DWC_CHAN_ST_WAIT_C_ANE 3
#define	DWC_CHAN_ST_RX_PKT 4
#define	DWC_CHAN_ST_RX_SPKT 5
#define	DWC_CHAN_ST_RX_SPKT_SYNC 6
#define	DWC_CHAN_ST_TX_PKT 4
#define	DWC_CHAN_ST_TX_CPKT 5
#define	DWC_CHAN_ST_TX_PKT_SYNC 6
	uint8_t	error:1;
	uint8_t	error_any:1;
	uint8_t	error_stall:1;
	uint8_t	alt_next:1;
	uint8_t	short_pkt:1;
	uint8_t	did_stall:1;
	uint8_t toggle:1;
	uint8_t set_toggle:1;
	uint8_t got_short:1;
	uint8_t did_nak:1;
} dwc_otg_td_t;


struct dwc_otg_std_temp {
	dwc_otg_cmd_t *func;
	usbd_xfer_handle xfer;
 	void *buf;
	struct dwc_otg_td *td;
	struct dwc_otg_td *td_next;
	uint32_t len;
	uint32_t offset;
	uint16_t max_frame_size;
	uint8_t	short_pkt;

	/*
	 * short_pkt = 0: transfer should be short terminated
	 * short_pkt = 1: transfer should not be short terminated
	 */
	uint8_t	setup_alt_next;
	uint8_t did_stall;
	uint8_t bulk_or_control;
};

struct dwc_otg_flags {
	uint8_t	change_connect:1;
	uint8_t	change_suspend:1;
	uint8_t change_reset:1;
	uint8_t change_enabled:1;
	uint8_t change_over_current:1;
	uint8_t	status_suspend:1;	/* set if suspended */
	uint8_t	status_vbus:1;		/* set if present */
	uint8_t	status_bus_reset:1;	/* set if reset complete */
	uint8_t	status_high_speed:1;	/* set if High Speed is selected */
	uint8_t	status_low_speed:1;	/* set if Low Speed is selected */
	uint8_t status_device_mode:1;	/* set if device mode */
	uint8_t	self_powered:1;
	uint8_t	clocks_off:1;
	uint8_t	port_powered:1;
	uint8_t	port_enabled:1;
	uint8_t port_over_current:1;
	uint8_t	d_pulled_up:1;
};

#if 0
typedef struct dwc_otg_soft_ed {
} dwc_otg_soft_ed_t;

typedef struct dwc_otg_soft_td {
	dwc_otg_td_t *td;
	usb_dma_t dma;
	int offs;
} dwc_otg_soft_td_t;
#endif

struct dwc_otg_work {
	struct work wk;
	struct dwc_otg_softc *sc;
	usbd_xfer_handle xfer;
};

struct dwc_otg_xfer {
	struct usbd_xfer xfer;			/* Needs to be first */
	struct usb_task	abort_task;
	TAILQ_ENTRY(dwc_otg_xfer) xnext;	/* list of active/complete xfers */
	bool		queued;

	void		*td_start[1];
	dwc_otg_td_t	*td_transfer_first;
	dwc_otg_td_t	*td_transfer_last;
	dwc_otg_td_t	*td_transfer_cache;

	int		toggle_next;

	struct dwc_otg_work work;
};

struct dwc_otg_chan_state {
	uint32_t hcint;
	uint8_t wait_sof;
	uint8_t allocated;
	uint8_t suspended;
};

typedef struct dwc_otg_softc {
	device_t sc_dev;
	struct usbd_bus sc_bus;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;

	void *sc_rhc_si;

	struct workqueue *sc_wq;
	struct dwc_otg_work sc_timer_work;

	int sc_noport;

	usbd_xfer_handle sc_intrxfer;

	device_t sc_child;	/* /dev/usb# device */
	char sc_dying;
	struct usb_dma_reserve sc_dma_reserve;

	char sc_vendor[32];		/* vendor string for root hub */
	int sc_id_vendor;		/* vendor ID for root hub */

	TAILQ_HEAD(, dwc_otg_xfer) sc_active;	/* active transfers */
	TAILQ_HEAD(, dwc_otg_xfer) sc_complete;	/* complete transfers */

	pool_cache_t sc_tdpool;
	pool_cache_t sc_xferpool;
	
#ifdef DOTG_COUNTERS
	struct evcnt sc_ev_intr;
	struct evcnt sc_ev_intr_bit[DWC_OTG_NINTRBITS];

	struct evcnt sc_ev_soft_intr;
	struct evcnt sc_ev_work;

	struct evcnt sc_ev_tdpoolget;
	struct evcnt sc_ev_tdpoolput;
	struct evcnt sc_ev_xferpoolget;
	struct evcnt sc_ev_xferpoolput;
#endif

	/* From FreeBSD softc */
	struct callout sc_timer;

	uint32_t sc_rx_bounce_buffer[1024 / 4];
	uint32_t sc_tx_bounce_buffer[(512 * DWC_OTG_MAX_TXP) / 4];

	uint32_t sc_fifo_size;
	uint32_t sc_irq_mask;
	uint32_t sc_last_rx_status;
#if 0
	uint32_t sc_out_ctl[DWC_OTG_MAX_ENDPOINTS];
	uint32_t sc_in_ctl[DWC_OTG_MAX_ENDPOINTS];
#endif
	struct dwc_otg_chan_state sc_chan_state[DWC_OTG_MAX_CHANNELS];
	uint32_t sc_hprt_val;
	uint32_t sc_tmr_val;	/* timer value */
	uint16_t sc_active_rx_ep;

	uint8_t sc_timer_active;
	uint8_t	sc_dev_ep_max;
	uint8_t sc_dev_in_ep_max;
	uint8_t	sc_host_ch_max;

	uint8_t sc_addr;		/* device address */
	uint8_t sc_conf;		/* device configuration */
	uint8_t sc_mode;		/* mode of operation */
#define	DWC_MODE_OTG	0	/* both modes */
#define	DWC_MODE_DEVICE	1	/* device only */
#define	DWC_MODE_HOST	2	/* host only */

	struct dwc_otg_flags sc_flags;

} dwc_otg_softc_t;

usbd_status	dwc_otg_init(dwc_otg_softc_t *);
int		dwc_otg_intr(void *);
int		dwc_otg_detach(dwc_otg_softc_t *, int);
bool		dwc_otg_shutdown(device_t, int);
void		dwc_otg_childdet(device_t, device_t);
int		dwc_otg_activate(device_t, enum devact);
bool		dwc_otg_resume(device_t, const pmf_qual_t *);
bool		dwc_otg_suspend(device_t, const pmf_qual_t *);

#endif	/* _DWC_OTGVAR_H_ */
