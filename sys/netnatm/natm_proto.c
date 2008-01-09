/*	$NetBSD: natm_proto.c,v 1.11.8.1 2008/01/09 01:57:50 matt Exp $	*/

/*
 *
 * Copyright (c) 1996 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * protocol layer for access to native mode ATM
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: natm_proto.c,v 1.11.8.1 2008/01/09 01:57:50 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/radix.h>
#include <net/route.h>

#include <netinet/in.h>

#include <netnatm/natm.h>

DOMAIN_DEFINE(natmdomain);

static void natm_init(void);

struct npcblist natm_pcbs = LIST_HEAD_INITIALIZER(natm_pcbs);
struct	ifqueue natmintrq;       	/* natm packet input queue */
int	natmqmaxlen = IFQ_MAXLEN;	/* max # of packets on queue */

const struct protosw natmsw[] = {
{ .pr_type = SOCK_STREAM,
  .pr_domain = &natmdomain,
  .pr_protocol = PROTO_NATMAAL5,
  .pr_flags = PR_CONNREQUIRED,
  .pr_input = 0,
  .pr_output = 0,
  .pr_ctlinput = 0,
  .pr_ctloutput = 0,
  .pr_usrreq = natm_usrreq,
  .pr_init = 0,
  .pr_fasttimo = 0,
  .pr_slowtimo = 0,
  .pr_drain = 0
},
{ .pr_type = SOCK_DGRAM,
  .pr_domain = &natmdomain,
  .pr_protocol = PROTO_NATMAAL5,
  .pr_flags = PR_CONNREQUIRED | PR_ATOMIC,
  .pr_input = 0,
  .pr_output = 0,
  .pr_ctlinput = 0,
  .pr_ctloutput = 0,
  .pr_usrreq = natm_usrreq,
  .pr_init = 0,
  .pr_fasttimo = 0,
  .pr_slowtimo = 0,
  .pr_drain = 0
},
{ .pr_type = SOCK_STREAM,
  .pr_domain = &natmdomain,
  .pr_protocol = PROTO_NATMAAL0,
  .pr_flags = PR_CONNREQUIRED,
  .pr_input = 0,
  .pr_output = 0,
  .pr_ctlinput = 0,
  .pr_ctloutput = 0,
  .pr_usrreq = natm_usrreq,
  .pr_init = 0,
  .pr_fasttimo = 0,
  .pr_slowtimo = 0,
  .pr_drain = 0
},
};

struct domain natmdomain = {
	.dom_family = PF_NATM,
	.dom_name = "natm",
	.dom_init = natm_init,
	.dom_protosw = natmsw,
	.dom_protoswNPROTOSW = &natmsw[sizeof(natmsw)/sizeof(natmsw[0])],
	.dom_ifqueues = { &natmintrq, NULL },
	.dom_rtcache = LIST_HEAD_INITIALIZER(natmdomain.dom_rtcache)
};
#ifdef NATM_STAT
u_int natm_sodropcnt = 0;		/* # mbufs dropped due to full sb */
u_int natm_sodropbytes = 0;		/* # of bytes dropped */
u_int natm_sookcnt = 0;			/* # mbufs ok */
u_int natm_sookbytes = 0;		/* # of bytes ok */
#endif

void natm_init(void)
{
	natmintrq.ifq_maxlen = natmqmaxlen;
}
