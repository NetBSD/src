/*	$NetBSD: iso_proto.c,v 1.24.8.2 2008/01/09 01:57:47 matt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)iso_proto.c	8.2 (Berkeley) 2/9/95
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/*
 * iso_proto.c : protocol switch tables in the ISO domain
 *
 * ISO protocol family includes TP, CLTP, CLNP, 8208
 * TP and CLNP are implemented here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iso_proto.c,v 1.24.8.2 2008/01/09 01:57:47 matt Exp $");


#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/radix.h>

#include <netiso/iso.h>

#include <netiso/clnp.h>
#include <netiso/tp_param.h>
#include <netiso/tp_var.h>
#include <netiso/esis.h>
#include <netiso/idrp_var.h>
#include <netiso/iso_pcb.h>
#include <netiso/cltp_var.h>

const int isoctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

DOMAIN_DEFINE(isodomain);	/* forward declare and add to link set */

const struct protosw  isosw[] = {
	/*
	 *  We need a datagram entry through which net mgmt programs can get
	 *	to the iso_control procedure (iso ioctls). Thus, a minimal
	 *	SOCK_DGRAM interface is provided here.
	 *  THIS ONE MUST BE FIRST: Kludge city : socket() says if(!proto) call
	 *  pffindtype, which gets the first entry that matches the type.
	 *  sigh.
	 */
	{ .pr_type = SOCK_DGRAM,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_CLTP,
	  .pr_flags = PR_ATOMIC | PR_ADDR,
	  .pr_input = 0,
	  .pr_output = cltp_output,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = 0,
	  .pr_usrreq = cltp_usrreq,
	  .pr_init = cltp_init,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0
	},

	/*
	 *	A datagram interface for clnp cannot co-exist with TP/CLNP
	 *  because CLNP has no way to discriminate incoming TP packets from
	 *  packets coming in for any other higher layer protocol.
	 *  Old way: set it up so that pffindproto(... dgm, clnp) fails.
	 *  New way: let pffindproto work (for x.25, thank you) but create
	 *  	a clnp_usrreq() that returns error on PRU_ATTACH.
	 */
	{ .pr_type = SOCK_DGRAM,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_CLNP,
	  .pr_flags = 0,
	  .pr_input = 0,
	  .pr_output = clnp_output,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = 0,
	  .pr_usrreq = clnp_usrreq,
	  .pr_init = clnp_init,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = clnp_slowtimo,
	  .pr_drain =  clnp_drain,
	},

	/* raw clnp */
	{ .pr_type = SOCK_RAW,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_RAW,
	  .pr_flags = PR_ATOMIC | PR_ADDR,
	  .pr_input = rclnp_input,
	  .pr_output = rclnp_output,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = rclnp_ctloutput,
	  .pr_usrreq = clnp_usrreq,
	  .pr_init = 0,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0
	},

	/* ES-IS protocol */
	{ .pr_type = SOCK_DGRAM,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_ESIS,
	  .pr_flags = PR_ATOMIC | PR_ADDR,
	  .pr_input = esis_input,
	  .pr_output = 0,
	  .pr_ctlinput = esis_ctlinput,
	  .pr_ctloutput = 0,
	  .pr_usrreq = esis_usrreq,
	  .pr_init = esis_init,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0
	},

	/* ISOPROTO_INTRAISIS */
	{ .pr_type = SOCK_DGRAM,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_INTRAISIS,
	  .pr_flags = PR_ATOMIC | PR_ADDR,
	  .pr_input = isis_input,
	  .pr_output = 0,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = 0,
	  .pr_usrreq = esis_usrreq,
	  .pr_init = 0,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0
	},

	/* ISOPROTO_IDRP */
	{ .pr_type = SOCK_DGRAM,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_IDRP,
	  .pr_flags = PR_ATOMIC | PR_ADDR,
	  .pr_input = idrp_input,
	  .pr_output = 0,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = 0,
	  .pr_usrreq = idrp_usrreq,
	  .pr_init = idrp_init,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0
	},

	/* ISOPROTO_TP */
	{ .pr_type = SOCK_SEQPACKET,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_TP,
	  .pr_flags = PR_CONNREQUIRED | PR_WANTRCVD | PR_LISTEN | PR_ABRTACPTDIS,
	  .pr_input = tpclnp_input,
	  .pr_output = 0,
	  .pr_ctlinput = tpclnp_ctlinput,
	  .pr_ctloutput = tp_ctloutput,
	  .pr_usrreq = tp_usrreq,
	  .pr_init = tp_init,
	  .pr_fasttimo = tp_fasttimo,
	  .pr_slowtimo = tp_slowtimo,
	  .pr_drain = tp_drain,
	},

#ifdef TPCONS
	/* ISOPROTO_TP */
	{ .pr_type = SOCK_SEQPACKET,
	  .pr_domain = &isodomain,
	  .pr_protocol = ISOPROTO_TP0,
	  .pr_flags = PR_CONNREQUIRED | PR_WANTRCVD | PR_LISTEN | PR_ABRTACPTDIS,
	  .pr_input = tpcons_input,
	  .pr_output = 0,
	  .pr_ctlinput = 0,
	  .pr_ctloutput = tp_ctloutput,
	  .pr_usrreq = tp_usrreq,
	  .pr_init = cons_init,
	  .pr_fasttimo = 0,
	  .pr_slowtimo = 0,
	  .pr_drain = 0,
	},
#endif
};

extern struct ifqueue clnlintrq;

struct domain   isodomain = {
	.dom_family = PF_ISO,
	.dom_name = "iso-domain",
	.dom_init = NULL,
	.dom_externalize = NULL,
	.dom_dispose = NULL,
	.dom_protosw = isosw,
	.dom_protoswNPROTOSW = &isosw[sizeof(isosw) / sizeof(isosw[0])],
	.dom_rtattach = rn_inithead,		/* rtattach */
	.dom_rtoffset = 48,			/* rtoffset */
	.dom_maxrtkey = sizeof(struct sockaddr_iso),	/* maxkeylen */
	.dom_ifattach = NULL,
	.dom_ifdetach = NULL,
	.dom_ifqueues = { &clnlintrq, NULL },		/* ifqueues */
	.dom_link = { NULL },
	.dom_mowner = MOWNER_INIT("",""),
	.dom_rtcache = LIST_HEAD_INITIALIZER(isodomain.dom_rtcache),
	.dom_sockaddr_cmp = sockaddr_iso_cmp
};

int
sockaddr_iso_cmp(const struct sockaddr *sa1, const struct sockaddr *sa2)
{
	int rc;
	uint_fast8_t len, nlen;
	const uint_fast8_t addrofs = offsetof(struct sockaddr_iso, siso_data);
	const struct sockaddr_iso *siso1, *siso2;

	siso1 = satocsiso(sa1);
	siso2 = satocsiso(sa2);

	len = MIN(siso1->siso_len, siso2->siso_len);
	/* No siso_nlen present? */ 
	if (len < addrofs)
		return siso1->siso_len - siso2->siso_len;

	nlen = MIN(siso1->siso_nlen, siso2->siso_nlen);
	if (nlen > addrofs &&
	    (rc = memcmp(siso1->siso_data, siso2->siso_data, nlen)) != 0)
		return rc;

	return siso1->siso_nlen - siso2->siso_nlen;
}
