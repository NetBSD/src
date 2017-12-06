/*	$NetBSD: if_canloop.c,v 1.4 2017/12/06 07:40:16 ozaki-r Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_canloop.c,v 1.4 2017/12/06 07:40:16 ozaki-r Exp $");

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

void canloopattach(int);
void canloopinit(void);
static int	canloop_clone_create(struct if_clone *, int);
static int	canloop_clone_destroy(struct ifnet *);
static int	canloop_ioctl(struct ifnet *, u_long, void *);
static void	canloop_ifstart(struct ifnet *);

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

	ifp->if_flags = IFF_LOOPBACK;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = canloop_ioctl;
	ifp->if_start = canloop_ifstart;
	can_ifattach(ifp);
#ifdef MBUFTRACE
	ifp->if_mowner = malloc(sizeof(struct mowner), M_DEVBUF,
	    M_WAITOK | M_ZERO);
	strlcpy(ifp->if_mowner->mo_name, ifp->if_xname,
	    sizeof(ifp->if_mowner->mo_name));
	MOWNER_ATTACH(ifp->if_mowner);
#endif
	canloop_count++;
	ifp->if_flags |= IFF_RUNNING;

	return (0);
}

static int
canloop_clone_destroy(struct ifnet *ifp)
{

	ifp->if_flags &= ~IFF_RUNNING;

#ifdef MBUFTRACE
	MOWNER_DETACH(ifp->if_mowner);
	free(ifp->if_mowner, M_DEVBUF);
#endif

	can_ifdetach(ifp);

	if_free(ifp);
	canloop_count--;
	KASSERT(canloop_count >= 0);
	return (0);
}

static void
canloop_ifstart(struct ifnet *ifp)
{
	size_t pktlen;
	struct mbuf *m;

	KERNEL_LOCK(1, NULL);
	while (true) {
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
		MCLAIM(m, ifp->if_mowner);

		if ((m->m_flags & M_PKTHDR) == 0)
			panic("canloop_output: no header mbuf");
		m_set_rcvif(m, ifp);
		if (ifp->if_flags & IFF_LOOPBACK)
			can_bpf_mtap(ifp, m, 0);

		pktlen = m->m_pkthdr.len;
		ifp->if_opackets++;
		ifp->if_obytes += pktlen;

#ifdef CAN
		can_mbuf_tag_clean(m);
		can_input(ifp, m);
#else
		printf("%s: can't handle CAN packet\n", ifp->if_xname);
		m_freem(m);
#endif
	}

	KERNEL_UNLOCK_ONE(NULL);
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
