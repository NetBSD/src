/*	$NetBSD: netif_of.c,v 1.7.6.1 2011/06/06 09:06:48 jruoho Exp $	*/

/*
 * Copyright (C) 1995 Wolfgang Solfrank.
 * Copyright (C) 1995 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Open Firmware does most of the job for interfacing to the hardware,
 * so it is easiest to just replace the netif module with
 * this adaptation to the PROM network interface.
 *
 * Note: this is based in part on sys/arch/sparc/stand/netif_sun.c
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include <machine/promlib.h>

#include "ofdev.h"

static struct netif netif_of;

struct iodesc sockets[SOPEN_MAX];

struct iodesc *
socktodesc(int sock)
{
	if (sock != 0)
		return NULL;
	return sockets;
}

int
netif_open(void *machdep_hint)
{
	struct of_dev *op = machdep_hint;
	struct iodesc *io;
	
#ifdef	NETIF_DEBUG
	printf("netif_open...");
#endif
	/* find a free socket */
	io = sockets;
	if (io->io_netif) {
#ifdef	NETIF_DEBUG
		printf("device busy\n");
#endif
		errno = ENFILE;
		return -1;
	}
	memset(io, 0, sizeof *io);

	netif_of.nif_devdata = op;
	io->io_netif = &netif_of;
	
	/* Put our ethernet address in io->myea */
	_prom_getprop(prom_instance_to_package(op->handle),
		   "mac-address", io->myea, sizeof io->myea);

#ifdef	NETIF_DEBUG
	printf("OK\n");
#endif
	return 0;
}

int
netif_close(int fd)
{
	struct iodesc *io;
	struct netif *ni;

#ifdef	NETIF_DEBUG
	printf("netif_close(%x)...", fd);
#endif
	if (fd != 0) {
#ifdef	NETIF_DEBUG
		printf("EBADF\n");
#endif
		errno = EBADF;
		return -1;
	}

	io = &sockets[fd];
	ni = io->io_netif;
	if (ni != NULL) {
		ni->nif_devdata = NULL;
		io->io_netif = NULL;
	}
#ifdef	NETIF_DEBUG
	printf("OK\n");
#endif
	return 0;
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
ssize_t
netif_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct of_dev *op;
	ssize_t rv;
	size_t sendlen;

	op = ((struct netif *)desc->io_netif)->nif_devdata;

#ifdef	NETIF_DEBUG
	{
		struct ether_header *eh;

		printf("netif_put: desc=0x%x pkt=0x%x len=%d\n",
		       desc, pkt, len);
		eh = pkt;
		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	sendlen = len;
	if (sendlen < 60) {
		sendlen = 60;
#ifdef	NETIF_DEBUG
		printf("netif_put: length padded to %d\n", sendlen);
#endif
	}

	rv = prom_write(op->handle, pkt, sendlen);

#ifdef	NETIF_DEBUG
	printf("netif_put: xmit returned %d\n", rv);
#endif

	if (rv > len)
		rv = len;

	return rv;
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
ssize_t
netif_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timo)
{
	struct of_dev *op;
	int tick0, tmo_ms;
	int len;

	op = ((struct netif *)desc->io_netif)->nif_devdata;

#ifdef	NETIF_DEBUG
	printf("netif_get: pkt=0x%x, maxlen=%d, tmo=%d\n",
	       pkt, maxlen, timo);
#endif

	tmo_ms = timo * 1000;
	tick0 = prom_ticks();

	do {
		len = prom_read(op->handle, pkt, maxlen);
	} while ((len == -2 || len == 0) &&
		 (prom_ticks() - tick0 < tmo_ms));

#ifdef	NETIF_DEBUG
	printf("netif_get: received len=%d\n", len);
#endif

	if (len < 12)
		return -1;

#ifdef	NETIF_DEBUG
	{
		struct ether_header *eh = pkt;

		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	return len;
}

/*
 * Shouldn't really be here, but is used solely for networking, so...
 */
satime_t
getsecs(void)
{

	return prom_ticks() / 1000;
}
