/*	$KAME: sctp_peeloff.c,v 1.13 2005/03/06 16:04:18 itojun Exp $	*/
/*	$NetBSD: sctp_peeloff.c,v 1.2.16.2 2017/12/03 11:39:04 jdolecek Exp $ */

/*
 * Copyright (C) 2002, 2003 Cisco Systems Inc,
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sctp_peeloff.c,v 1.2.16.2 2017/12/03 11:39:04 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_sctp.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_peeloff.h>
#include <netinet/sctputil.h>

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif /*IPSEC*/

#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif /* SCTP_DEBUG */


int
sctp_can_peel_off(struct socket *head, u_int32_t assoc_id)
{
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;
	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL) {
		return (EFAULT);
	}
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL) {
		return (ENOTCONN);
	}
	/* We are clear to peel this one off */
	return (0);
}

int
sctp_do_peeloff(struct socket *head, struct socket *so, u_int32_t assoc_id)
{
	struct sctp_inpcb *inp, *n_inp;
	struct sctp_tcb *stcb;

	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL)
		return (EFAULT);
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL)
		return (ENOTCONN);

	n_inp = (struct sctp_inpcb *)so->so_pcb;
	n_inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
	    SCTP_PCB_FLAGS_CONNECTED |
	    SCTP_PCB_FLAGS_IN_TCPPOOL | /* Turn on Blocking IO */
	    (SCTP_PCB_COPY_FLAGS & inp->sctp_flags));
	n_inp->sctp_socket = so;

	/*
	 * Now we must move it from one hash table to another and get
	 * the stcb in the right place.
	 */
	sctp_move_pcb_and_assoc(inp, n_inp, stcb);
	/*
	 * And now the final hack. We move data in the
	 * pending side i.e. head to the new socket
	 * buffer. Let the GRUBBING begin :-0
	 */
	sctp_grub_through_socket_buffer(inp, head, so, stcb);
	return (0);
}

struct socket *
sctp_get_peeloff(struct socket *head, u_int32_t assoc_id, int *error)
{
	struct socket *newso;
	struct sctp_inpcb *inp, *n_inp;
	struct sctp_tcb *stcb;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PEEL1) {
		printf("SCTP peel-off called\n");
	}
#endif /* SCTP_DEBUG */

	inp = (struct sctp_inpcb *)head->so_pcb;
	if (inp == NULL) {
		*error = EFAULT;
		return (NULL);
	}
	stcb = sctp_findassociation_ep_asocid(inp, assoc_id);
	if (stcb == NULL) {
		*error = ENOTCONN;
		return (NULL);
	}
	newso = sonewconn(head, SS_ISCONNECTED);
	if (newso == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_PEEL1) {
			printf("sctp_peeloff:sonewconn failed err\n");
		}
#endif /* SCTP_DEBUG */
		*error = ENOMEM;
		SCTP_TCB_UNLOCK(stcb);
		return (NULL);
	}
	n_inp = (struct sctp_inpcb *)newso->so_pcb;
	SCTP_INP_WLOCK(n_inp);
	n_inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
	    SCTP_PCB_FLAGS_CONNECTED |
	    SCTP_PCB_FLAGS_IN_TCPPOOL | /* Turn on Blocking IO */
	    (SCTP_PCB_COPY_FLAGS & inp->sctp_flags));
	n_inp->sctp_socket = newso;
	/* Turn off any non-blocking symantic. */
	newso->so_state &= ~SS_NBIO;
	newso->so_state |= SS_ISCONNECTED;
	/* We remove it right away */
	newso = TAILQ_FIRST(&head->so_q);
	if (soqremque(newso, 1) == 0)
		panic("sctp_peeloff");

	/*
	 * Now we must move it from one hash table to another and get
	 * the stcb in the right place.
	 */
	SCTP_INP_WUNLOCK(n_inp);
	sctp_move_pcb_and_assoc(inp, n_inp, stcb);
	/*
	 * And now the final hack. We move data in the
	 * pending side i.e. head to the new socket
	 * buffer. Let the GRUBBING begin :-0
	 */
	sctp_grub_through_socket_buffer(inp, head, newso, stcb);
	SCTP_TCB_UNLOCK(stcb);
	return (newso);
}
