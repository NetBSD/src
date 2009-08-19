/*	$NetBSD: bt_proto.c,v 1.10.2.1 2009/08/19 18:48:24 yamt Exp $	*/

/*-
 * Copyright (c) 2005 Iain Hibbert.
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bt_proto.c,v 1.10.2.1 2009/08/19 18:48:24 yamt Exp $");

#include <sys/param.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <net/route.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>
#include <netbt/l2cap.h>
#include <netbt/rfcomm.h>
#include <netbt/sco.h>

DOMAIN_DEFINE(btdomain);	/* forward declare and add to link set */

static void	bt_init(void);

PR_WRAP_CTLOUTPUT(hci_ctloutput)
PR_WRAP_CTLOUTPUT(sco_ctloutput)
PR_WRAP_CTLOUTPUT(l2cap_ctloutput)
PR_WRAP_CTLOUTPUT(rfcomm_ctloutput)

#define	hci_ctloutput		hci_ctloutput_wrapper
#define	sco_ctloutput		sco_ctloutput_wrapper
#define	l2cap_ctloutput		l2cap_ctloutput_wrapper
#define	rfcomm_ctloutput	rfcomm_ctloutput_wrapper

PR_WRAP_USRREQ(hci_usrreq)
PR_WRAP_USRREQ(sco_usrreq)
PR_WRAP_USRREQ(l2cap_usrreq)
PR_WRAP_USRREQ(rfcomm_usrreq)

#define	hci_usrreq		hci_usrreq_wrapper
#define	sco_usrreq		sco_usrreq_wrapper
#define	l2cap_usrreq		l2cap_usrreq_wrapper
#define	rfcomm_usrreq		rfcomm_usrreq_wrapper

const struct protosw btsw[] = {
	{ /* raw HCI commands */
		.pr_type = SOCK_RAW,
		.pr_domain = &btdomain,
		.pr_protocol = BTPROTO_HCI,
		.pr_flags = (PR_ADDR | PR_ATOMIC),
		.pr_init = hci_init,
		.pr_ctloutput = hci_ctloutput,
		.pr_usrreq = hci_usrreq,
	},
	{ /* HCI SCO data (audio) */
		.pr_type = SOCK_SEQPACKET,
		.pr_domain = &btdomain,
		.pr_protocol = BTPROTO_SCO,
		.pr_flags = (PR_CONNREQUIRED | PR_ATOMIC | PR_LISTEN),
		.pr_ctloutput = sco_ctloutput,
		.pr_usrreq = sco_usrreq,
	},
	{ /* L2CAP Connection Oriented */
		.pr_type = SOCK_SEQPACKET,
		.pr_domain = &btdomain,
		.pr_protocol = BTPROTO_L2CAP,
		.pr_flags = (PR_CONNREQUIRED | PR_ATOMIC | PR_LISTEN),
		.pr_ctloutput = l2cap_ctloutput,
		.pr_usrreq = l2cap_usrreq,
	},
	{ /* RFCOMM */
		.pr_type = SOCK_STREAM,
		.pr_domain = &btdomain,
		.pr_protocol = BTPROTO_RFCOMM,
		.pr_flags = (PR_CONNREQUIRED | PR_LISTEN | PR_WANTRCVD),
		.pr_ctloutput = rfcomm_ctloutput,
		.pr_usrreq = rfcomm_usrreq,
	},
};

struct domain btdomain = {
	.dom_family = AF_BLUETOOTH,
	.dom_name = "bluetooth",
	.dom_init = bt_init,
	.dom_protosw = btsw,
	.dom_protoswNPROTOSW = &btsw[__arraycount(btsw)],
};

kmutex_t *bt_lock;

static void
bt_init(void)
{

	bt_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
}
