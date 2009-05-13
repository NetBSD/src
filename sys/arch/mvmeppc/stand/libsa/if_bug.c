/*	$NetBSD: if_bug.c,v 1.3.2.1 2009/05/13 17:18:09 jym Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include "libsa.h"
#include "bugsyscalls.h"

static int	bug_match(struct netif *, void *);
static int	bug_probe(struct netif *, void *);
static void	bug_init(struct iodesc *, void *);
static int	bug_get(struct iodesc *, void *, size_t, saseconds_t);
static int	bug_put(struct iodesc *, void *, size_t);
static void	bug_end(struct netif *);

static struct netif_stats bug_stats;

static struct netif_dif bug0_dif = {
	0, 1, &bug_stats, 0, 0
};

struct netif_driver bug_driver = {
	"net",			/* netif_bname */
	bug_match,		/* match */
	bug_probe,		/* probe */
	bug_init,		/* init */
	bug_get,		/* get */
	bug_put,		/* put */
	bug_end,		/* end */
	&bug0_dif,		/* netif_ifs */
	1,			/* netif_nifs */
};

struct bug_softc {
	u_int32_t	sc_pad1;
	u_int8_t	sc_rxbuf[ETHER_MAX_LEN];
	u_int32_t	sc_pad2;
	u_int8_t	sc_txbuf[ETHER_MAX_LEN];
};

static struct bug_softc bug_softc;

int
bug_match(struct netif *nif, void *machdep_hint)
{

	if (machdep_hint &&
	    memcmp(bug_driver.netif_bname, machdep_hint,
	           strlen(bug_driver.netif_bname)) == 0)
		return (1);

	return (0);
}

int
bug_probe(struct netif *nif, void *machdep_hint)
{

	return (0);
}

void
bug_init(struct iodesc *desc, void *machdep_hint)
{
	struct netif *nif = desc->io_netif;
	struct bug_netio nio;

	nio.nc_clun = 0;
	nio.nc_dlun = 0;
	nio.nc_status = 0;
	nio.nc_command = BUG_NETIO_CMD_GET_MAC;
	nio.nc_buffer = desc->myea;
	nio.nc_length = 6;
	nio.nc_csr = 0;

	if (bugsys_netio(&nio) != 0 || nio.nc_status != 0)
		panic("bug_init: Failed to get MAC address! (code 0x%x)",
		    nio.nc_status);

	nio.nc_clun = 0;
	nio.nc_dlun = 0;
	nio.nc_status = 0;
	nio.nc_command = BUG_NETIO_CMD_FLUSH;
	nio.nc_buffer = NULL;
	nio.nc_length = 0;
	nio.nc_csr = 0;

	if (bugsys_netio(&nio) != 0 || nio.nc_status != 0)
		panic("bug_init: Failed to flush netio device (code 0x%x)",
		    nio.nc_status);

	printf("network: %s%d attached to %s\n", nif->nif_driver->netif_bname,
	    nif->nif_unit, ether_sprintf(desc->myea));

	nif->nif_devdata = &bug_softc;
}

int
bug_get(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
	struct netif *nif = desc->io_netif;
	struct bug_softc *sc = nif->nif_devdata;
	struct bug_netio nio;

	nio.nc_clun = 0;
	nio.nc_dlun = 0;
	nio.nc_status = 0;
	nio.nc_command = BUG_NETIO_CMD_RECEIVE;
	nio.nc_buffer = sc->sc_rxbuf;
	nio.nc_length = ETHER_MAX_LEN;
	nio.nc_csr = 0;

	if (bugsys_netio(&nio) != 0 || nio.nc_status != 0) {
		printf("bug_get: Receive packet failed (code: 0x%x)\n",
		    nio.nc_status);
		return (0);
	}

	if (nio.nc_length) {
		memcpy(pkt, sc->sc_rxbuf, MIN(len, nio.nc_length));
		return (MIN(len, nio.nc_length));
	}

	return (0);
}

int
bug_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct netif *nif = desc->io_netif;
	struct bug_softc *sc = nif->nif_devdata;
	struct bug_netio nio;

	memcpy(&sc->sc_txbuf, pkt, len);

	nio.nc_clun = 0;
	nio.nc_dlun = 0;
	nio.nc_status = 0;
	nio.nc_command = BUG_NETIO_CMD_TRANSMIT;
	nio.nc_buffer = sc->sc_txbuf;
	nio.nc_length = MAX(len, ETHER_MIN_LEN);
	nio.nc_csr = 0;

	if (bugsys_netio(&nio) != 0 || nio.nc_status != 0) {
		printf("bug_put: Send packet failed (code: 0x%x)\n",
		    nio.nc_status);
		return (0);
	}

	return (len);
}

void
bug_end(struct netif *nif)
{
	struct bug_netio nio;

	nio.nc_clun = 0;
	nio.nc_dlun = 0;
	nio.nc_status = 0;
	nio.nc_command = BUG_NETIO_CMD_FLUSH;
	nio.nc_buffer = NULL;
	nio.nc_length = 0;
	nio.nc_csr = 0;

	if (bugsys_netio(&nio) != 0 || nio.nc_status != 0)
		printf("bug_end: netio failed (code: 0x%x)\n", nio.nc_status);
}
