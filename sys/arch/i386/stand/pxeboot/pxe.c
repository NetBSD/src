/*	$NetBSD: pxe.c,v 1.17 2009/12/13 23:01:42 jakllsch Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*      
 * Copyright (c) 2000 Alfred Perlstein <alfred@freebsd.org>
 * All rights reserved.
 * Copyright (c) 2000 Paul Saab <ps@freebsd.org>
 * All rights reserved.
 * Copyright (c) 2000 John Baldwin <jhb@freebsd.org>
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

/*
 * Support for the Intel Preboot Execution Environment (PXE).
 *
 * PXE provides a UDP implementation as well as a UNDI network device
 * driver.  UNDI is much more complicated to use than PXE UDP, so we
 * use PXE UDP as a cheap and easy way to get PXE support.
 */

#include <sys/param.h>
#include <sys/socket.h>

#ifdef _STANDALONE
#include <lib/libkern/libkern.h>
#else
#include <string.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <net/if_ether.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/bootp.h>

#include <libi386.h>
#include <bootinfo.h>

#include "pxeboot.h"
#include "pxe.h"
#include "pxe_netif.h"

void	(*pxe_call)(uint16_t);

void	pxecall_bangpxe(uint16_t);	/* pxe_call.S */
void	pxecall_pxenv(uint16_t);	/* pxe_call.S */

char pxe_command_buf[256];

BOOTPLAYER bootplayer;

static struct btinfo_netif bi_netif;

/*****************************************************************************
 * This section is a replacement for libsa/udp.c
 *****************************************************************************/

/* Caller must leave room for ethernet, ip, and udp headers in front!! */
ssize_t
sendudp(struct iodesc *d, void *pkt, size_t len)
{
	t_PXENV_UDP_WRITE *uw = (void *) pxe_command_buf;

	uw->status = 0;

	uw->ip = d->destip.s_addr;
	uw->gw = gateip.s_addr;
	uw->src_port = d->myport;
	uw->dst_port = d->destport;
	uw->buffer_size = len;
	uw->buffer.segment = VTOPSEG(pkt);
	uw->buffer.offset = VTOPOFF(pkt);

	pxe_call(PXENV_UDP_WRITE);

	if (uw->status != PXENV_STATUS_SUCCESS) {
		/* XXX This happens a lot; it shouldn't. */
		if (uw->status != PXENV_STATUS_FAILURE)
			printf("sendudp: PXENV_UDP_WRITE failed: 0x%x\n",
			    uw->status);
		return (-1);
	}

	return (len);
}

/*
 * Receive a UDP packet and validate it for us.
 * Caller leaves room for the headers (Ether, IP, UDP).
 */
ssize_t
readudp(struct iodesc *d, void *pkt, size_t len, saseconds_t tleft)
{
	t_PXENV_UDP_READ *ur = (void *) pxe_command_buf;
	struct udphdr *uh;
	struct ip *ip;

	uh = (struct udphdr *)pkt - 1;
	ip = (struct ip *)uh - 1;

	(void)memset(ur, 0, sizeof(*ur));

	ur->dest_ip = d->myip.s_addr;
	ur->d_port = d->myport;
	ur->buffer_size = len;
	ur->buffer.segment = VTOPSEG(pkt);
	ur->buffer.offset = VTOPOFF(pkt);

	/* XXX Timeout unused. */

	pxe_call(PXENV_UDP_READ);

	if (ur->status != PXENV_STATUS_SUCCESS) {
		/* XXX This happens a lot; it shouldn't. */
		if (ur->status != PXENV_STATUS_FAILURE)
			printf("readudp: PXENV_UDP_READ_failed: 0x%0x\n",
			    ur->status);
		return (-1);
	}

	ip->ip_src.s_addr = ur->src_ip;
	uh->uh_sport = ur->s_port;
	uh->uh_dport = d->myport;

	return (ur->buffer_size);
}

/*
 * netif layer:
 *  open, close, shutdown: called from dev_net.c
 *  socktodesc: called by network protocol modules
 *
 * We only allow one open socket.
 */

static int pxe_inited;
static struct iodesc desc;

int
pxe_netif_open(void)
{
	t_PXENV_UDP_OPEN *uo = (void *) pxe_command_buf;

	if (!pxe_inited) {
		if (pxe_init() != 0)
			return (-1);
		pxe_inited = 1;
	}
	BI_ADD(&bi_netif, BTINFO_NETIF, sizeof(bi_netif));

	(void)memset(uo, 0, sizeof(*uo));

	uo->src_ip = bootplayer.yip;

	pxe_call(PXENV_UDP_OPEN);

	if (uo->status != PXENV_STATUS_SUCCESS) {
		printf("pxe_netif_probe: PXENV_UDP_OPEN failed: 0x%x\n",
		    uo->status);
		return (-1);
	}

	memcpy(desc.myea, bootplayer.CAddr, ETHER_ADDR_LEN);

	/*
	 * Since the PXE BIOS has already done DHCP, make sure we
	 * don't reuse any of its transaction IDs.
	 */
	desc.xid = bootplayer.ident;

	return (0);
}

void
pxe_netif_close(int sock)
{
	t_PXENV_UDP_CLOSE *uc = (void *) pxe_command_buf;

#ifdef NETIF_DEBUG
	if (sock != 0)
		printf("pxe_netif_close: sock=%d\n", sock);
#endif

	uc->status = 0;

	pxe_call(PXENV_UDP_CLOSE);

	if (uc->status != PXENV_STATUS_SUCCESS)
		printf("pxe_netif_end: PXENV_UDP_CLOSE failed: 0x%x\n",
		    uc->status);
}

struct iodesc *
socktodesc(int sock)
{

#ifdef NETIF_DEBUG
	if (sock != 0)
		return (0);
	else
#endif
		return (&desc);
}

/*****************************************************************************
 * PXE initialization and support routines
 *****************************************************************************/

uint16_t pxe_command_buf_seg;
uint16_t pxe_command_buf_off;

extern uint16_t bangpxe_off, bangpxe_seg;
extern uint16_t pxenv_off, pxenv_seg;

static struct btinfo_netif bi_netif;

int
pxe_init(void)
{
	t_PXENV_GET_CACHED_INFO *gci = (void *) pxe_command_buf;
	t_PXENV_UNDI_GET_NIC_TYPE *gnt = (void *) pxe_command_buf;
	pxenv_t *pxenv;
	pxe_t *pxe;
	char *cp;
	int i;
	uint8_t cksum, *ucp;

	/*
	 * Checking for the presence of PXE is a machine-dependent
	 * operation.  On the IA-32, this can be done two ways:
	 *
	 *	Int 0x1a function 0x5650
	 *
	 *	Scan memory for the !PXE or PXENV+ signatures
	 *
	 * We do the latter, since the Int method returns a pointer
	 * to a deprecated structure (PXENV+).
	 */

	pxenv = NULL;
	pxe = NULL;

	for (cp = (char *)0xa0000; cp > (char *)0x10000; cp -= 2) {
		if (pxenv == NULL) {
			pxenv = (pxenv_t *)cp;
			if (MEMSTRCMP(pxenv->Signature, "PXENV+"))
				pxenv = NULL;
			else {
				for (i = 0, ucp = (uint8_t *)cp, cksum = 0;
				     i < pxenv->Length; i++)
					cksum += ucp[i];
				if (cksum != 0) {
					printf("pxe_init: bad cksum (0x%x) "
					    "for PXENV+ at 0x%lx\n", cksum,
					    (u_long) cp);
					pxenv = NULL;
				}
			}
		}

		if (pxe == NULL) {
			pxe = (pxe_t *)cp;
			if (MEMSTRCMP(pxe->Signature, "!PXE"))
				pxe = NULL;
			else {
				for (i = 0, ucp = (uint8_t *)cp, cksum = 0;
				     i < pxe->StructLength; i++)
					cksum += ucp[i];
				if (cksum != 0) {
					printf("pxe_init: bad cksum (0x%x) "
					    "for !PXE at 0x%lx\n", cksum,
					    (u_long) cp);
					pxe = NULL;
				}
			}
		}

		if (pxe != NULL && pxenv != NULL)
			break;
	}

	if (pxe == NULL && pxenv == NULL) {
		printf("pxe_init: No PXE BIOS found.\n");
		return (1);
	}

	if (pxenv != NULL) {
		printf("PXE BIOS Version %d.%d\n",
		    (pxenv->Version >> 8) & 0xff, pxenv->Version & 0xff);
		if (pxenv->Version >= 0x0201 && pxe != NULL) {
			/* 2.1 or greater -- don't use PXENV+ */
			pxenv = NULL;
		}
	}

	if (pxe != NULL) {
		pxe_call = pxecall_bangpxe;
		bangpxe_off = pxe->EntryPointSP.offset;
		bangpxe_seg = pxe->EntryPointSP.segment;
	} else {
		pxe_call = pxecall_pxenv;
		pxenv_off = pxenv->RMEntry.offset;
		pxenv_seg = pxenv->RMEntry.segment;
	}

	/*
	 * Pre-compute the segment/offset of the pxe_command_buf
	 * to make things nicer in the low-level calling glue.
	 */
	pxe_command_buf_seg = VTOPSEG(pxe_command_buf);
	pxe_command_buf_off = VTOPOFF(pxe_command_buf);

	/*
	 * Get the cached info from the server's Discovery reply packet.
	 */
	(void)memset(gci, 0, sizeof(*gci));
	gci->PacketType = PXENV_PACKET_TYPE_BINL_REPLY;
	pxe_call(PXENV_GET_CACHED_INFO);
	if (gci->Status != PXENV_STATUS_SUCCESS) {
		printf("pxe_init: PXENV_GET_CACHED_INFO failed: 0x%x\n",
		    gci->Status);
		return (1);
	}
	pvbcopy((void *)((gci->Buffer.segment << 4) + gci->Buffer.offset),
	    &bootplayer, gci->BufferSize);

	/*
	 * Get network interface information.
	 */
	(void)memset(gnt, 0, sizeof(*gnt));
	pxe_call(PXENV_UNDI_GET_NIC_TYPE);

	if (gnt->Status != PXENV_STATUS_SUCCESS) {
		printf("pxe_init: PXENV_UNDI_GET_NIC_TYPE failed: 0x%x\n",
		    gnt->Status);
		return (0);
	}

	switch (gnt->NicType) {
	case PCI_NIC:
	case CardBus_NIC:
		strncpy(bi_netif.ifname, "pxe", sizeof(bi_netif.ifname));
		bi_netif.bus = BI_BUS_PCI;
		bi_netif.addr.tag = gnt->info.pci.BusDevFunc;

		printf("Using %s device at bus %d device %d function %d\n",
		    gnt->NicType == PCI_NIC ? "PCI" : "CardBus",
		    (gnt->info.pci.BusDevFunc >> 8) & 0xff,
		    (gnt->info.pci.BusDevFunc >> 3) & 0x1f,
		    gnt->info.pci.BusDevFunc & 0x7);
		break;

	case PnP_NIC:
		/* XXX Make bootinfo work with this. */
		printf("Using PnP device at 0x%x\n", gnt->info.pnp.CardSelNum);
	}

	printf("Ethernet address %s\n", ether_sprintf(bootplayer.CAddr));

	return (0);
}

void
pxe_fini(void)
{
	t_PXENV_UNDI_SHUTDOWN *shutdown = (void *) pxe_command_buf;

	if (pxe_call == NULL)
		return;

	pxe_call(PXENV_UNDI_SHUTDOWN);

	if (shutdown->Status != PXENV_STATUS_SUCCESS)
		printf("pxe_fini: PXENV_UNDI_SHUTDOWN failed: 0x%x\n",
		    shutdown->Status);
}
