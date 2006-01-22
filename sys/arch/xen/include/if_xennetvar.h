/*	$NetBSD: if_xennetvar.h,v 1.4.2.4 2006/01/22 10:42:58 tron Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _XEN_IF_XENNETVAR_H_
#define _XEN_IF_XENNETVAR_H_

#include <machine/xen.h>

union xennet_bufarray {
	struct {
		struct mbuf *xbtx_m;
	} xb_tx;
	struct {
		vaddr_t xbrx_va;
		paddr_t xbrx_pa;
		struct xennet_softc *xbrx_sc;
	} xb_rx;
	int xb_next;
};

struct xennet_softc {
	struct device		sc_dev;		/* base device glue */
	struct ethercom		sc_ethercom;	/* Ethernet common part */

	int			sc_ifno;

	uint8_t			sc_enaddr[6];

#ifdef mediacode
	struct ifmedia		sc_media;
#endif

	/* What is the status of our connection to the remote backend? */
#define BEST_CLOSED       0
#define BEST_DISCONNECTED 1
#define BEST_CONNECTED    2
	unsigned int		sc_backend_state;

	unsigned int		sc_evtchn;

	netif_tx_interface_t	*sc_tx;
	netif_rx_interface_t	*sc_rx;
	struct vm_page		*sc_pg_tx;
	struct vm_page		*sc_pg_rx;

	uint32_t		sc_tx_entries;
	uint32_t		sc_tx_resp_cons;

	uint32_t		sc_rx_resp_cons;
	uint32_t		sc_rx_bufs_to_notify;

	union xennet_bufarray	sc_tx_bufa[NETIF_TX_RING_SIZE];
	union xennet_bufarray	sc_rx_bufa[NETIF_RX_RING_SIZE];

#if NRND > 0
	rndsource_element_t	sc_rnd_source;
#endif
};

struct xennet_attach_args {
	const char 		*xa_device;
	int			xa_handle;
};

struct nfs_diskless;

int xennet_scan(struct device *, struct xennet_attach_args *, cfprint_t);
void xennet_start(struct ifnet *);
int xennet_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
void xennet_watchdog(struct ifnet *ifp);
int xennet_bootstatic_callback(struct nfs_diskless *);

#endif /* _XEN_IF_XENNETVAR_H_ */
