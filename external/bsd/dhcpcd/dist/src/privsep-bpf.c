/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation BPF Initiator
 * Copyright (c) 2006-2020 Roy Marples <roy@marples.name>
 * All rights reserved

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

#include <sys/socket.h>
#include <sys/types.h>

/* Need these headers just for if_ether on some OS. */
#ifndef __NetBSD__
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#endif
#include <netinet/if_ether.h>

#include <assert.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arp.h"
#include "bpf.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "eloop.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "privsep.h"

static void
ps_bpf_recvbpf(void *arg)
{
	struct ps_process *psp = arg;
	unsigned int flags;
	uint8_t buf[FRAMELEN_MAX];
	ssize_t len;
	struct ps_msghdr psm = {
		.ps_id = psp->psp_id,
		.ps_cmd = psp->psp_id.psi_cmd,
	};

	/* A BPF read can read more than one filtered packet at time.
	 * This mechanism allows us to read each packet from the buffer. */
	flags = 0;
	while (!(flags & BPF_EOF)) {
		len = bpf_read(&psp->psp_ifp, psp->psp_work_fd,
		    buf, sizeof(buf), &flags);
		if (len == -1)
			logerr(__func__);
		if (len == -1 || len == 0)
			break;
		len = ps_sendpsmdata(psp->psp_ctx, psp->psp_ctx->ps_data_fd,
		    &psm, buf, (size_t)len);
		if (len == -1 && errno != ECONNRESET)
			logerr(__func__);
		if (len == -1 || len == 0)
			break;
	}
}

#ifdef ARP
static ssize_t
ps_bpf_arp_addr(uint8_t cmd, struct ps_process *psp, struct msghdr *msg)
{
	struct interface *ifp = &psp->psp_ifp;
	struct iovec *iov = msg->msg_iov;
	struct in_addr addr;
	struct arp_state *astate;

	if (psp == NULL) {
		errno = ESRCH;
		return -1;
	}

	assert(msg->msg_iovlen == 1);
	assert(iov->iov_len == sizeof(addr));
	memcpy(&addr, iov->iov_base, sizeof(addr));
	if (cmd & PS_START) {
		astate = arp_new(ifp, &addr);
		if (astate == NULL)
			return -1;
	} else if (cmd & PS_DELETE) {
		astate = arp_find(ifp, &addr);
		if (astate == NULL) {
			errno = ESRCH;
			return -1;
		}
		arp_free(astate);
	} else {
		errno = EINVAL;
		return -1;
	}

	return bpf_arp(ifp, psp->psp_work_fd);
}
#endif

static ssize_t
ps_bpf_recvmsgcb(void *arg, struct ps_msghdr *psm, struct msghdr *msg)
{
	struct ps_process *psp = arg;
	struct iovec *iov = msg->msg_iov;

#ifdef ARP
	if (psm->ps_cmd & (PS_START | PS_DELETE))
		return ps_bpf_arp_addr(psm->ps_cmd, psp, msg);
#endif

	return bpf_send(&psp->psp_ifp, psp->psp_work_fd, psp->psp_proto,
	    iov->iov_base, iov->iov_len);
}

static void
ps_bpf_recvmsg(void *arg)
{
	struct ps_process *psp = arg;

	if (ps_recvpsmsg(psp->psp_ctx, psp->psp_fd,
	    ps_bpf_recvmsgcb, arg) == -1)
		logerr(__func__);
}

static int
ps_bpf_start_bpf(void *arg)
{
	struct ps_process *psp = arg;
	struct dhcpcd_ctx *ctx = psp->psp_ctx;

	setproctitle("[BPF %s] %s", psp->psp_protostr, psp->psp_ifname);

	ps_freeprocesses(ctx, psp);

	psp->psp_work_fd = bpf_open(&psp->psp_ifp, psp->psp_filter);
	if (psp->psp_work_fd == -1)
		logerr("%s: bpf_open",__func__);
	else if (eloop_event_add(ctx->eloop,
	    psp->psp_work_fd, ps_bpf_recvbpf, psp) == -1)
		logerr("%s: eloop_event_add", __func__);
	else
		return 0;

	eloop_exit(ctx->eloop, EXIT_FAILURE);
	return -1;
}

static void
ps_bpf_signal_bpfcb(int sig, void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	eloop_exit(ctx->eloop, sig == SIGTERM ? EXIT_SUCCESS : EXIT_FAILURE);
}

ssize_t
ps_bpf_cmd(struct dhcpcd_ctx *ctx, struct ps_msghdr *psm, struct msghdr *msg)
{
	uint8_t cmd;
	struct ps_process *psp;
	pid_t start;
	struct iovec *iov = msg->msg_iov;
	struct interface *ifp;
	struct ipv4_state *istate;

	cmd = (uint8_t)(psm->ps_cmd & ~(PS_START | PS_STOP));
	psp = ps_findprocess(ctx, &psm->ps_id);

#ifdef PRIVSEP_DEBUG
	logerrx("%s: IN cmd %x, psp %p", __func__, psm->ps_cmd, psp);
#endif

	switch (cmd) {
#ifdef ARP
	case PS_BPF_ARP:	/* FALLTHROUGH */
#endif
	case PS_BPF_BOOTP:
		break;
	default:
		logerrx("%s: unknown command %x", __func__, psm->ps_cmd);
		errno = ENOTSUP;
		return -1;
	}

	if (!(psm->ps_cmd & PS_START)) {
		errno = EINVAL;
		return -1;
	}

	if (psp != NULL)
		return 1;

	psp = ps_newprocess(ctx, &psm->ps_id);
	if (psp == NULL)
		return -1;

	ifp = &psp->psp_ifp;
	assert(msg->msg_iovlen == 1);
	assert(iov->iov_len == sizeof(*ifp));
	memcpy(ifp, iov->iov_base, sizeof(*ifp));
	ifp->ctx = psp->psp_ctx;
	ifp->options = NULL;
	memset(ifp->if_data, 0, sizeof(ifp->if_data));

	if ((istate = ipv4_getstate(ifp)) == NULL) {
		ps_freeprocess(psp);
		return -1;
	}

	memcpy(psp->psp_ifname, ifp->name, sizeof(psp->psp_ifname));

	switch (cmd) {
#ifdef ARP
	case PS_BPF_ARP:
		psp->psp_proto = ETHERTYPE_ARP;
		psp->psp_protostr = "ARP";
		psp->psp_filter = bpf_arp;
		break;
#endif
	case PS_BPF_BOOTP:
		psp->psp_proto = ETHERTYPE_IP;
		psp->psp_protostr = "BOOTP";
		psp->psp_filter = bpf_bootp;
		break;
	}

	start = ps_dostart(ctx,
	    &psp->psp_pid, &psp->psp_fd,
	    ps_bpf_recvmsg, NULL, psp,
	    ps_bpf_start_bpf, ps_bpf_signal_bpfcb, PSF_DROPPRIVS);
	switch (start) {
	case -1:
		ps_freeprocess(psp);
		return -1;
	case 0:
		break;
	default:
#ifdef PRIVSEP_DEBUG
		logdebugx("%s: spawned BPF %s on PID %d",
		    psp->psp_ifname, psp->psp_protostr, start);
#endif
		break;
	}
	return start;
}

ssize_t
ps_bpf_dispatch(struct dhcpcd_ctx *ctx,
    struct ps_msghdr *psm, struct msghdr *msg)
{
	struct iovec *iov = msg->msg_iov;
	struct interface *ifp;

	ifp = if_findindex(ctx->ifaces, psm->ps_id.psi_ifindex);

	switch (psm->ps_cmd) {
#ifdef ARP
	case PS_BPF_ARP:
		arp_packet(ifp, iov->iov_base, iov->iov_len);
		break;
#endif
	case PS_BPF_BOOTP:
		dhcp_packet(ifp, iov->iov_base, iov->iov_len);
		break;
	default:
		errno = ENOTSUP;
		return -1;
	}

	return 1;
}

static ssize_t
ps_bpf_send(const struct interface *ifp, uint8_t cmd,
    const void *data, size_t len)
{
	struct dhcpcd_ctx *ctx = ifp->ctx;
	struct ps_msghdr psm = {
		.ps_cmd = cmd,
		.ps_id = {
			.psi_ifindex = ifp->index,
			.psi_cmd = (uint8_t)(cmd &
			    ~(PS_START | PS_STOP | PS_DELETE)),
		},
	};

	if (psm.ps_id.psi_cmd == PS_BPF_ARP_ADDR)
		psm.ps_id.psi_cmd = PS_BPF_ARP;

	return ps_sendpsmdata(ctx, ctx->ps_root_fd, &psm, data, len);
}

#ifdef ARP
ssize_t
ps_bpf_openarp(const struct interface *ifp)
{

	return ps_bpf_send(ifp, PS_BPF_ARP | PS_START, ifp, sizeof(*ifp));
}

ssize_t
ps_bpf_addaddr(const struct interface *ifp, const struct in_addr *addr)
{

	return ps_bpf_send(ifp, PS_BPF_ARP_ADDR | PS_START, addr, sizeof(*addr));
}

ssize_t
ps_bpf_deladdr(const struct interface *ifp, const struct in_addr *addr)
{

	return ps_bpf_send(ifp, PS_BPF_ARP_ADDR | PS_DELETE, addr, sizeof(*addr));
}

ssize_t
ps_bpf_closearp(const struct interface *ifp)
{

	return ps_bpf_send(ifp, PS_BPF_ARP | PS_STOP, NULL, 0);
}

ssize_t
ps_bpf_sendarp(const struct interface *ifp, const void *data, size_t len)
{

	return ps_bpf_send(ifp, PS_BPF_ARP, data, len);
}
#endif

ssize_t
ps_bpf_openbootp(const struct interface *ifp)
{

	return ps_bpf_send(ifp, PS_BPF_BOOTP | PS_START, ifp, sizeof(*ifp));
}

ssize_t
ps_bpf_closebootp(const struct interface *ifp)
{

	return ps_bpf_send(ifp, PS_BPF_BOOTP | PS_STOP, NULL, 0);
}

ssize_t
ps_bpf_sendbootp(const struct interface *ifp, const void *data, size_t len)
{

	return ps_bpf_send(ifp, PS_BPF_BOOTP, data, len);
}
