/*	$NetBSD: netif_sun.c,v 1.3.2.2 1996/01/31 16:57:25 gwr Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon W. Ross
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

/*
 * The Sun PROM has a fairly general set of network drivers,
 * so it is easiest to just replace the netif module with
 * this adaptation to the PROM network interface.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include <machine/control.h>
#include <machine/idprom.h>
#include <machine/mon.h>
#include <machine/saio.h>

#include "stand.h"
#include "net.h"
#include "netif.h"

#include "clock.h"
#include "dvma.h"
#include "promdev.h"

static struct netif netif_prom;
static void sun3_getether __P((u_char *));

#ifdef NETIF_DEBUG
int netif_debug;
#endif

struct saioreq net_ioreq;
struct iodesc sockets[SOPEN_MAX];
static char *tbuf;
static char *rbuf;
#define	BUFSIZE 0x800	/* 2K */

struct iodesc *
socktodesc(sock)
	int sock;
{
	if (sock != 0) {
		return(NULL);
	}
	return (sockets);
}

int
netif_open(machdep_hint)
	void *machdep_hint;
{
	struct bootparam *bp;
	struct saioreq *si;
	struct iodesc *io;
	int error;

	/* find a free socket */
	io = sockets;
	if (io->io_netif) {
#ifdef	DEBUG
		printf("netif_open: device busy\n");
#endif
		return (-1);
	}
	bzero(io, sizeof(*io));

	/*
	 * Setup our part of the saioreq.
	 * (determines what gets opened)
	 */
	si = &net_ioreq;
	bzero((caddr_t)si, sizeof(*si));
	bp = *romp->bootParam;

	si->si_boottab = bp->bootDevice;
	si->si_ctlr = bp->ctlrNum;
	si->si_unit = bp->unitNum;
	si->si_boff = bp->partNum;

	/*
	 * Note: Sun PROMs will do RARP on open, but does not tell
	 * you the IP address it gets, so it is just noise to us...
	 */
	if ((error = prom_iopen(si)) != 0) {
#ifdef	DEBUG
		printf("netif_open: prom_iopen, error=%d\n", error);
#endif
		return (-1);
	}
	if (si->si_sif == NULL) {
#ifdef	DEBUG
		printf("netif_open: not a network device\n");
#endif
		prom_iclose(si);
		return (-1);
	}

	netif_prom.devdata = si;
	io->io_netif = &netif_prom;

	/* Put our ethernet address in io->myea */
	sun3_getether(io->myea);

	/* Allocate the transmit/receive buffers. */
	if (tbuf == NULL) {
		tbuf = dvma_alloc(2 * BUFSIZE);
		if (tbuf == NULL)
			panic("netif_init: dvma_alloc failed\n");
		rbuf = tbuf + BUFSIZE;
	}

#ifdef	DEBUG
	printf("netif_init: tbuf=0x%x, rbuf=0x%x\n", tbuf, rbuf);
#endif

	return(0);
}

int
netif_close(fd)
	int fd;
{
	struct saioreq *si;
	struct iodesc *io;
	struct netif *ni;

	if (fd != 0) {
		errno = EBADF;
		return(-1);
	}

	io = sockets;
	ni = io->io_netif;
	if (ni != NULL) {
		si = ni->devdata;
		prom_iclose(si);
		ni->devdata = NULL;
		io->io_netif = NULL;
	}
	return(0);
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
int
netif_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	size_t len;
{
	struct saioreq *si;
	struct saif *sif;
	int rv, sendlen;

#ifdef NETIF_DEBUG
	if (netif_debug) {
		struct ether_header *eh;

		printf("netif_put: desc=0x%x pkt=0x%x len=%d\n",
			   desc, pkt, len);
		eh = pkt;
		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	si = desc->io_netif->devdata;
	sif = si->si_sif;
	sendlen = len;

	/*
	 * Copy into our transmit buffer because the PROM
	 * network driver might continue using the packet
	 * after the sif_xmit call returns.  We never send
	 * very much data anyway, so the copy is fine.
	 */
	bcopy(pkt, tbuf, sendlen);

	if (sendlen < 60) {
		sendlen = 60;
#ifdef NETIF_DEBUG
		printf("netif_put: length padded to %d\n", sendlen);
#endif
	}

#ifdef PARANOID
	if (sif == NULL)
		panic("netif_put: no saif ptr\n");
#endif

	rv = sif->sif_xmit(si->si_devdata, tbuf, sendlen);

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_put: xmit returned %d\n", rv);
#endif
	/*
	 * Just ignore the return value.  If the PROM transmit
	 * function fails, it will make some noise, such as:
	 *      le: No Carrier
	 */

	return len;
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
int
netif_get(desc, pkt, maxlen, timo)
	struct iodesc *desc;
	void *pkt;
	size_t maxlen;
	time_t timo;
{
	struct saioreq *si;
	struct saif *sif;
	int tick0, tmo_ticks;
	int len;

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_get: pkt=0x%x, maxlen=%d, tmo=%d\n",
			   pkt, maxlen, timo);
#endif

	si = desc->io_netif->devdata;
	sif = si->si_sif;

#ifdef PARANOID
	if (sif == NULL)
		panic("netif_get: no saif ptr\n");
#endif

	tmo_ticks = timo * hz;

	/* Have to receive into our own buffer and copy. */
	do {
		tick0 = getticks();
		do {
			len = (*sif->sif_poll)(si->si_devdata, rbuf);
			if (len != 0)
				goto break2;
		} while (getticks() == tick0);
	} while (--tmo_ticks > 0);

	/* No packet arrived.  Better reset the interface. */
	printf("netif_get: timeout; resetting\n");
	(*sif->sif_reset)(si->si_devdata, si);

break2:

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_get: received len=%d\n", len);
#endif

	if (len < 12)
		return -1;

	/* The caller's buffer may be smaller... */
	if (len > maxlen)
		len = maxlen;

	bcopy(rbuf, pkt, len);

#ifdef NETIF_DEBUG
	if (netif_debug) {
		struct ether_header *eh = pkt;

		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	return len;
}

static struct idprom sun3_idprom;

static void
sun3_getether(ea)
	u_char *ea;
{
	u_char *src, *dst;
	int len, x;

	if (sun3_idprom.idp_format == 0) {
		dst = (char*)&sun3_idprom;
		src = (char*)IDPROM_BASE;
		len = IDPROM_SIZE;
		do {
			x = get_control_byte(src++);
			*dst++ = x;
		} while (--len > 0);
	}
	MACPY(sun3_idprom.idp_etheraddr, ea);
}

