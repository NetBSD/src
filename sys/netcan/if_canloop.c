/*	$NetBSD: if_canloop.c,v 1.1.2.3 2017/02/05 11:45:11 bouyer Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 * Loopback interface driver for the CAN protocol
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_canloop.c,v 1.1.2.3 2017/02/05 11:45:11 bouyer Exp $");

#ifdef _KERNEL_OPT
#include "opt_can.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/device.h>
#include <sys/module.h>

#include <sys/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>

#ifdef	CAN
#include <netcan/can.h>
#endif
#include <net/bpf.h>

void canloopattach(int);
void canloopinit(void);
static int	canloop_clone_create(struct if_clone *, int);
static int	canloop_clone_destroy(struct ifnet *);
static int	canloop_ioctl(struct ifnet *, u_long, void *);
static int	canloop_output(struct ifnet *,
	struct mbuf *, const struct sockaddr *, const struct rtentry *);

static int	canloop_count;

static struct if_clone canloop_cloner =
    IF_CLONE_INITIALIZER("canlo", canloop_clone_create, canloop_clone_destroy);

void
canloopattach(int n)
{

	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in canloopinit() below).
	 */
}

void
canloopinit(void)
{

	canloop_count = 0;
	if_clone_attach(&canloop_cloner);
}

static int
canloopdetach(void)
{
	if (canloop_count > 0)
		return EBUSY;
	if_clone_detach(&canloop_cloner);
	return 0;
}

static int
canloop_clone_create(struct if_clone *ifc, int unit)
{
	struct ifnet *ifp;

	ifp = if_alloc(IFT_OTHER);

	if_initname(ifp, ifc->ifc_name, unit);

	ifp->if_mtu = sizeof(struct can_frame);
	ifp->if_flags = IFF_LOOPBACK | IFF_RUNNING;
	ifp->if_extflags = IFEF_OUTPUT_MPSAFE;
	ifp->if_ioctl = canloop_ioctl;
	ifp->if_output = canloop_output;
	ifp->if_type = IFT_OTHER;
	ifp->if_hdrlen = 0;
	ifp->if_addrlen = 0;
	ifp->if_dlt = DLT_CAN_SOCKETCAN;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_alloc_sadl(ifp);
	bpf_attach(ifp, DLT_CAN_SOCKETCAN, sizeof(u_int));
#ifdef MBUFTRACE
	ifp->if_mowner = malloc(sizeof(struct mowner), M_DEVBUF,
	    M_WAITOK | M_ZERO);
	strlcpy(ifp->if_mowner->mo_name, ifp->if_xname,
	    sizeof(ifp->if_mowner->mo_name));
	MOWNER_ATTACH(ifp->if_mowner);
#endif
	canloop_count++;

	return (0);
}

static int
canloop_clone_destroy(struct ifnet *ifp)
{

#ifdef MBUFTRACE
	MOWNER_DETACH(ifp->if_mowner);
	free(ifp->if_mowner, M_DEVBUF);
#endif

	bpf_detach(ifp);
	if_detach(ifp);

	if_free(ifp);
	canloop_count--;
	KASSERT(canloop_count >= 0);
	return (0);
}

static int
canloop_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	int error = 0;
	size_t pktlen;

	MCLAIM(m, ifp->if_mowner);

	KERNEL_LOCK(1, NULL);

	if ((m->m_flags & M_PKTHDR) == 0)
		panic("canloop_output: no header mbuf");
	if (ifp->if_flags & IFF_LOOPBACK)
		bpf_mtap_af(ifp, AF_CAN, m);
	m_set_rcvif(m, ifp);

	pktlen = m->m_pkthdr.len;
	ifp->if_opackets++;
	ifp->if_obytes += pktlen;

#ifdef CAN
	can_mbuf_tag_clean(m);
	can_input(ifp, m);
#else
	printf("%s: can't handle CAN packet\n", ifp->if_xname);
	m_freem(m);
	error = EAFNOSUPPORT;
#endif

	KERNEL_UNLOCK_ONE(NULL);
	return error;
}


/*
 * Process an ioctl request.
 */
/* ARGSUSED */
static int
canloop_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifreq *ifr = data;
	int error = 0;

	switch (cmd) {

	case SIOCINITIFADDR:
		error = EAFNOSUPPORT;
		break;

	case SIOCSIFMTU:
		if ((unsigned)ifr->ifr_mtu != sizeof(struct can_frame))
			error = EINVAL;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = EAFNOSUPPORT;
		break;

	default:
		error = ifioctl_common(ifp, cmd, data);
	}
	return (error);
}

/*
 * Module infrastructure
 */
#include "../net/if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, canloop, "")
