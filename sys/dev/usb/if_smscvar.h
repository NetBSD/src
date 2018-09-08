/*	$NetBSD: if_smscvar.h,v 1.6 2018/09/08 13:10:08 mlelstv Exp $	*/

/*	$OpenBSD: if_smscreg.h,v 1.2 2012/09/27 12:38:11 jsg Exp $	*/
/*-
 * Copyright (c) 2012
 *	Ben Gray <bgray@freebsd.org>.
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/usb/net/if_smscreg.h,v 1.1 2012/08/15 04:03:55 gonzo Exp $
 */
#ifndef _IF_SMSCVAR_H_
#define _IF_SMSCVAR_H_

#include <sys/rndsource.h>

struct smsc_chain {
	struct smsc_softc	*sc_sc;
	struct usbd_xfer	*sc_xfer;
	char			*sc_buf;
	struct mbuf		*sc_mbuf;
	int			 sc_accum;
	int			 sc_idx;
};

struct smsc_cdata {
	struct smsc_chain	 tx_chain[SMSC_TX_LIST_CNT];
	struct smsc_chain	 rx_chain[SMSC_RX_LIST_CNT];
	int			 tx_free;
	int			 tx_next;
	int			 rx_next;
};

struct smsc_softc {
	device_t		sc_dev;
	struct usbd_device *	sc_udev;
	bool			sc_dying;
	bool			sc_stopping;
	bool			sc_ttpending;

	uint8_t			sc_enaddr[ETHER_ADDR_LEN];
	struct ethercom		sc_ec;
	struct mii_data		sc_mii;
	krndsource_t		sc_rnd_source;
	int			sc_phyno;
	struct usbd_interface *	sc_iface;

	/*
	 * The following stores the settings in the mac control (MAC_CSR)
	 * register
	 */
	uint32_t		sc_mac_csr;
	uint32_t		sc_rev_id;

	uint32_t		sc_coe_ctrl;

	int			sc_if_flags;
	int			sc_refcnt;

	struct usb_task		sc_tick_task;

	int			sc_ed[SMSC_ENDPT_MAX];
	struct usbd_pipe *	sc_ep[SMSC_ENDPT_MAX];

	kmutex_t		sc_lock;
	kmutex_t		sc_txlock;
	kmutex_t		sc_rxlock;
	kmutex_t		sc_mii_lock;
	kcondvar_t		sc_detachcv;

	struct smsc_cdata	sc_cdata;
	callout_t		sc_stat_ch;

	struct timeval		sc_rx_notice;
	u_int			sc_bufsz;

	uint32_t		sc_flags;
#define	SMSC_FLAG_LINK      0x0001

	struct if_percpuq *sc_ipq;		/* softint-based input queues */

};

#define SMSC_MIN_BUFSZ		2048
#define SMSC_MAX_BUFSZ		18944

#endif  /* _IF_SMSCVAR_H_ */
