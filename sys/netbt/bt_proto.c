/*	$NetBSD: bt_proto.c,v 1.4 2006/10/01 10:13:54 plunky Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bt_proto.c,v 1.4 2006/10/01 10:13:54 plunky Exp $");

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

const struct protosw btsw[] = {
    {	/* raw HCI commands */
	SOCK_RAW,	&btdomain,
	BTPROTO_HCI,	PR_ADDR | PR_ATOMIC,
	NULL,		NULL,		NULL,		hci_ctloutput,
	hci_usrreq,	NULL,		NULL,		NULL,
	NULL,
    },
    {	/* HCI SCO data (audio) */
	SOCK_SEQPACKET,	&btdomain,
	BTPROTO_SCO,	PR_CONNREQUIRED | PR_ATOMIC | PR_LISTEN,
	NULL,		NULL,		NULL,		sco_ctloutput,
	sco_usrreq,	NULL,		NULL,		NULL,
	NULL,
    },
    {	/* L2CAP Connection Oriented */
	SOCK_SEQPACKET,	&btdomain,
	BTPROTO_L2CAP,	PR_CONNREQUIRED | PR_ATOMIC | PR_LISTEN,
	NULL,		NULL,		NULL,		l2cap_ctloutput,
	l2cap_usrreq,	NULL,		NULL,		NULL,
	NULL,
    },
#if 0
    {	/* L2CAP Ping Requests */
	SOCK_RAW,	&btdomain,
	BTPROTO_L2CAP,	PR_ADDR | PR_ATOMIC,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,
    },
    {	/* L2CAP Connectionless */
	SOCK_DGRAM,	&btdomain,
	BTPROTO_L2CAP,	PR_ADDR | PR_ATOMIC,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,
    },
#endif
    {	/* RFCOMM */
	SOCK_STREAM,	&btdomain,
	BTPROTO_RFCOMM,	PR_CONNREQUIRED | PR_LISTEN | PR_WANTRCVD,
	NULL,		NULL,		NULL,		rfcomm_ctloutput,
	rfcomm_usrreq,	NULL,		NULL,		NULL,
	NULL,
    }
};

struct domain btdomain = {
	AF_BLUETOOTH,			/* family */
	"bluetooth",			/* name */
	NULL,				/* init routine */
	NULL,				/* externalise access rights */
	NULL,				/* dispose of internalised rights */
	btsw,				/* protosw */
	&btsw[sizeof(btsw)/sizeof(btsw[0])],	/* NPROTOSW */
	NULL,				/* attach to routing table */
	32,				/* rtoffset */
	sizeof(struct sockaddr_bt),	/* maxrtkey */
	NULL,				/* attach af-data */
	NULL,				/* detach af-data */
	{ NULL, NULL },			/* queues */
	{ NULL },			/* link */
	MOWNER_INIT			/* owner */
};
