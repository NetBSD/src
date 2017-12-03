/*	$NetBSD: can_proto.c,v 1.2.10.2 2017/12/03 11:39:03 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Robert Swindells and Manuel Bouyer
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: can_proto.c,v 1.2.10.2 2017/12/03 11:39:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/radix.h>
#include <net/route.h>

/*
 * CAN protocol family
 */
#include <netcan/can.h>
#include <netcan/can_var.h>

DOMAIN_DEFINE(candomain);	/* forward declare and add to link set */

const struct protosw cansw[] = {
{
	.pr_type = SOCK_RAW,
	.pr_domain = &candomain,
	.pr_init = can_init,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_usrreqs = &can_usrreqs,
	.pr_ctloutput = &can_ctloutput,
}
};

struct domain candomain = {
	.dom_family = PF_CAN,
	.dom_name = "can",
	.dom_init = can_init,
	.dom_externalize = NULL, .dom_dispose = NULL,
	.dom_protosw = cansw,
	.dom_protoswNPROTOSW = &cansw[__arraycount(cansw)],
	.dom_ifqueues = { &canintrq, NULL },
	.dom_link = { NULL },
	.dom_mowner = MOWNER_INIT("",""),
	.dom_sa_cmpofs = offsetof(struct sockaddr_can, can_ifindex),
	.dom_sa_cmplen = sizeof(int)
};
