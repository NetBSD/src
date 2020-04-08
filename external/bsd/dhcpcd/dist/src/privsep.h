/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation for dhcpcd
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

#ifndef PRIVSEP_H
#define PRIVSEP_H

//#define PRIVSEP_DEBUG

/* Start flags */
#define	PSF_DROPPRIVS		0x01

/* Commands */
#define	PS_BOOTP		0x01
#define	PS_ND			0x02
#define	PS_DHCP6		0x03
#define	PS_BPF_BOOTP		0x04
#define	PS_BPF_ARP		0x05
#define	PS_BPF_ARP_ADDR		0x06

#define	PS_IOCTL		0x10
#define	PS_ROUTE		0x11	/* Also used for NETLINK */
#define	PS_SCRIPT		0x12
#define	PS_UNLINK		0x13
#define	PS_COPY			0x14

/* BSD Commands */
#define	PS_IOCTLLINK		0x15
#define	PS_IOCTL6		0x16

/* Linux commands */
#define	PS_WRITEPATHUINT	0x17

#define	PS_DELETE		0x20
#define	PS_START		0x40
#define	PS_STOP			0x80

/* Max INET message size + meta data for IPC */
#define	PS_BUFLEN		((64 * 1024) +			\
				 sizeof(struct ps_msghdr) +	\
				 sizeof(struct msghdr) +	\
				 CMSG_SPACE(sizeof(struct in6_pktinfo) + \
				            sizeof(int)))

/* Handy macro to work out if in the privsep engine or not. */
#define	IN_PRIVSEP(ctx)	\
	((ctx)->options & DHCPCD_PRIVSEP)
#define	IN_PRIVSEP_SE(ctx)	\
	(((ctx)->options & (DHCPCD_PRIVSEP | DHCPCD_FORKED)) == DHCPCD_PRIVSEP)

#include "config.h"
#include "arp.h"
#include "dhcp.h"
#include "dhcpcd.h"

struct ps_addr {
	sa_family_t psa_family;
	uint8_t psa_pad[4 - sizeof(sa_family_t)];
	union {
		struct in_addr psau_in_addr;
		struct in6_addr psau_in6_addr;
	} psa_u;
#define	psa_in_addr	psa_u.psau_in_addr
#define	psa_in6_addr	psa_u.psau_in6_addr
};

/* Uniquely identify a process */
struct ps_id {
	struct ps_addr psi_addr;
	unsigned int psi_ifindex;
	uint8_t psi_cmd;
	uint8_t psi_pad[3];
};

struct ps_msghdr {
	uint8_t ps_cmd;
	uint8_t ps_pad[sizeof(unsigned long) - 1];
	unsigned long ps_flags;
	struct ps_id ps_id;
	socklen_t ps_namelen;
	socklen_t ps_controllen;
	uint8_t ps_pad2[sizeof(size_t) - sizeof(socklen_t)];
	size_t ps_datalen;
};

struct ps_msg {
	struct ps_msghdr psm_hdr;
	uint8_t psm_data[PS_BUFLEN];
};

struct ps_process {
	TAILQ_ENTRY(ps_process) next;
	struct dhcpcd_ctx *psp_ctx;
	struct ps_id psp_id;
	pid_t psp_pid;
	int psp_fd;
	int psp_work_fd;
	unsigned int psp_ifindex;
	char psp_ifname[IF_NAMESIZE];
	uint16_t psp_proto;
	const char *psp_protostr;

#ifdef INET
	int (*psp_filter)(struct interface *, int);
	struct interface psp_ifp; /* Move BPF gubbins elsewhere */
#endif
};
TAILQ_HEAD(ps_process_head, ps_process);

#include "privsep-inet.h"
#include "privsep-root.h"
#ifdef INET
#include "privsep-bpf.h"
#endif

int ps_mkdir(char *);
int ps_init(struct dhcpcd_ctx *);
int ps_dropprivs(struct dhcpcd_ctx *);
int ps_start(struct dhcpcd_ctx *);
int ps_stop(struct dhcpcd_ctx *);

int ps_unrollmsg(struct msghdr *, struct ps_msghdr *, const void *, size_t);
ssize_t ps_sendpsmmsg(struct dhcpcd_ctx *, int,
    struct ps_msghdr *, const struct msghdr *);
ssize_t ps_sendpsmdata(struct dhcpcd_ctx *, int,
    struct ps_msghdr *, const void *, size_t);
ssize_t ps_sendmsg(struct dhcpcd_ctx *, int, uint8_t, unsigned long,
    const struct msghdr *);
ssize_t ps_sendcmd(struct dhcpcd_ctx *, int, uint8_t, unsigned long,
    const void *data, size_t len);
ssize_t ps_recvmsg(struct dhcpcd_ctx *, int, uint8_t, int);
ssize_t ps_recvpsmsg(struct dhcpcd_ctx *, int,
    ssize_t (*callback)(void *, struct ps_msghdr *, struct msghdr *), void *);

/* Internal privsep functions. */
pid_t ps_dostart(struct dhcpcd_ctx * ctx,
    pid_t *priv_pid, int *priv_fd,
    void (*recv_msg)(void *), void (*recv_unpriv_msg),
    void *recv_ctx, int (*callback)(void *), void (*)(int, void *),
    unsigned int);
int ps_dostop(struct dhcpcd_ctx *ctx, pid_t *pid, int *fd);

struct ps_process *ps_findprocess(struct dhcpcd_ctx *, struct ps_id *);
struct ps_process *ps_newprocess(struct dhcpcd_ctx *, struct ps_id *);
void ps_freeprocess(struct ps_process *);
void ps_freeprocesses(struct dhcpcd_ctx *, struct ps_process *);
#endif
