/*	$NetBSD: octeon_usbcvar.h,v 1.4 2018/04/09 16:21:10 jakllsch Exp $	*/

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

#ifndef _OCTEON_USBCVAR_H_
#define _OCTEON_USBCVAR_H_
#include <dev/usb/usb.h>

typedef u_int32_t octeon_usbc_physaddr_t;
typedef u_int32_t octeon_usbc_link_t;
struct octeon_usbc_soft_td;
struct octeon_usbc_soft_ed;
struct octeon_usbc_softc;

typedef enum octeon_usbc_halt_status{
	USBC_HALT_NO_HALT_STATUS,
	USBC_HALT_XACT_COMPLETE,
	USBC_HALT_ACK,
	USBC_HALT_NAK,
	USBC_HALT_NYET,
	USBC_HALT_STALL,
	USBC_HALT_XACT_ERR,
	USBC_HALT_FRAME_OVERRUN,
	USBC_HALT_CHANNEL_BABBLE_ERR,
	USBC_HALT_DATA_TOGGLE_ERR,
	USBC_HALT_AHB_ERR,
	USBC_HALT_PERIODIC_INCOMPLETE,
	USBC_HALT_XFER_COMPLETE
} octeon_usbc_halt_status_t;

#define USBC_DMA_BURST_SIZE 0

#define USBC_MAX_PERIO_FIFOS 15
#define USBC_MAX_ENDPOINTS_CHANNELS 16
typedef struct octeon_usbc_hostch { /* FIXME */
	u_int32_t	hc_id;
	u_int32_t	hc_num;

	struct octeon_usbc_softc *sc;
	callout_t	tmo_retry;

	struct octeon_usbc_soft_ed *sed;
	struct octeon_usbc_soft_td *std_cur;
	struct octeon_usbc_soft_td *std_done_head;

	u_int8_t	status;
#define USBC_HOST_CH_STATUS_FREE	0x0
#define USBC_HOST_CH_STATUS_RESERVED	0x1
#define USBC_HOST_CH_STATUS_WAIT_HLT	0x2
#define USBC_HOST_CH_STATUS_NOW_XFER	0x4
#define USBC_HOST_CH_STATUS_SLAVE_HALT	0x8

	u_int8_t	error_state	:1;

	/* stored registers */
	u_int32_t	hcint;
	u_int32_t	hcintmsk;
	u_int32_t	hcchar;
	u_int32_t	hcsplt;
	u_int32_t	hctsiz;

	/* register offsets */
	bus_size_t	offset_usbc_hcchar;
	bus_size_t	offset_usbc_hcsplt;
	bus_size_t	offset_usbc_hcint;
	bus_size_t	offset_usbc_hcintmsk;
	bus_size_t	offset_usbc_hctsiz;
#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
	bus_size_t	offset_usbc_dfifo;
#endif
} octeon_usbc_hostch_t;

typedef struct octeon_usbc_xfer {
	struct usbd_xfer xfer;
	struct usb_task	abort_task;
} octeon_usbc_xfer_t;


/* endpoint descriptor */
typedef struct octeon_usbc_soft_ed {
	struct octeon_usbc_soft_ed	*next;

	u_int8_t	ref_count;

	struct octeon_usbc_soft_td	*std_head;
	
	u_int8_t	status;
#define	USBC_ENDPT_STATUS_SKIP		0
#define USBC_ENDPT_STATUS_READY		1
#define USBC_ENDPT_STATUS_IN_PROGRESS	2
#define USBC_ENDPT_STATUS_WAIT_NEXT_SOF	3
#define USBC_ENDPT_STATUS_WAIT_FIFO_EMPTY	4
#define USBC_ENDPT_STATUS_WAIT_PING_ACK	5

	octeon_usbc_hostch_t	*hostch;

	u_int32_t	interval;
	u_int32_t	interval_count;
	u_int8_t	ping_state;
	u_int8_t	toggle_state;
#define USBC_DATA_TGL_DATA0		0x00
#define USBC_DATA_TGL_DATA1		0x01
#define USBC_DATA_TGL_DATA2		0x02
#define USBC_DATA_TGL_MDATA		0x03
#define USBC_DATA_TGL_SETUP		0x04
#define USBC_DATA_TGL_INHERIT		0xFF

	/* hcsplt params */
	u_int8_t	hcsplt_splt_ena;

	/* hcchar params */
	u_int16_t	hcchar_mps;
	u_int8_t	hcchar_epnum;
	u_int8_t	hcchar_devaddr;
	u_int8_t	hcchar_lspddev;
	u_int8_t	hcchar_eptype;
#define USBC_ENDPT_TYPE_CTRL	UE_CONTROL
#define USBC_ENDPT_TYPE_ISOC	UE_ISOCHRONOUS
#define USBC_ENDPT_TYPE_BULK	UE_BULK
#define USBC_ENDPT_TYPE_INTR	UE_INTERRUPT
#define USBC_ENDPT_TYPE_IS_PERIO(eptype) (((eptype) & 0x1) != 0x0)
} octeon_usbc_soft_ed_t;
#define USBC_SED_SIZE	( sizeof(octeon_usbc_soft_ed_t) )
#define USBC_SED_LIST_MAX 128

/* transaction descriptor */
typedef struct octeon_usbc_soft_td {
	struct octeon_usbc_soft_td	*next;
	struct octeon_usbc_soft_ed	*sed;
	usb_dma_t		*buff;
	struct usbd_xfer	*xfer;
	u_int32_t	offset;
	u_int32_t	base_offset;
	u_int8_t	error_count;
	u_int8_t	data_toggle;
	u_int8_t	direction;
#define USBC_ENDPT_DIRECTION_OUT	0
#define USBC_ENDPT_DIRECTION_IN		1
	u_int8_t  pid;
	u_int32_t len;
	u_int32_t actlen;
	u_int16_t flags;
#define USBC_STD_FLAG_CALL_DONE 0x0001
#define USBC_STD_FLAG_ADD_LEN	0x0002
#define USBC_STD_FLAG_SKIP	0x0004
	u_int8_t is_used;
} octeon_usbc_soft_td_t;
#define USBC_STD_SIZE	( sizeof(octeon_usbc_soft_td_t) )
#define USBC_STD_LIST_MAX 128


typedef struct octeon_usbc_softc {
	device_t		sc_dev;
	struct usbd_bus		sc_bus;
	
	bus_space_tag_t         sc_bust;	/* iobus space */
	bus_space_handle_t	sc_regh;	/* usbc register space */

	bus_size_t sc_size;


	usb_dma_t		sc_hccadma;
	u_int8_t		sc_dma_enable;

#ifdef OCTEON_USBN_CN31XX_DMA_WORKAROUND
	/*
	 * Indicate enable/disable workaround for CN31xx Pass 1.1 USBN's broken DMA.
	 */
	u_int8_t	sc_workaround_force_slave_mode; /** force to use slave mode */
#endif

	int sc_noport;
	u_int8_t sc_addr;		/* device address */
	u_int8_t sc_conf;		/* device configuration */
	struct usbd_xfer *sc_intrxfer;
	char sc_isreset;		/* USBC is reseted ? */
	char sc_connstat_change;	/* USBC is disconnected ? */
#define USBC_PRT_STAT_CONNECTED		0x01
#define USBC_PRT_STAT_DISCONNECTED	0x02
	char sc_isprtbbl;		/* USBC is port babble ? */
#ifdef USB_USE_SOFTINTR
	char sc_softwake;
#endif /* USB_USE_SOFTINTR */

	u_int16_t	sc_frame_number;
	u_int16_t	sc_frame_interval;

	octeon_usbc_soft_ed_t *sc_isoc_head;
	octeon_usbc_soft_ed_t *sc_ctrl_head;
	octeon_usbc_soft_ed_t *sc_intr_head;
	octeon_usbc_soft_ed_t *sc_bulk_head;

	octeon_usbc_soft_ed_t *sc_freeseds;
	octeon_usbc_soft_td_t *sc_freestds;

	SIMPLEQ_HEAD(, usbd_xfer) sc_free_xfers; /* free xfers */

	void                    *sc_ih;
#if defined(__NetBSD__)
	device_t sc_child;
#elif defined(__OpenBSD__)
	device_ptr_t sc_child;			/* /dev/usb# device */
#endif
	char sc_dying;

	u_int8_t sc_port_speed;
#ifndef USBC_HPRT_PRTSPD_HIGH
#define  USBC_HPRT_PRTSPD_HIGH		0x0
#endif
#ifndef USBC_HPRT_PRTSPD_FULL
#define  USBC_HPRT_PRTSPD_FULL		0x1
#endif
#ifndef USBC_HPRT_PRTSPD_LOW
#define  USBC_HPRT_PRTSPD_LOW			0x2
#endif
#ifndef USBC_HPRT_PRTSPD_RESERVED
#define  USBC_HPRT_PRTSPD_RESERVED		0x3
#endif


	u_int32_t sc_rx_fifo_size;
	u_int32_t sc_nperio_tx_fifo_siz;
	u_int32_t sc_perio_tx_fifo_siz;

	/* stored registors */
	u_int32_t sc_haint;
	u_int32_t sc_haintmsk;

	/* H/W Configuration */
	struct{
		u_int32_t ptxqdepth;
		u_int32_t nptxqdepth;
		u_int8_t dynfifosizing;
		u_int8_t periosupport;
		u_int8_t numhstch; /** number of host channels */

#if USBC_USE_DEV_MODE
		/* device mode params */
		u_int8_t num_dev_eps;
		u_int8_t num_dev_perio_eps;
#endif
	} sc_hwcfg;

	octeon_usbc_hostch_t *sc_hostch_list; /** host channel list */

	callout_t sc_tmo_hprt;
	callout_t sc_tmo_nptxfemp;

} octeon_usbc_softc_t;

struct octeon_usbc_pipe {
	struct usbd_pipe pipe;
	int nexttoggle;

	octeon_usbc_soft_ed_t *sed;

	union {
		octeon_usbc_soft_td_t *std;
	} tail;

	/* Info needed for different pipe kinds. */
	union {
		/* Control pipe */
		struct {
			usb_dma_t req_setup_dma;
			usb_dma_t req_stat_dma;
#define USBC_CONTROL_STATUS_BUF_SIZE 64
			u_int length;
			octeon_usbc_soft_td_t *setup, *data, *stat;
		} ctl;
		/* Interrupt pipe */
		struct {
			u_int length;
			int nslots;
			int pos;
		} intr;
		/* Bulk pipe */
		struct {
			u_int length;
			int isread;
			usb_dma_t term_dma;
#define USBC_BULK_TERMINATE_BUF_SIZE 64
		} bulk;
		/* Iso pipe */
		struct iso {
			int next, inuse;
		} iso;
	} u;
};


usbd_status octeon_usbc_init(struct octeon_usbc_softc *sc);

#endif /* _OCTEON_USBCVAR_H_ */
