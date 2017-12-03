/*	$NetBSD: sctp_asconf.c,v 1.11.4.2 2017/12/03 11:39:04 jdolecek Exp $ */
/*	$KAME: sctp_asconf.c,v 1.25 2005/06/16 20:44:24 jinmei Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Cisco Systems, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sctp_asconf.c,v 1.11.4.2 2017/12/03 11:39:04 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_ipsec.h"
#include "opt_inet.h"
#include "opt_sctp.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#include <netinet6/nd6.h>
#endif /* INET6 */

#include <netinet/in_pcb.h>

#include <netinet/sctp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_asconf.h>

/*
 * debug flags:
 *   SCTP_DEBUG_ASCONF1: protocol info, general info and errors
 *   SCTP_DEBUG_ASCONF2: detailed info
 */
#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif /* SCTP_DEBUG */

/*
 * draft-ietf-tsvwg-addip-sctp
 *
 * Address management only currently supported
 * For the bound all case:
 *	the asoc local addr list is always a "DO NOT USE" list
 * For the subset bound case:
 *	If ASCONFs are allowed:
 *		the endpoint local addr list is the usable address list
 *		the asoc local addr list is the "DO NOT USE" list
 *	If ASCONFs are not allowed:
 *		the endpoint local addr list is the default usable list
 *		the asoc local addr list is the usable address list
 *
 * An ASCONF parameter queue exists per asoc which holds the pending
 * address operations.  Lists are updated upon receipt of ASCONF-ACK.
 *
 * Deleted addresses are always immediately removed from the lists as
 * they will (shortly) no longer exist in the kernel.  We send ASCONFs
 * as a courtesy, only if allowed.
 */

/*
 * ASCONF parameter processing
 * response_required: set if a reply is required (eg. SUCCESS_REPORT)
 * returns a mbuf to an "error" response parameter or NULL/"success" if ok
 * FIX: allocating this many mbufs on the fly is pretty inefficient...
 */

static struct mbuf *
sctp_asconf_success_response(uint32_t id)
{
	struct mbuf *m_reply = NULL;
	struct sctp_asconf_paramhdr *aph;

	MGET(m_reply, M_DONTWAIT, MT_DATA);
	if (m_reply == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_success_response: couldn't get mbuf!\n");
		}
#endif /* SCTP_DEBUG */
		return NULL;
	}
	aph = mtod(m_reply, struct sctp_asconf_paramhdr *);
	aph->correlation_id = id;
	aph->ph.param_type = htons(SCTP_SUCCESS_REPORT);
	aph->ph.param_length = sizeof(struct sctp_asconf_paramhdr);
	m_reply->m_len = aph->ph.param_length;
	aph->ph.param_length = htons(aph->ph.param_length);

	return m_reply;
}

static struct mbuf *
sctp_asconf_error_response(uint32_t id, uint16_t cause, uint8_t *error_tlv,
    uint16_t tlv_length)
{
	struct mbuf *m_reply = NULL;
	struct sctp_asconf_paramhdr *aph;
	struct sctp_error_cause *error;
	uint8_t *tlv;

	MGET(m_reply, M_DONTWAIT, MT_DATA);
	if (m_reply == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_error_response: couldn't get mbuf!\n");
		}
#endif /* SCTP_DEBUG */
		return NULL;
	}
	aph = mtod(m_reply, struct sctp_asconf_paramhdr *);
	error = (struct sctp_error_cause *)(aph + 1);

	aph->correlation_id = id;
	aph->ph.param_type = htons(SCTP_ERROR_CAUSE_IND);
	error->code = htons(cause);
	error->length = tlv_length + sizeof(struct sctp_error_cause);
	aph->ph.param_length = error->length +
	    sizeof(struct sctp_asconf_paramhdr);

	if (aph->ph.param_length > MLEN) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_error_response: tlv_length (%xh) too big\n",
			    tlv_length);
		}
#endif /* SCTP_DEBUG */
		sctp_m_freem(m_reply);	/* discard */
		return NULL;
	}

	if (error_tlv != NULL) {
		tlv = (uint8_t *)(error + 1);
		memcpy(tlv, error_tlv, tlv_length);
	}

	m_reply->m_len = aph->ph.param_length;
	error->length = htons(error->length);
	aph->ph.param_length = htons(aph->ph.param_length);

	return m_reply;
}

static struct mbuf *
sctp_process_asconf_add_ip(struct sctp_asconf_paramhdr *aph,
    struct sctp_tcb *stcb, int response_required)
{
	struct mbuf *m_reply = NULL;
	struct sockaddr_storage sa_store;
	struct sctp_ipv4addr_param *v4addr;
	uint16_t param_type, param_length, aparam_length;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
	struct sctp_ipv6addr_param *v6addr;
#endif /* INET6 */

	aparam_length = ntohs(aph->ph.param_length);
	v4addr = (struct sctp_ipv4addr_param *)(aph + 1);
#ifdef INET6
	v6addr = (struct sctp_ipv6addr_param *)(aph + 1);
#endif /* INET6 */
	param_type = ntohs(v4addr->ph.param_type);
	param_length = ntohs(v4addr->ph.param_length);

	sa = (struct sockaddr *)&sa_store;
	switch (param_type) {
	case SCTP_IPV4_ADDRESS:
		if (param_length != sizeof(struct sctp_ipv4addr_param)) {
			/* invalid param size */
			return NULL;
		}
		sin = (struct sockaddr_in *)&sa_store;
		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_port = stcb->rport;
		sin->sin_addr.s_addr = v4addr->addr;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_add_ip: adding ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		break;
	case SCTP_IPV6_ADDRESS:
#ifdef INET6
		if (param_length != sizeof(struct sctp_ipv6addr_param)) {
			/* invalid param size */
			return NULL;
		}
		sin6 = (struct sockaddr_in6 *)&sa_store;
		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		sin6->sin6_port = stcb->rport;
		memcpy((void *)&sin6->sin6_addr, v6addr->addr,
		    sizeof(struct in6_addr));
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_add_ip: adding ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
#else
		/* IPv6 not enabled! */
		/* FIX ME: currently sends back an invalid param error */
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_INVALID_PARAM, (uint8_t *)aph, aparam_length);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_add_ip: v6 disabled- skipping ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		return m_reply;
#endif /* INET6 */
		break;
	default:
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_UNRESOLVABLE_ADDR, (uint8_t *)aph,
		    aparam_length);
		return m_reply;
	} /* end switch */

	/* add the address */
	if (sctp_add_remote_addr(stcb, sa, 0, 6) != 0) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_add_ip: error adding address\n");
		}
#endif /* SCTP_DEBUG */
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_RESOURCE_SHORTAGE, (uint8_t *)aph,
		    aparam_length);
	} else {
		/* notify upper layer */
		sctp_ulp_notify(SCTP_NOTIFY_ASCONF_ADD_IP, stcb, 0, sa);
		if (response_required) {
			m_reply =
			    sctp_asconf_success_response(aph->correlation_id);
		}
	}

	return m_reply;
}

static struct mbuf *
sctp_process_asconf_delete_ip(struct mbuf *m, struct sctp_asconf_paramhdr *aph,
    struct sctp_tcb *stcb, int response_required)
{
	struct mbuf *m_reply = NULL;
	struct sockaddr_storage sa_store, sa_source;
	struct sctp_ipv4addr_param *v4addr;
	uint16_t param_type, param_length, aparam_length;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct ip *iph;
	int result;
#ifdef INET6
	struct sockaddr_in6 *sin6;
	struct sctp_ipv6addr_param *v6addr;
#endif /* INET6 */

	aparam_length = ntohs(aph->ph.param_length);
	v4addr = (struct sctp_ipv4addr_param *)(aph + 1);
#ifdef INET6
	v6addr = (struct sctp_ipv6addr_param *)(aph + 1);
#endif /* INET6 */
	param_type = ntohs(v4addr->ph.param_type);
	param_length = ntohs(v4addr->ph.param_length);

	/* get the source IP address for deletion check */
	iph = mtod(m, struct ip *);
	if (iph->ip_v == IPVERSION) {
		/* IPv4 source */
		sin = (struct sockaddr_in *)&sa_source;
		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_port = stcb->rport;
		sin->sin_addr.s_addr = iph->ip_src.s_addr;
	}
#ifdef INET6
	else if (iph->ip_v == (IPV6_VERSION >> 4)) {
		/* IPv6 source */
		struct ip6_hdr *ip6;

		sin6 = (struct sockaddr_in6 *)&sa_source;
		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		sin6->sin6_port = stcb->rport;
		ip6 = mtod(m, struct ip6_hdr *);
		sin6->sin6_addr = ip6->ip6_src;
	}
#endif /* INET6 */
	else
		return NULL;

	sa = (struct sockaddr *)&sa_store;
	switch (param_type) {
	case SCTP_IPV4_ADDRESS:
		if (param_length != sizeof(struct sctp_ipv4addr_param)) {
			/* invalid param size */
			return NULL;
		}
		sin = (struct sockaddr_in *)&sa_store;
		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_port = stcb->rport;
		sin->sin_addr.s_addr = v4addr->addr;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_delete_ip: deleting ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		break;
	case SCTP_IPV6_ADDRESS:
		if (param_length != sizeof(struct sctp_ipv6addr_param)) {
			/* invalid param size */
			return NULL;
		}
#ifdef INET6
		sin6 = (struct sockaddr_in6 *)&sa_store;
		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		sin6->sin6_port = stcb->rport;
		memcpy(&sin6->sin6_addr, v6addr->addr,
		    sizeof(struct in6_addr));
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_delete_ip: deleting ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
#else
		/* IPv6 not enabled!  No "action" needed; just ack it */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_delete_ip: v6 disabled- ignoring: ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		/* just respond with a "success" ASCONF-ACK */
		return NULL;
#endif /* INET6 */
		break;
	default:
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_UNRESOLVABLE_ADDR, (uint8_t *)aph,
		    aparam_length);
		return m_reply;
	} /* end switch */

	/* make sure the source address is not being deleted */
	if ((memcmp(sa, &sa_source, sizeof(struct sockaddr_in)) == 0)
#ifdef INET6
	    || (memcmp(sa, &sa_source, sizeof(struct sockaddr_in6)) == 0)
#endif /* INET6 */
		) {
		/* trying to delete the source address! */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_delete_ip: tried to delete source addr\n");
		}
#endif /* SCTP_DEBUG */
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_DELETE_SOURCE_ADDR, (uint8_t *)aph,
		    aparam_length);
		return m_reply;
	}

	/* delete the address */
	result = sctp_del_remote_addr(stcb, sa);
	/*
	 * note if result == -2, the address doesn't exist in the asoc
	 * but since it's being deleted anyways, we just ack the delete
	 * -- but this probably means something has already gone awry
	 */
	if (result == -1) {
		/* only one address in the asoc */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_delete_ip: tried to delete last IP addr!\n");
		}
#endif /* SCTP_DEBUG */
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_DELETE_LAST_ADDR, (uint8_t *)aph,
		    aparam_length);
	} else {
		/* notify upper layer */
		sctp_ulp_notify(SCTP_NOTIFY_ASCONF_DELETE_IP, stcb, 0, sa);
	}

	if (response_required) {
		m_reply = sctp_asconf_success_response(aph->correlation_id);
	}
	return m_reply;
}

static struct mbuf *
sctp_process_asconf_set_primary(struct sctp_asconf_paramhdr *aph,
    struct sctp_tcb *stcb, int response_required)
{
	struct mbuf *m_reply = NULL;
	struct sockaddr_storage sa_store;
	struct sctp_ipv4addr_param *v4addr;
	uint16_t param_type, param_length, aparam_length;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
	struct sctp_ipv6addr_param *v6addr;
#endif /* INET6 */

	aparam_length = ntohs(aph->ph.param_length);
	v4addr = (struct sctp_ipv4addr_param *)(aph + 1);
#ifdef INET6
	v6addr = (struct sctp_ipv6addr_param *)(aph + 1);
#endif /* INET6 */
	param_type = ntohs(v4addr->ph.param_type);
	param_length = ntohs(v4addr->ph.param_length);

	sa = (struct sockaddr *)&sa_store;
	switch (param_type) {
	case SCTP_IPV4_ADDRESS:
		if (param_length != sizeof(struct sctp_ipv4addr_param)) {
			/* invalid param size */
			return NULL;
		}
		sin = (struct sockaddr_in *)&sa_store;
		memset(sin, 0, sizeof(*sin));
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(struct sockaddr_in);
		sin->sin_addr.s_addr = v4addr->addr;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_set_primary: ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		break;
	case SCTP_IPV6_ADDRESS:
		if (param_length != sizeof(struct sctp_ipv6addr_param)) {
			/* invalid param size */
			return NULL;
		}
#ifdef INET6
		sin6 = (struct sockaddr_in6 *)&sa_store;
		memset(sin6, 0, sizeof(*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(struct sockaddr_in6);
		memcpy((void *)&sin6->sin6_addr, v6addr->addr,
		    sizeof(struct in6_addr));
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_set_primary: ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
#else
		/* IPv6 not enabled!  No "action" needed; just ack it */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_set_primary: v6 disabled- ignoring: ");
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		/* just respond with a "success" ASCONF-ACK */
		return NULL;
#endif /* INET6 */
		break;
	default:
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_UNRESOLVABLE_ADDR, (uint8_t *)aph,
		    aparam_length);
		return m_reply;
	} /* end switch */

	/* set the primary address */
	if (sctp_set_primary_addr(stcb, sa, NULL) == 0) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_set_primary: primary address set\n");
		}
#endif /* SCTP_DEBUG */
		/* notify upper layer */
		sctp_ulp_notify(SCTP_NOTIFY_ASCONF_SET_PRIMARY, stcb, 0, sa);

		if (response_required) {
			m_reply = sctp_asconf_success_response(aph->correlation_id);
		}
	} else {
		/* couldn't set the requested primary address! */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_asconf_set_primary: set primary failed!\n");
		}
#endif /* SCTP_DEBUG */
		/* must have been an invalid address, so report */
		m_reply = sctp_asconf_error_response(aph->correlation_id,
		    SCTP_ERROR_UNRESOLVABLE_ADDR, (uint8_t *)aph,
		    aparam_length);
	}

	return m_reply;
}

/*
 * handles an ASCONF chunk
 * if all parameters are processed ok, send a plain (empty) ASCONF-ACK
 */
void
sctp_handle_asconf(struct mbuf *m, unsigned int offset, struct sctp_asconf_chunk *cp,
    struct sctp_tcb *stcb, struct sctp_nets *net)
{
	struct sctp_association *asoc;
	uint32_t serial_num;
	struct mbuf *m_ack, *m_result, *m_tail;
	struct sctp_asconf_ack_chunk *ack_cp;
	struct sctp_asconf_paramhdr *aph, *ack_aph;
	struct sctp_ipv6addr_param *p_addr;
	unsigned int asconf_limit;
	int error = 0;		/* did an error occur? */
	/* asconf param buffer */
	static u_int8_t aparam_buf[DEFAULT_PARAM_BUFFER];

	/* verify minimum length */
	if (ntohs(cp->ch.chunk_length) < sizeof(struct sctp_asconf_chunk)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf: chunk too small = %xh\n",
			    ntohs(cp->ch.chunk_length));
		}
#endif /* SCTP_DEBUG */
		return;
	}

	asoc = &stcb->asoc;
	serial_num = ntohl(cp->serial_number);

	if (serial_num == asoc->asconf_seq_in) {
		/* got a duplicate ASCONF */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf: got duplicate serial number = %xh\n",
			       serial_num);
		}
#endif /* SCTP_DEBUG */
		/* resend last ASCONF-ACK... */
		sctp_send_asconf_ack(stcb, 1);
		return;
	} else if (serial_num != (asoc->asconf_seq_in + 1)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf: incorrect serial number = %xh (expected next = %xh)\n",
			    serial_num, asoc->asconf_seq_in+1);
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* it's the expected "next" sequence number, so process it */
	asoc->asconf_seq_in = serial_num;	/* update sequence */
	/* get length of all the param's in the ASCONF */
	asconf_limit = offset + ntohs(cp->ch.chunk_length);
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
		printf("handle_asconf: asconf_limit=%u, sequence=%xh\n",
		    asconf_limit, serial_num);
	}
#endif /* SCTP_DEBUG */
	if (asoc->last_asconf_ack_sent != NULL) {
		/* free last ASCONF-ACK message sent */
		sctp_m_freem(asoc->last_asconf_ack_sent);
		asoc->last_asconf_ack_sent = NULL;
	}
	MGETHDR(m_ack, M_DONTWAIT, MT_DATA);
	if (m_ack == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf: couldn't get mbuf!\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}
	m_tail = m_ack;		/* current reply chain's tail */

	/* fill in ASCONF-ACK header */
	ack_cp = mtod(m_ack, struct sctp_asconf_ack_chunk *);
	ack_cp->ch.chunk_type = SCTP_ASCONF_ACK;
	ack_cp->ch.chunk_flags = 0;
	ack_cp->serial_number = htonl(serial_num);
	/* set initial lengths (eg. just an ASCONF-ACK), ntohx at the end! */
	m_ack->m_len = sizeof(struct sctp_asconf_ack_chunk);
	ack_cp->ch.chunk_length = sizeof(struct sctp_asconf_ack_chunk);
	m_ack->m_pkthdr.len = sizeof(struct sctp_asconf_ack_chunk);

	/* skip the lookup address parameter */
	offset += sizeof(struct sctp_asconf_chunk);
	p_addr = (struct sctp_ipv6addr_param *)sctp_m_getptr(m, offset, sizeof(struct sctp_paramhdr), (uint8_t *)&aparam_buf);
	if (p_addr == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf: couldn't get lookup addr!\n");
		}
#endif /* SCTP_DEBUG */

		/* respond with a missing/invalid mandatory parameter error */
		return;
	}
	/* param_length is already validated in process_control... */
	offset += ntohs(p_addr->ph.param_length);   /* skip lookup addr */

	/* get pointer to first asconf param in ASCONF */
	aph = (struct sctp_asconf_paramhdr *)sctp_m_getptr(m, offset, sizeof(struct sctp_asconf_paramhdr), (uint8_t *)&aparam_buf);
	/* get pointer to first asconf param in ASCONF-ACK */
	if (aph == NULL) {
		printf("Gak in asconf\n");
		return;
	}
	ack_aph = (struct sctp_asconf_paramhdr *)(mtod(m_ack, vaddr_t) + sizeof(struct sctp_asconf_ack_chunk));
	if (ack_aph == NULL) {
		printf("Gak in asconf2\n");
		return;
	}

	/* process through all parameters */
	while (aph != NULL) {
		unsigned int param_length, param_type;

		param_type = ntohs(aph->ph.param_type);
		param_length = ntohs(aph->ph.param_length);
		if (offset + param_length > asconf_limit) {
			/* parameter goes beyond end of chunk! */
			sctp_m_freem(m_ack);
			return;
		}
		m_result = NULL;

		if (param_length > sizeof(aparam_buf)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("handle_asconf: param length (%u) larger than buffer size!\n", param_length);
			}
#endif /* SCTP_DEBUG */
			sctp_m_freem(m_ack);
			return;
		}
		if (param_length <= sizeof(struct sctp_paramhdr)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("handle_asconf: param length (%u) too short\n", param_length);
			}
#endif /* SCTP_DEBUG */
			sctp_m_freem(m_ack);
		}

		/* get the entire parameter */
		aph = (struct sctp_asconf_paramhdr *)sctp_m_getptr(m, offset, param_length, aparam_buf);
		if (aph == NULL) {
			printf("Gag\n");
			sctp_m_freem(m_ack);
			return;
		}
		switch (param_type) {
		case SCTP_ADD_IP_ADDRESS:
			asoc->peer_supports_asconf = 1;
			m_result = sctp_process_asconf_add_ip(aph, stcb, error);
			break;
		case SCTP_DEL_IP_ADDRESS:
			asoc->peer_supports_asconf = 1;
			m_result = sctp_process_asconf_delete_ip(m, aph, stcb,
			    error);
			break;
		case SCTP_ERROR_CAUSE_IND:
			/* not valid in an ASCONF chunk */
			break;
		case SCTP_SET_PRIM_ADDR:
			asoc->peer_supports_asconf_setprim = 1;
			m_result = sctp_process_asconf_set_primary(aph, stcb,
			    error);
			break;
		case SCTP_SUCCESS_REPORT:
			/* not valid in an ASCONF chunk */
			break;
		case SCTP_ULP_ADAPTION:
			/* FIX */
			break;
		default:
			if ((param_type & 0x8000) == 0) {
				/* Been told to STOP at this param */
				asconf_limit = offset;
				/* FIX FIX - We need to call sctp_arethere_unrecognized_parameters()
				 * to get a operr and send it for any param's with the
				 * 0x4000 bit set OR do it here ourselves... note we still
				 * must STOP if the 0x8000 bit is clear.
				 */
			}
			/* unknown/invalid param type */
			break;
		} /* switch */

		/* add any (error) result to the reply mbuf chain */
		if (m_result != NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("handle_asconf: adding reply...\n");
			}
#endif /* SCTP_DEBUG */
			m_tail->m_next = m_result;
			m_tail = m_result;
			/* update lengths, make sure it's aligned too */
			m_result->m_len = SCTP_SIZE32(m_result->m_len);
			m_ack->m_pkthdr.len += m_result->m_len;
			ack_cp->ch.chunk_length += m_result->m_len;
			/* set flag to force success reports */
			error = 1;
		}

		offset += SCTP_SIZE32(param_length);
		/* update remaining ASCONF message length to process */
		if (offset >= asconf_limit) {
			/* no more data in the mbuf chain */
			break;
		}
		/* get pointer to next asconf param */
		aph = (struct sctp_asconf_paramhdr *)sctp_m_getptr(m, offset,
		    sizeof(struct sctp_asconf_paramhdr),
		    (uint8_t *)&aparam_buf);
		if (aph == NULL) {
			/* can't get an asconf paramhdr */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("handle_asconf: can't get asconf param hdr!\n");
			}
#endif /* SCTP_DEBUG */
			/* FIX ME - add error here... */
		}
	} /* while */

	ack_cp->ch.chunk_length = htons(ack_cp->ch.chunk_length);
	/* save the ASCONF-ACK reply */
	asoc->last_asconf_ack_sent = m_ack;
	/* and send (a new one) it out... */
	sctp_send_asconf_ack(stcb, 0);
}

/*
 * does the address match?
 * returns 0 if not, 1 if so
 */
static uint32_t
sctp_asconf_addr_match(struct sctp_asconf_addr *aa, struct sockaddr *sa)
{
#ifdef INET6
	if (sa->sa_family == AF_INET6) {
		/* IPv6 sa address */
		/* XXX scopeid */
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		if ((aa->ap.addrp.ph.param_type == SCTP_IPV6_ADDRESS) &&
		    (memcmp(&aa->ap.addrp.addr, &sin6->sin6_addr,
			    sizeof(struct in6_addr)) == 0)) {
			return (1);
		}
	} else
#endif /* INET6 */
	if (sa->sa_family == AF_INET) {
		/* IPv4 sa address */
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		if ((aa->ap.addrp.ph.param_type == SCTP_IPV4_ADDRESS) &&
		    (memcmp(&aa->ap.addrp.addr, &sin->sin_addr,
			    sizeof(struct in_addr)) == 0)) {
			return (1);
		}
	}
	return (0);
}

/*
 * Cleanup for non-responded/OP ERR'd ASCONF
 */
void
sctp_asconf_cleanup(struct sctp_tcb *stcb, struct sctp_nets *net)
{
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
		printf("asconf_cleanup: marking peer ASCONF incapable and cleaning up\n");
	}
#endif /* SCTP_DEBUG */
	/* mark peer as ASCONF incapable */
	stcb->asoc.peer_supports_asconf = 0;
	stcb->asoc.peer_supports_asconf_setprim = 0;
	/*
	 * clear out any existing asconfs going out
	 */
	sctp_timer_stop(SCTP_TIMER_TYPE_ASCONF, stcb->sctp_ep, stcb, net);
	stcb->asoc.asconf_seq_out++;
	/* remove the old ASCONF on our outbound queue */
	sctp_toss_old_asconf(stcb);
}

/*
 * process an ADD/DELETE IP ack from peer
 * ifa:  corresponding ifaddr to the address being added/deleted
 * type: SCTP_ADD_IP_ADDRESS or SCTP_DEL_IP_ADDRESS
 * flag: 1=success, 0=failure
 */
static void
sctp_asconf_addr_mgmt_ack(struct sctp_tcb *stcb, struct ifaddr *addr,
    uint16_t type, uint32_t flag)
{

	/*
	 * do the necessary asoc list work-
	 * if we get a failure indication, leave the address on the
	 *   "do not use" asoc list
	 * if we get a success indication, remove the address from
	 *   the list
	 */
	/*
	 * Note: this will only occur for ADD_IP_ADDRESS, since
	 * DEL_IP_ADDRESS is never actually added to the list...
	 */
	if (flag) {
		/* success case, so remove from the list */
		sctp_del_local_addr_assoc(stcb, addr);
	}
	/* else, leave it on the list */
}

/*
 * add an asconf add/delete IP address parameter to the queue
 * type = SCTP_ADD_IP_ADDRESS, SCTP_DEL_IP_ADDRESS, SCTP_SET_PRIM_ADDR
 * returns 0 if completed, non-zero if not completed
 * NOTE: if adding, but delete already scheduled (and not yet
 * 	sent out), simply remove from queue.  Same for deleting
 *	an address already scheduled for add.  If a duplicate
 *	operation is found, ignore the new one.
 */
static uint32_t
sctp_asconf_queue_add(struct sctp_tcb *stcb, struct ifaddr *ifa, uint16_t type)
{
	struct sctp_asconf_addr *aa, *aa_next;
#ifdef SCTP_DEBUG
	char buf[128];	/* for address in string format */
#endif /* SCTP_DEBUG */

	/* see if peer supports ASCONF */
	if (stcb->asoc.peer_supports_asconf == 0) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add: peer doesn't support ASCONF\n");
		}
#endif /* SCTP_DEBUG */
		return (-1);
	}

	/* make sure the request isn't already in the queue */
	for (aa=TAILQ_FIRST(&stcb->asoc.asconf_queue); aa!=NULL; aa=aa_next) {
		aa_next = TAILQ_NEXT(aa, next);
		/* address match? */
		if (sctp_asconf_addr_match(aa, ifa->ifa_addr) == 0)
			continue;
		/* is the request already in queue (sent or not) */
		if (aa->ap.aph.ph.param_type == type) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("asconf_queue_add: request already exists\n");
			}
#endif /* SCTP_DEBUG */
			return (-1);
		}
		/* is the negative request already in queue, and not sent */
		if (aa->sent == 0 &&
		    /* add requested, delete already queued */
		    ((type == SCTP_ADD_IP_ADDRESS &&
		      aa->ap.aph.ph.param_type == SCTP_DEL_IP_ADDRESS) ||
		     /* delete requested, add already queued */
		     (type == SCTP_DEL_IP_ADDRESS &&
		      aa->ap.aph.ph.param_type == SCTP_ADD_IP_ADDRESS))) {
			/* delete the existing entry in the queue */
			TAILQ_REMOVE(&stcb->asoc.asconf_queue, aa, next);
			/* take the entry off the appropriate list */
			sctp_asconf_addr_mgmt_ack(stcb, aa->ifa, type, 1);
			/* free the entry */
			free(aa, M_PCB);

#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("asconf_queue_add: removing 'opposite' queued request\n");
			}
#endif /* SCTP_DEBUG */
			return (-1);
		}
	} /* for each aa */

	/* adding new request to the queue */
	aa = malloc(sizeof(*aa), M_PCB, M_NOWAIT);
	if (aa == NULL) {
		/* didn't get memory */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add: failed to get memory!\n");
		}
#endif /* SCTP_DEBUG */
		return (-1);
	}
	/* fill in asconf address parameter fields */
	/* top level elements are "networked" during send */
	aa->ap.aph.ph.param_type = type;
	aa->ifa = ifa;
	/* correlation_id filled in during send routine later... */
	if (ifa->ifa_addr->sa_family == AF_INET6) {
		/* IPv6 address */
#ifdef SCTP_DEBUG
		char ip6buf[INET6_ADDRSTRLEN];
#endif
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
		aa->ap.addrp.ph.param_type = SCTP_IPV6_ADDRESS;
		aa->ap.addrp.ph.param_length = (sizeof(struct sctp_ipv6addr_param));
		aa->ap.aph.ph.param_length =
		    sizeof(struct sctp_asconf_paramhdr) +
		    sizeof(struct sctp_ipv6addr_param);
		memcpy(&aa->ap.addrp.addr, &sin6->sin6_addr,
		    sizeof(struct in6_addr));
#ifdef SCTP_DEBUG
		strlcpy(buf, IN6_PRINT(ip6buf, &sin6->sin6_addr), sizeof(buf));
#endif /* SCTP_DEBUG */

	} else if (ifa->ifa_addr->sa_family == AF_INET) {
		/* IPv4 address */
		struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
		aa->ap.addrp.ph.param_type = SCTP_IPV4_ADDRESS;
		aa->ap.addrp.ph.param_length = (sizeof(struct sctp_ipv4addr_param));
		aa->ap.aph.ph.param_length =
		    sizeof(struct sctp_asconf_paramhdr) +
		    sizeof(struct sctp_ipv4addr_param);
		memcpy(&aa->ap.addrp.addr, &sin->sin_addr,
		    sizeof(struct in_addr));
#ifdef SCTP_DEBUG
		strlcpy(buf, inet_ntoa(sin->sin_addr), sizeof(buf));
#endif /* SCTP_DEBUG */
	} else {
		/* invalid family! */
		return (-1);
	}
	aa->sent = 0;			/* clear sent flag */

	/*
	 * if we are deleting an address it should go out last
	 * otherwise, add it to front of the pending queue
	 */
	if (type == SCTP_ADD_IP_ADDRESS) {
		/* add goes to the front of the queue */
		TAILQ_INSERT_HEAD(&stcb->asoc.asconf_queue, aa, next);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add: appended asconf ADD_IP_ADDRESS: %s\n", buf);
		}
#endif /* SCTP_DEBUG */
	} else {
		/* delete and set primary goes to the back of the queue */
		TAILQ_INSERT_TAIL(&stcb->asoc.asconf_queue, aa, next);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			if (type == SCTP_DEL_IP_ADDRESS) {
				printf("asconf_queue_add: inserted asconf DEL_IP_ADDRESS: %s\n", buf);
			} else {
				printf("asconf_queue_add: inserted asconf SET_PRIM_ADDR: %s\n", buf);
			}
		}
#endif /* SCTP_DEBUG */
	}

	return (0);
}

/*
 * add an asconf add/delete IP address parameter to the queue by addr
 * type = SCTP_ADD_IP_ADDRESS, SCTP_DEL_IP_ADDRESS, SCTP_SET_PRIM_ADDR
 * returns 0 if completed, non-zero if not completed
 * NOTE: if adding, but delete already scheduled (and not yet
 * 	sent out), simply remove from queue.  Same for deleting
 *	an address already scheduled for add.  If a duplicate
 *	operation is found, ignore the new one.
 */
static uint32_t
sctp_asconf_queue_add_sa(struct sctp_tcb *stcb, struct sockaddr *sa,
    uint16_t type)
{
	struct sctp_asconf_addr *aa, *aa_next;

	/* see if peer supports ASCONF */
	if (stcb->asoc.peer_supports_asconf == 0) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add_sa: peer doesn't support ASCONF\n");
		}
#endif /* SCTP_DEBUG */
		return (-1);
	}

	/* make sure the request isn't already in the queue */
	for (aa = TAILQ_FIRST(&stcb->asoc.asconf_queue); aa != NULL;
	    aa = aa_next) {
		aa_next = TAILQ_NEXT(aa, next);
		/* address match? */
		if (sctp_asconf_addr_match(aa, sa) == 0)
			continue;
		/* is the request already in queue (sent or not) */
		if (aa->ap.aph.ph.param_type == type) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("asconf_queue_add_sa: request already exists\n");
			}
#endif /* SCTP_DEBUG */
			return (-1);
		}

		/* is the negative request already in queue, and not sent */
		if (aa->sent == 1)
			continue;
		if (type == SCTP_ADD_IP_ADDRESS &&
		    aa->ap.aph.ph.param_type == SCTP_DEL_IP_ADDRESS) {
			/* add requested, delete already queued */

			/* delete the existing entry in the queue */
			TAILQ_REMOVE(&stcb->asoc.asconf_queue, aa, next);
			/* free the entry */
			free(aa, M_PCB);
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("asconf_queue_add_sa: removing queued delete request\n");
			}
#endif /* SCTP_DEBUG */
			return (-1);
		} else if (type == SCTP_DEL_IP_ADDRESS &&
			   aa->ap.aph.ph.param_type == SCTP_ADD_IP_ADDRESS) {
			/* delete requested, add already queued */

			/* delete the existing entry in the queue */
			TAILQ_REMOVE(&stcb->asoc.asconf_queue, aa, next);
			/* take the entry off the appropriate list */
			sctp_asconf_addr_mgmt_ack(stcb, aa->ifa, type, 1);
			/* free the entry */
			free(aa, M_PCB);
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("asconf_queue_add_sa: removing queued add request\n");
			}
#endif /* SCTP_DEBUG */
			return (-1);
		}
	} /* for each aa */

	/* adding new request to the queue */
	aa = malloc(sizeof(*aa), M_PCB, M_NOWAIT);
	if (aa == NULL) {
		/* didn't get memory */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add_sa: failed to get memory!\n");
		}
#endif /* SCTP_DEBUG */
		return (-1);
	}
	/* fill in asconf address parameter fields */
	/* top level elements are "networked" during send */
	aa->ap.aph.ph.param_type = type;
	aa->ifa = sctp_find_ifa_by_addr(sa);
	/* correlation_id filled in during send routine later... */
	if (sa->sa_family == AF_INET6) {
		/* IPv6 address */
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)sa;
		aa->ap.addrp.ph.param_type = SCTP_IPV6_ADDRESS;
		aa->ap.addrp.ph.param_length = (sizeof(struct sctp_ipv6addr_param));
		aa->ap.aph.ph.param_length = sizeof(struct sctp_asconf_paramhdr) + sizeof(struct sctp_ipv6addr_param);
		memcpy(&aa->ap.addrp.addr, &sin6->sin6_addr,
		    sizeof(struct in6_addr));
	} else if (sa->sa_family == AF_INET) {
		/* IPv4 address */
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		aa->ap.addrp.ph.param_type = SCTP_IPV4_ADDRESS;
		aa->ap.addrp.ph.param_length = (sizeof(struct sctp_ipv4addr_param));
		aa->ap.aph.ph.param_length = sizeof(struct sctp_asconf_paramhdr) + sizeof(struct sctp_ipv4addr_param);
		memcpy(&aa->ap.addrp.addr, &sin->sin_addr,
		    sizeof(struct in_addr));
	} else {
		/* invalid family! */
		return (-1);
	}
	aa->sent = 0;			/* clear sent flag */

	/*
	 * if we are deleting an address it should go out last
	 * otherwise, add it to front of the pending queue
	 */
	if (type == SCTP_ADD_IP_ADDRESS) {
		/* add goes to the front of the queue */
		TAILQ_INSERT_HEAD(&stcb->asoc.asconf_queue, aa, next);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_queue_add_sa: appended asconf ADD_IP_ADDRESS\n");
		}
#endif /* SCTP_DEBUG */
	} else {
		/* delete and set primary goes to the back of the queue */
		TAILQ_INSERT_TAIL(&stcb->asoc.asconf_queue, aa, next);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			if (type == SCTP_DEL_IP_ADDRESS) {
				printf("asconf_queue_add_sa: inserted asconf DEL_IP_ADDRESS\n");
			} else {
				printf("asconf_queue_add_sa: inserted asconf SET_PRIM_ADDR\n");
			}
		}
#endif /* SCTP_DEBUG */
	}

	return (0);
}

/*
 * find a specific asconf param on our "sent" queue
 */
static struct sctp_asconf_addr *
sctp_asconf_find_param(struct sctp_tcb *stcb, uint32_t correlation_id)
{
	struct sctp_asconf_addr *aa;

	TAILQ_FOREACH(aa, &stcb->asoc.asconf_queue, next) {
		if (aa->ap.aph.correlation_id == correlation_id &&
		    aa->sent == 1) {
			/* found it */
			return (aa);
		}
	}
	/* didn't find it */
	return (NULL);
}

/*
 * process an SCTP_ERROR_CAUSE_IND for a ASCONF-ACK parameter
 * and do notifications based on the error response
 */
static void
sctp_asconf_process_error(struct sctp_tcb *stcb,
    struct sctp_asconf_paramhdr *aph)
{
	struct sctp_error_cause *eh;
	struct sctp_paramhdr *ph;
	uint16_t param_type;
	uint16_t error_code;

	eh = (struct sctp_error_cause *)(aph + 1);
	ph = (struct sctp_paramhdr *)(eh + 1);
	/* validate lengths */
	if (htons(eh->length) + sizeof(struct sctp_error_cause) >
	    htons(aph->ph.param_length)) {
		/* invalid error cause length */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_process_error: cause element too long\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}
	if (htons(ph->param_length) + sizeof(struct sctp_paramhdr) >
	    htons(eh->length)) {
		/* invalid included TLV length */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("asconf_process_error: included TLV too long\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* which error code ? */
	error_code = ntohs(eh->code);
	param_type = ntohs(aph->ph.param_type);
	/* FIX: this should go back up the REMOTE_ERROR ULP notify */
	switch (error_code) {
	case SCTP_ERROR_RESOURCE_SHORTAGE:
		/* we allow ourselves to "try again" for this error */
		break;
	default:
		/* peer can't handle it... */
		switch (param_type) {
		case SCTP_ADD_IP_ADDRESS:
		case SCTP_DEL_IP_ADDRESS:
			stcb->asoc.peer_supports_asconf = 0;
			break;
		case SCTP_SET_PRIM_ADDR:
			stcb->asoc.peer_supports_asconf_setprim = 0;
			break;
		default:
			break;
		}
	}
}

/*
 * process an asconf queue param
 * aparam: parameter to process, will be removed from the queue
 * flag: 1=success, 0=failure
 */
static void
sctp_asconf_process_param_ack(struct sctp_tcb *stcb,
    struct sctp_asconf_addr *aparam, uint32_t flag)
{
	uint16_t param_type;

	/* process this param */
	param_type = aparam->ap.aph.ph.param_type;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
		printf("process_param_ack: handling asconf parameter type=%xh\n", param_type);
	}
#endif /* SCTP_DEBUG */
	switch (param_type) {
	case SCTP_ADD_IP_ADDRESS:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_param_ack: added IP address\n");
		}
#endif /* SCTP_DEBUG */
		sctp_asconf_addr_mgmt_ack(stcb, aparam->ifa, param_type, flag);
		break;
	case SCTP_DEL_IP_ADDRESS:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_param_ack: deleted IP address\n");
		}
#endif /* SCTP_DEBUG */
		/* nothing really to do... lists already updated */
		break;
	case SCTP_SET_PRIM_ADDR:
		/* nothing to do... peer may start using this addr */
		if (flag == 0)
			stcb->asoc.peer_supports_asconf_setprim = 0;
		break;
	default:
		/* should NEVER happen */
		break;
	} /* switch */

	/* remove the param and free it */
	TAILQ_REMOVE(&stcb->asoc.asconf_queue, aparam, next);
	free(aparam, M_PCB);
}

/*
 * cleanup from a bad asconf ack parameter
 */
static void
sctp_asconf_ack_clear(struct sctp_tcb *stcb)
{
	/* assume peer doesn't really know how to do asconfs */
	stcb->asoc.peer_supports_asconf = 0;
	stcb->asoc.peer_supports_asconf_setprim = 0;
	/* XXX we could free the pending queue here */
}

void
sctp_handle_asconf_ack(struct mbuf *m, int offset,
    struct sctp_asconf_ack_chunk *cp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	struct sctp_association *asoc;
	uint32_t serial_num;
	uint16_t ack_length;
	struct sctp_asconf_paramhdr *aph;
	struct sctp_asconf_addr *aa, *aa_next;
	uint32_t last_error_id = 0;		/* last error correlation id */
	uint32_t id;
	struct sctp_asconf_addr *ap;
	/* asconf param buffer */
	static u_int8_t aparam_buf[DEFAULT_PARAM_BUFFER];

	/* verify minimum length */
	if (ntohs(cp->ch.chunk_length) < sizeof(struct sctp_asconf_ack_chunk)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf_ack: chunk too small = %xh\n",
			       ntohs(cp->ch.chunk_length));
		}
#endif /* SCTP_DEBUG */
		return;
	}

	asoc = &stcb->asoc;
	serial_num = ntohl(cp->serial_number);

	/*
	 * NOTE: we may want to handle this differently- currently, we
	 * will abort when we get an ack for the expected serial number + 1
	 * (eg. we didn't send it), process an ack normally if it is the
	 * expected serial number, and re-send the previous ack for *ALL*
	 * other serial numbers
	 */

	/*
	 * if the serial number is the next expected, but I didn't send it,
	 * abort the asoc, since someone probably just hijacked us...
	 */
	if (serial_num == (asoc->asconf_seq_out + 1)) {
		sctp_abort_an_association(stcb->sctp_ep, stcb,
		    SCTP_ERROR_ILLEGAL_ASCONF_ACK, NULL);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf_ack: got unexpected next serial number! Aborting asoc!\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	if (serial_num != asoc->asconf_seq_out) {
		/* got a duplicate/unexpected ASCONF-ACK */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("handle_asconf_ack: got duplicate/unexpected serial number = %xh (expected = %xh)\n", serial_num, asoc->asconf_seq_out);
		}
#endif /* SCTP_DEBUG */
		return;
	}
	/* stop our timer */
	sctp_timer_stop(SCTP_TIMER_TYPE_ASCONF, stcb->sctp_ep, stcb, net);

	/* process the ASCONF-ACK contents */
	ack_length = ntohs(cp->ch.chunk_length) -
	    sizeof(struct sctp_asconf_ack_chunk);
	offset += sizeof(struct sctp_asconf_ack_chunk);
	/* process through all parameters */
	while (ack_length >= sizeof(struct sctp_asconf_paramhdr)) {
		unsigned int param_length, param_type;

		/* get pointer to next asconf parameter */
		aph = (struct sctp_asconf_paramhdr *)sctp_m_getptr(m, offset,
		    sizeof(struct sctp_asconf_paramhdr), aparam_buf);
		if (aph == NULL) {
			/* can't get an asconf paramhdr */
			sctp_asconf_ack_clear(stcb);
			return;
		}
		param_type = ntohs(aph->ph.param_type);
		param_length = ntohs(aph->ph.param_length);
		if (param_length > ack_length) {
			sctp_asconf_ack_clear(stcb);
			return;
		}
		if (param_length < sizeof(struct sctp_paramhdr)) {
			sctp_asconf_ack_clear(stcb);
			return;
		}

		/* get the complete parameter... */
		if (param_length > sizeof(aparam_buf)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("param length (%u) larger than buffer size!\n", param_length);
			}
#endif /* SCTP_DEBUG */
			sctp_asconf_ack_clear(stcb);
			return;
		}
		aph = (struct sctp_asconf_paramhdr *)sctp_m_getptr(m, offset, param_length, aparam_buf);
		if (aph == NULL) {
			sctp_asconf_ack_clear(stcb);
			return;
		}
		/* correlation_id is transparent to peer, no ntohl needed */
		id = aph->correlation_id;

		switch (param_type) {
		case SCTP_ERROR_CAUSE_IND:
			last_error_id = id;
			/* find the corresponding asconf param in our queue */
			ap = sctp_asconf_find_param(stcb, id);
			if (ap == NULL) {
				/* hmm... can't find this in our queue! */
				break;
			}
			/* process the parameter, failed flag */
			sctp_asconf_process_param_ack(stcb, ap, 0);
			/* process the error response */
			sctp_asconf_process_error(stcb, aph);
			break;
		case SCTP_SUCCESS_REPORT:
			/* find the corresponding asconf param in our queue */
			ap = sctp_asconf_find_param(stcb, id);
			if (ap == NULL) {
				/* hmm... can't find this in our queue! */
				break;
			}
			/* process the parameter, success flag */
			sctp_asconf_process_param_ack(stcb, ap, 1);
			break;
		default:
			break;
		} /* switch */

		/* update remaining ASCONF-ACK message length to process */
		ack_length -= SCTP_SIZE32(param_length);
		if (ack_length <= 0) {
			/* no more data in the mbuf chain */
			break;
		}
		offset += SCTP_SIZE32(param_length);
	} /* while */

	/*
	 * if there are any "sent" params still on the queue, these are
	 * implicitly "success", or "failed" (if we got an error back)
	 * ... so process these appropriately
	 *
	 * we assume that the correlation_id's are monotonically increasing
	 * beginning from 1 and that we don't have *that* many outstanding
	 * at any given time
	 */
	if (last_error_id == 0)
		last_error_id--;	/* set to "max" value */
	for (aa = TAILQ_FIRST(&stcb->asoc.asconf_queue); aa != NULL;
	    aa = aa_next) {
		aa_next = TAILQ_NEXT(aa, next);
		if (aa->sent == 1) {
			/*
			 * implicitly successful or failed
			 * if correlation_id < last_error_id, then success
			 * else, failure
			 */
			if (aa->ap.aph.correlation_id < last_error_id)
				sctp_asconf_process_param_ack(stcb, aa,
				    SCTP_SUCCESS_REPORT);
			else
				sctp_asconf_process_param_ack(stcb, aa,
				    SCTP_ERROR_CAUSE_IND);
		} else {
			/*
			 * since we always process in order (FIFO queue)
			 * if we reach one that hasn't been sent, the
			 * rest should not have been sent either.
			 * so, we're done...
			 */
			break;
		}
	}

	/* update the next sequence number to use */
	asoc->asconf_seq_out++;
	/* remove the old ASCONF on our outbound queue */
	sctp_toss_old_asconf(stcb);
	/* clear the sent flag to allow new ASCONFs */
	asoc->asconf_sent = 0;
	if (!TAILQ_EMPTY(&stcb->asoc.asconf_queue)) {
		/* we have more params, so restart our timer */
		sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, stcb->sctp_ep,
		    stcb, net);
	}
}

/* is this an interface that we care about at all? */
static uint32_t
sctp_is_desired_interface_type(struct ifaddr *ifa)
{
	int result;

	/* check the interface type to see if it's one we care about */
	switch (ifa->ifa_ifp->if_type) {
	case IFT_ETHER:
	case IFT_ISO88023:
	case IFT_ISO88025:
	case IFT_STARLAN:
	case IFT_P10:
	case IFT_P80:
	case IFT_HY:
	case IFT_FDDI:
	case IFT_PPP:
	case IFT_XETHER:
	case IFT_SLIP:
	case IFT_GIF:
		result = 1;
		break;
	default:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("ignoring interface type = %u\n",
			       ifa->ifa_ifp->if_type);
		}
#endif /* SCTP_DEBUG */
		result = 0;
	} /* end switch */

	return (result);
}

static uint32_t
sctp_is_scopeid_in_nets(struct sctp_tcb *stcb, struct sockaddr *sa)
{
	struct sockaddr_in6 *sin6;
	const struct sockaddr_in6 *net6;
	struct sctp_nets *net;

	if (sa->sa_family != AF_INET6) {
		/* wrong family */
		return (0);
	}

	sin6 = (struct sockaddr_in6 *)sa;
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) == 0) {
		/* not link local address */
		return (0);
	}
	/* hunt through our destination nets list for this scope_id */
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		if ((rtcache_getdst(&net->ro))->sa_family !=
		    AF_INET6)
			continue;
		net6 = (const struct sockaddr_in6 *)rtcache_getdst(&net->ro);
		if (IN6_IS_ADDR_LINKLOCAL(&net6->sin6_addr) == 0)
			continue;
		if (sctp_is_same_scope(sin6, net6)) {
			/* found one */
			return (1);
		}
	}
	/* didn't find one */
	return (0);
}

/*
 * address management functions
 */
static void
sctp_addr_mgmt_assoc(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct ifaddr *ifa, uint16_t type)
{
	int status;
#ifdef SCTP_DEBUG
	char buf[128];	/* for address in string format */
#endif /* SCTP_DEBUG */

	if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0 &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) == 0) {
		/* subset bound, no ASCONF allowed case, so ignore */
		return;
	}

	/*
	 * note: we know this is not the subset bound, no ASCONF case
	 *	eg. this is boundall or subset bound w/ASCONF allowed
	 */

	/* first, make sure it's a good address family */
	if (ifa->ifa_addr->sa_family != AF_INET6 &&
	    ifa->ifa_addr->sa_family != AF_INET) {
		return;
	}

	/* make sure we're "allowed" to add this type of addr */
	if (ifa->ifa_addr->sa_family == AF_INET6) {
		struct in6_ifaddr *ifa6;

		/* invalid if we're not a v6 endpoint */
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0)
			return;
		/* is the v6 addr really valid ? */
		ifa6 = (struct in6_ifaddr *)ifa;
		if (IFA6_IS_DEPRECATED(ifa6) ||
		    (ifa6->ia6_flags &
		     (IN6_IFF_DETACHED | IN6_IFF_ANYCAST | IN6_IFF_NOTREADY))) {
			/* can't use an invalid address */
			return;
		}
	}

	/* put this address on the "pending/do not use yet" list */
	/*
	 * Note: we do this primarily for the subset bind case
	 * We don't have scoping flags at the EP level, so we must
	 * add link local/site local addresses to the EP, then need
	 * to "negate" them here.  Recall that this routine is only
	 * called for the subset bound w/ASCONF allowed case.
	 */

	/*
	 * do a scope_id check against any link local addresses
	 * in the destination nets list to see if we should put
	 * this local address on the pending list or not
	 * eg. don't put on the list if we have a link local
	 * destination with the same scope_id
	 */
	if (type == SCTP_ADD_IP_ADDRESS) {
		if (sctp_is_scopeid_in_nets(stcb, ifa->ifa_addr) == 0) {
			sctp_add_local_addr_assoc(stcb, ifa);
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("addr_mgmt_assoc: added to pending list ");
				sctp_print_address(ifa->ifa_addr);
			}
#endif /* SCTP_DEBUG */
		}
	}
	/*
	 * check address scope
	 * if address is out of scope, don't queue anything...
	 * note: this would leave the address on both inp and asoc lists
	 */
	if (ifa->ifa_addr->sa_family == AF_INET6) {
#ifdef SCTP_DEBUG
		char ip6buf[INET6_ADDRSTRLEN];
#endif /* SCTP_DEBUG */
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
#ifdef SCTP_DEBUG
		strlcpy(buf, IN6_PRINT(ip6buf, &sin6->sin6_addr), sizeof(buf));
#endif /* SCTP_DEBUG */
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			/* we skip unspecifed addresses */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("addr_mgmt_assoc: unspecified IPv6 addr\n");
			}
#endif /* SCTP_DEBUG */
			return;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
			if (stcb->asoc.local_scope == 0) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("addr_mgmt_assoc: skipping link local IPv6 addr: %s\n", buf);
				}
#endif /* SCTP_DEBUG */
				return;
			}
			/* is it the right link local scope? */
			if (sctp_is_scopeid_in_nets(stcb, ifa->ifa_addr) == 0) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("addr_mgmt_assoc: skipping link local IPv6 addr: %s, wrong scope_id\n", buf);
				}
#endif /* SCTP_DEBUG */
				return;
			}
		}
		if (stcb->asoc.site_scope == 0 &&
		    IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("addr_mgmt_assoc: skipping site local IPv6 addr: %s\n", buf);
			}
#endif /* SCTP_DEBUG */
			return;
		}
	} else if (ifa->ifa_addr->sa_family == AF_INET) {
		struct sockaddr_in *sin;
		struct in6pcb *inp6;

		inp6 = (struct in6pcb *)&inp->ip_inp.inp;
		/* invalid if we are a v6 only endpoint */
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
		    (inp6->in6p_flags & IN6P_IPV6_V6ONLY)
			)
			return;

		sin = (struct sockaddr_in *)ifa->ifa_addr;
#ifdef SCTP_DEBUG
		strlcpy(buf, inet_ntoa(sin->sin_addr), sizeof(buf));
#endif /* SCTP_DEBUG */
		if (sin->sin_addr.s_addr == 0) {
			/* we skip unspecifed addresses */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("addr_mgmt_assoc: unspecified IPv4 addr\n");
			}
#endif /* SCTP_DEBUG */
			return;
		}
		if (stcb->asoc.ipv4_local_scope == 0 &&
		    IN4_ISPRIVATE_ADDRESS(&sin->sin_addr)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("addr_mgmt_assoc: skipping private IPv4 addr: %s\n", buf);
			}
#endif /* SCTP_DEBUG */
			return;
		}
	} else {
		/* else, not AF_INET or AF_INET6, so skip */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("addr_mgmt_assoc: not AF_INET or AF_INET6\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* queue an asconf for this address add/delete */
	if (inp->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) {
		/* does the peer do asconf? */
		if (stcb->asoc.peer_supports_asconf) {
			/* queue an asconf for this addr */
			status = sctp_asconf_queue_add(stcb, ifa, type);
			/*
			 * if queued ok, and in correct state, set the
			 * ASCONF timer
			 * if in non-open state, we will set this timer
			 * when the state does go open and do all the
			 * asconf's
			 */
			if (status == 0 &&
			    SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_OPEN) {
				sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, inp,
				    stcb, stcb->asoc.primary_destination);
			}
		}
	} else {
		/* this is the boundall, no ASCONF case */
#if 0 /* assume kernel will delete this very shortly; add done above */
		if (type == SCTP_DEL_IP_ADDRESS) {
			/* if deleting, add this addr to the do not use list */
			sctp_add_local_addr_assoc(stcb, ifa);
		}
#endif
	}
}

static void
sctp_addr_mgmt_ep(struct sctp_inpcb *inp, struct ifaddr *ifa, uint16_t type)
{
	struct sctp_tcb *stcb;
	int s;

	SCTP_INP_WLOCK(inp);
	/* make sure we're "allowed" to add this type of addr */
	if (ifa->ifa_addr->sa_family == AF_INET6) {
		struct in6_ifaddr *ifa6;

		/* invalid if we're not a v6 endpoint */
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0) {
			SCTP_INP_WUNLOCK(inp);
			return;
		}
		/* is the v6 addr really valid ? */
		ifa6 = (struct in6_ifaddr *)ifa;
		if (IFA6_IS_DEPRECATED(ifa6) ||
		    (ifa6->ia6_flags &
		     (IN6_IFF_DETACHED | IN6_IFF_ANYCAST | IN6_IFF_NOTREADY))) {
			/* can't use an invalid address */
			SCTP_INP_WUNLOCK(inp);
			return;
		}
	} else if (ifa->ifa_addr->sa_family == AF_INET) {
		/* invalid if we are a v6 only endpoint */
		struct in6pcb *inp6;
		inp6 = (struct in6pcb *)&inp->ip_inp.inp;

		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
		    (inp6->in6p_flags & IN6P_IPV6_V6ONLY)
			) {
			SCTP_INP_WUNLOCK(inp);
			return;
		}
	} else {
		/* invalid address family */
		SCTP_INP_WUNLOCK(inp);
		return;
	}
	/* is this endpoint subset bound ? */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0) {
		/* subset bound endpoint */
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) == 0) {
			/*
			 * subset bound, but ASCONFs not allowed...
			 * if adding, nothing to do, since not allowed
			 * if deleting, remove address from endpoint
			 *	peer will have to "timeout" this addr
			 */
			if (type == SCTP_DEL_IP_ADDRESS) {
				sctp_del_local_addr_ep(inp, ifa);
			}
			/* no asconfs to queue for this inp... */
			SCTP_INP_WUNLOCK(inp);
			return;
		} else {
			/*
			 * subset bound, ASCONFs allowed...
			 * if adding, add address to endpoint list
			 * if deleting, remove address from endpoint
			 */
			if (type == SCTP_ADD_IP_ADDRESS) {
				sctp_add_local_addr_ep(inp, ifa);
			} else {
				sctp_del_local_addr_ep(inp, ifa);
			}
			/* drop through and notify all asocs */
		}
	}

	s = splsoftnet();
	/* process for all associations for this endpoint */
	LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
		SCTP_TCB_LOCK(stcb);
		sctp_addr_mgmt_assoc(inp, stcb, ifa, type);
		SCTP_TCB_UNLOCK(stcb);
	} /* for each stcb */
	splx(s);
	SCTP_INP_WUNLOCK(inp);
}

/*
 * restrict the use of this address
 */
static void
sctp_addr_mgmt_restrict_ep(struct sctp_inpcb *inp, struct ifaddr *ifa)
{
	struct sctp_tcb *stcb;
	int s;

	/* is this endpoint bound to all? */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0) {
		/*
		 * Nothing to do for subset bound case.
		 * Allow sctp_bindx() to manage the address lists
		 */
		return;
	}

	s = splsoftnet();
	SCTP_INP_RLOCK(inp);
	/* process for all associations for this endpoint */
	LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
		/* put this address on the "pending/do not use yet" list */
		SCTP_TCB_LOCK(stcb);
		sctp_add_local_addr_assoc(stcb, ifa);
		SCTP_TCB_UNLOCK(stcb);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("restrict_ep: added addr to unusable list\n");
		}
#endif /* SCTP_DEBUG */
	} /* for each stcb */
	splx(s);
	SCTP_INP_RUNLOCK(inp);
}

/*
 * this is only called for kernel initiated address changes
 * eg. it will check the PCB_FLAGS_AUTO_ASCONF flag
 */
static void
sctp_addr_mgmt(struct ifaddr *ifa, uint16_t type) {
	struct sockaddr *sa;
	struct sctp_inpcb *inp;

	/* make sure we care about this interface... */
	if (!sctp_is_desired_interface_type(ifa)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("sctp_addr_mgmt: ignoring this interface\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	sa = ifa->ifa_addr;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_INET6)
		return;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
		if (type == SCTP_ADD_IP_ADDRESS)
			printf("sctp_addr_mgmt: kernel adds ");
		else
			printf("sctp_addr_mgmt: kernel deletes ");
		sctp_print_address(sa);
	}
#endif /* SCTP_DEBUG */

	/* go through all our PCB's */
	LIST_FOREACH(inp, &sctppcbinfo.listhead, sctp_list) {
		if (inp->sctp_flags & SCTP_PCB_FLAGS_AUTO_ASCONF) {
			sctp_addr_mgmt_ep(inp, ifa, type);
		} else {
			/* this address is going away anyways... */
			if (type == SCTP_DEL_IP_ADDRESS)
				return;
			/* (temporarily) restrict this address */
			sctp_addr_mgmt_restrict_ep(inp, ifa);
		}
		/* else, not allowing automatic asconf's, so ignore */
	} /* for each inp */
}

/*
 * add/delete IP address requests from kernel (via routing change)
 * assumed that the address is non-broadcast, non-multicast
 * all addresses are passed from any type of interface-- need to filter
 * duplicate addresses may get requested
 */

void
sctp_add_ip_address(struct ifaddr *ifa)
{
	sctp_addr_mgmt(ifa, SCTP_ADD_IP_ADDRESS);
}

void
sctp_delete_ip_address(struct ifaddr *ifa)
{
	struct sctp_inpcb *inp;

	/* process the delete */
	sctp_addr_mgmt(ifa, SCTP_DEL_IP_ADDRESS);

	/*
	 * need to remove this ifaddr from any cached routes
	 * and also any from any assoc "restricted/pending" lists
	 */
	/* make sure we care about this interface... */
	if (!sctp_is_desired_interface_type(ifa)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("sctp_delete_ip_address: ignoring this interface\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* go through all our PCB's */
	SCTP_INP_INFO_RLOCK();
	LIST_FOREACH(inp, &sctppcbinfo.listhead, sctp_list) {
		struct sctp_tcb *stcb;
		struct sctp_laddr *laddr, *laddr_next;

		/* process for all associations for this endpoint */
		SCTP_INP_RLOCK(inp);
		LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
			struct sctp_nets *net;

			/* process through the nets list */
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
				rtcache_free(&net->ro); /* XXX - was clear */
			} /* for each net */
			/* process through the asoc "pending" list */
			laddr = LIST_FIRST(&stcb->asoc.sctp_local_addr_list);
			while (laddr != NULL) {
				laddr_next = LIST_NEXT(laddr, sctp_nxt_addr);
				/* remove if in use */
				if (laddr->ifa == ifa) {
					sctp_remove_laddr(laddr);
				}
				laddr = laddr_next;
			} /* while */
		} /* for each stcb */
		/* process through the inp bound addr list */
		laddr = LIST_FIRST(&inp->sctp_addr_list);
		while (laddr != NULL) {
			laddr_next = LIST_NEXT(laddr, sctp_nxt_addr);
			/* remove if in use */
			if (laddr->ifa == ifa) {
				sctp_remove_laddr(laddr);
			}
			laddr = laddr_next;
		} /* while */
		SCTP_INP_RUNLOCK(inp);
	} /* for each inp */
	SCTP_INP_INFO_RUNLOCK();
}

/*
 * sa is the sockaddr to ask the peer to set primary to
 * returns: 0 = completed, -1 = error
 */
int32_t
sctp_set_primary_ip_address_sa(struct sctp_tcb *stcb, struct sockaddr *sa)
{
	/* NOTE: we currently don't check the validity of the address! */

	/* queue an ASCONF:SET_PRIM_ADDR to be sent */
	if (!sctp_asconf_queue_add_sa(stcb, sa, SCTP_SET_PRIM_ADDR)) {
		/* set primary queuing succeeded */
		if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_OPEN) {
			sctp_timer_start(SCTP_TIMER_TYPE_ASCONF,
			    stcb->sctp_ep, stcb,
			    stcb->asoc.primary_destination);
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("set_primary_ip_address_sa: queued on tcb=%p, ",
			    stcb);
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
	} else {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("set_primary_ip_address_sa: failed to add to queue on tcb=%p, ",
			    stcb);
			sctp_print_address(sa);
		}
#endif /* SCTP_DEBUG */
		return (-1);
	}
	return (0);
}

void
sctp_set_primary_ip_address(struct ifaddr *ifa)
{
	struct sctp_inpcb *inp;

	/* make sure we care about this interface... */
	if (!sctp_is_desired_interface_type(ifa)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("set_primary_ip_address: ignoring this interface\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* go through all our PCB's */
	LIST_FOREACH(inp, &sctppcbinfo.listhead, sctp_list) {
		struct sctp_tcb *stcb;

		/* process for all associations for this endpoint */
		LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
			/* queue an ASCONF:SET_PRIM_ADDR to be sent */
			if (!sctp_asconf_queue_add(stcb, ifa,
			    SCTP_SET_PRIM_ADDR)) {
				/* set primary queuing succeeded */
				if (SCTP_GET_STATE(&stcb->asoc) ==
				    SCTP_STATE_OPEN) {
					sctp_timer_start(SCTP_TIMER_TYPE_ASCONF,
					    stcb->sctp_ep, stcb,
					    stcb->asoc.primary_destination);
				}
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("set_primary_ip_address: queued on stcb=%p, ",
					    stcb);
					sctp_print_address(ifa->ifa_addr);
				}
#endif /* SCTP_DEBUG */
			} else {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("set_primary_ip_address: failed to add to queue, ");
					sctp_print_address(ifa->ifa_addr);
				}
#endif /* SCTP_DEBUG */
			}
		} /* for each stcb */
	} /* for each inp */
}

static struct sockaddr *
sctp_find_valid_localaddr(struct sctp_tcb *stcb)
{
	struct ifnet *ifn;
	struct ifaddr *ifa;
	int s;

	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifn) {
		if (stcb->asoc.loopback_scope == 0 && ifn->if_type == IFT_LOOP) {
			/* Skip if loopback_scope not set */
			continue;
		}
		IFADDR_READER_FOREACH(ifa, ifn) {
			if (ifa->ifa_addr->sa_family == AF_INET &&
			    stcb->asoc.ipv4_addr_legal) {
				struct sockaddr_in *sin;

				sin = (struct sockaddr_in *)ifa->ifa_addr;
				if (sin->sin_addr.s_addr == 0) {
					/* skip unspecifed addresses */
					continue;
				}
				if (stcb->asoc.ipv4_local_scope == 0 &&
				    IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))
					continue;

				if (sctp_is_addr_restricted(stcb,
				    ifa->ifa_addr))
					continue;
				pserialize_read_exit(s);

				/* found a valid local v4 address to use */
				return (ifa->ifa_addr);
			} else if (ifa->ifa_addr->sa_family == AF_INET6 &&
			    stcb->asoc.ipv6_addr_legal) {
				struct sockaddr_in6 *sin6;
				struct in6_ifaddr *ifa6;

				ifa6 = (struct in6_ifaddr *)ifa;
				if (IFA6_IS_DEPRECATED(ifa6) ||
				    (ifa6->ia6_flags & (IN6_IFF_DETACHED |
				     IN6_IFF_ANYCAST | IN6_IFF_NOTREADY)))
					continue;

				sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
				if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
					/* we skip unspecifed addresses */
					continue;
				}
				if (stcb->asoc.local_scope == 0 &&
				    IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
					continue;
				if (stcb->asoc.site_scope == 0 &&
				    IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))
					continue;

				pserialize_read_exit(s);
				/* found a valid local v6 address to use */
				return (ifa->ifa_addr);
			}
		}
	}
	pserialize_read_exit(s);

	/* no valid addresses found */
	return (NULL);
}

static struct sockaddr *
sctp_find_valid_localaddr_ep(struct sctp_tcb *stcb)
{
	struct sctp_laddr *laddr;

	LIST_FOREACH(laddr, &stcb->sctp_ep->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("find_valid_localaddr_ep: laddr error\n");
			}
#endif /* SCTP_DEBUG */
			continue;
		}
		if (laddr->ifa->ifa_addr == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("find_valid_localaddr_ep: laddr->ifa error\n");
			}
#endif /* SCTP_DEBUG */
			continue;
		}
		/* is the address restricted ? */
		if (sctp_is_addr_restricted(stcb, laddr->ifa->ifa_addr))
			continue;

		/* found a valid local address to use */
		return (laddr->ifa->ifa_addr);
	}
	/* no valid addresses found */
	return (NULL);
}

/*
 * builds an ASCONF chunk from queued ASCONF params
 * returns NULL on error (no mbuf, no ASCONF params queued, etc)
 */
struct mbuf *
sctp_compose_asconf(struct sctp_tcb *stcb)
{
	struct mbuf *m_asconf, *m_asconf_chk;
	struct sctp_asconf_addr *aa;
	struct sctp_asconf_chunk *acp;
	struct sctp_asconf_paramhdr *aph;
	struct sctp_asconf_addr_param *aap;
	uint32_t p_length;
	uint32_t correlation_id = 1;		/* 0 is reserved... */
	vaddr_t ptr, lookup_ptr;
	uint8_t lookup_used = 0;

	/* are there any asconf params to send? */
	if (TAILQ_EMPTY(&stcb->asoc.asconf_queue)) {
		return (NULL);
	}

	/*
	 * get a chunk header mbuf and a cluster for the asconf params
	 * since it's simpler to fill in the asconf chunk header lookup
	 * address on the fly
	 */
	m_asconf_chk = NULL;
	MGETHDR(m_asconf_chk, M_DONTWAIT, MT_DATA);
	if (m_asconf_chk == NULL) {
		/* no mbuf's */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1)
			printf("compose_asconf: couldn't get chunk mbuf!\n");
#endif /* SCTP_DEBUG */
		return (NULL);
	}
	m_asconf = NULL;
	MGETHDR(m_asconf, M_DONTWAIT, MT_HEADER);
	if (m_asconf == NULL) {
		/* no mbuf's */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1)
			printf("compose_asconf: couldn't get mbuf!\n");
#endif /* SCTP_DEBUG */
		sctp_m_freem(m_asconf_chk);
		return (NULL);
	}
	MCLGET(m_asconf, M_DONTWAIT);
	if ((m_asconf->m_flags & M_EXT) != M_EXT) {
		/* failed to get cluster buffer */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1)
			printf("compose_asconf: couldn't get cluster!\n");
#endif /* SCTP_DEBUG */
		sctp_m_freem(m_asconf_chk);
		sctp_m_freem(m_asconf);
		return (NULL);
	}

	m_asconf_chk->m_len = sizeof(struct sctp_asconf_chunk);
	m_asconf->m_len = 0;
	acp = mtod(m_asconf_chk, struct sctp_asconf_chunk *);
	memset(acp, 0, sizeof(struct sctp_asconf_chunk));
	/* save pointers to lookup address and asconf params */
	lookup_ptr = (vaddr_t)(acp + 1);	/* after the header */
	ptr = mtod(m_asconf, vaddr_t);		/* beginning of cluster */

	/* fill in chunk header info */
	acp->ch.chunk_type = SCTP_ASCONF;
	acp->ch.chunk_flags = 0;
	acp->serial_number = htonl(stcb->asoc.asconf_seq_out);

	/* add parameters... up to smallest MTU allowed */
	TAILQ_FOREACH(aa, &stcb->asoc.asconf_queue, next) {
		/* get the parameter length */
		p_length = SCTP_SIZE32(aa->ap.aph.ph.param_length);
		/* will it fit in current chunk? */
		if (m_asconf->m_len + p_length > stcb->asoc.smallest_mtu) {
			/* won't fit, so we're done with this chunk */
			break;
		}
		/* assign (and store) a correlation id */
		aa->ap.aph.correlation_id = correlation_id++;

		/*
		 * fill in address if we're doing a delete
		 * this is a simple way for us to fill in the correlation
		 * address, which should only be used by the peer if we're
		 * deleting our source address and adding a new address
		 * (e.g. renumbering case)
		 */
		if (lookup_used == 0 &&
		    aa->ap.aph.ph.param_type == SCTP_DEL_IP_ADDRESS) {
			struct sctp_ipv6addr_param *lookup;
			uint16_t p_size, addr_size;

			lookup = (struct sctp_ipv6addr_param *)lookup_ptr;
			lookup->ph.param_type =
			    htons(aa->ap.addrp.ph.param_type);
			if (aa->ap.addrp.ph.param_type == SCTP_IPV6_ADDRESS) {
				/* copy IPv6 address */
				p_size = sizeof(struct sctp_ipv6addr_param);
				addr_size = sizeof(struct in6_addr);
			} else {
				/* copy IPv4 address */
				p_size = sizeof(struct sctp_ipv4addr_param);
				addr_size = sizeof(struct in_addr);
			}
			lookup->ph.param_length = htons(SCTP_SIZE32(p_size));
			memcpy(lookup->addr, &aa->ap.addrp.addr, addr_size);
			m_asconf_chk->m_len += SCTP_SIZE32(p_size);
			lookup_used = 1;
		}

		/* copy into current space */
		memcpy((void *)ptr, &aa->ap, p_length);

		/* network elements and update lengths */
		aph = (struct sctp_asconf_paramhdr *) ptr;
		aap = (struct sctp_asconf_addr_param *) ptr;
		/* correlation_id is transparent to peer, no htonl needed */
		aph->ph.param_type = htons(aph->ph.param_type);
		aph->ph.param_length = htons(aph->ph.param_length);
		aap->addrp.ph.param_type = htons(aap->addrp.ph.param_type);
		aap->addrp.ph.param_length = htons(aap->addrp.ph.param_length);

		m_asconf->m_len += SCTP_SIZE32(p_length);
		ptr += SCTP_SIZE32(p_length);

		/*
		 * these params are removed off the pending list upon
		 * getting an ASCONF-ACK back from the peer, just set flag
		 */
		aa->sent = 1;
	}
	/* check to see if the lookup addr has been populated yet */
	if (lookup_used == 0) {
		/* NOTE: if the address param is optional, can skip this... */
		/* add any valid (existing) address... */
		struct sctp_ipv6addr_param *lookup;
		uint16_t p_size, addr_size;
		struct sockaddr *found_addr;
		vaddr_t addr_ptr;

		if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL)
			found_addr = sctp_find_valid_localaddr(stcb);
		else
			found_addr = sctp_find_valid_localaddr_ep(stcb);

		lookup = (struct sctp_ipv6addr_param *)lookup_ptr;
		if (found_addr != NULL) {
			if (found_addr->sa_family == AF_INET6) {
				/* copy IPv6 address */
				lookup->ph.param_type =
				    htons(SCTP_IPV6_ADDRESS);
				p_size = sizeof(struct sctp_ipv6addr_param);
				addr_size = sizeof(struct in6_addr);
				addr_ptr = (vaddr_t)&((struct sockaddr_in6 *)
				    found_addr)->sin6_addr;
			} else {
				/* copy IPv4 address */
				lookup->ph.param_type =
				    htons(SCTP_IPV4_ADDRESS);
				p_size = sizeof(struct sctp_ipv4addr_param);
				addr_size = sizeof(struct in_addr);
				addr_ptr = (vaddr_t)&((struct sockaddr_in *)
				    found_addr)->sin_addr;
			}
			lookup->ph.param_length = htons(SCTP_SIZE32(p_size));
			memcpy(lookup->addr, (void *)addr_ptr, addr_size);
			m_asconf_chk->m_len += SCTP_SIZE32(p_size);
			lookup_used = 1;
		} else {
			/* uh oh... don't have any address?? */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1)
				printf("compose_asconf: no lookup addr!\n");
#endif /* SCTP_DEBUG */
			/* for now, we send a IPv4 address of 0.0.0.0 */
			lookup->ph.param_type = htons(SCTP_IPV4_ADDRESS);
			lookup->ph.param_length = htons(SCTP_SIZE32(sizeof(struct sctp_ipv4addr_param)));
			memset(lookup->addr, 0, sizeof(struct in_addr));
			m_asconf_chk->m_len += SCTP_SIZE32(sizeof(struct sctp_ipv4addr_param));
			lookup_used = 1;
		}
	}

	/* chain it all together */
	m_asconf_chk->m_next = m_asconf;
	m_asconf_chk->m_pkthdr.len = m_asconf_chk->m_len + m_asconf->m_len;
	acp->ch.chunk_length = ntohs(m_asconf_chk->m_pkthdr.len);

	/* update "sent" flag */
	stcb->asoc.asconf_sent++;

	return (m_asconf_chk);
}

/*
 * section to handle address changes before an association is up
 * eg. changes during INIT/INIT-ACK/COOKIE-ECHO handshake
 */

/*
 * processes the (local) addresses in the INIT-ACK chunk
 */
static void
sctp_process_initack_addresses(struct sctp_tcb *stcb, struct mbuf *m,
    unsigned int offset, unsigned int length)
{
	struct sctp_paramhdr tmp_param, *ph;
	uint16_t plen, ptype;
	struct sctp_ipv6addr_param addr_store;
	struct sockaddr_in6 sin6;
	struct sockaddr_in sin;
	struct sockaddr *sa;
	struct ifaddr *ifa;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
		printf("processing init-ack addresses\n");
	}
#endif /* SCTP_DEBUG */

	/* convert to upper bound */
	length += offset;

	if ((offset + sizeof(struct sctp_paramhdr)) > length) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("process_initack_addrs: invalid offset?\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}

	/* init the addresses */
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(sin6);
	sin6.sin6_port = stcb->rport;

	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = stcb->rport;

	/* go through the addresses in the init-ack */
	ph = (struct sctp_paramhdr *)sctp_m_getptr(m, offset,
	    sizeof(struct sctp_paramhdr), (uint8_t *)&tmp_param);
	while (ph != NULL) {
		ptype = ntohs(ph->param_type);
		plen = ntohs(ph->param_length);
		if (ptype == SCTP_IPV6_ADDRESS) {
			struct sctp_ipv6addr_param *a6p;
			/* get the entire IPv6 address param */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("process_initack_addrs: checking IPv6 param\n");
			}
#endif /* SCTP_DEBUG */
			a6p = (struct sctp_ipv6addr_param *)
				sctp_m_getptr(m, offset,
				    sizeof(struct sctp_ipv6addr_param),
				    (uint8_t *)&addr_store);
			if (plen != sizeof(struct sctp_ipv6addr_param) ||
			    a6p == NULL) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("process_initack_addrs: invalid IPv6 param length\n");
				}
#endif /* SCTP_DEBUG */
				return;
			}
			memcpy(&sin6.sin6_addr, a6p->addr,
			    sizeof(struct in6_addr));
			sa = (struct sockaddr *)&sin6;
		} else if (ptype == SCTP_IPV4_ADDRESS) {
			struct sctp_ipv4addr_param *a4p;
			/* get the entire IPv4 address param */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("process_initack_addrs: checking IPv4 param\n");
			}
#endif /* SCTP_DEBUG */
			a4p = (struct sctp_ipv4addr_param *)sctp_m_getptr(m, offset, sizeof(struct sctp_ipv4addr_param), (uint8_t *)&addr_store);
			if (plen != sizeof(struct sctp_ipv4addr_param) ||
			    a4p == NULL) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
					printf("process_initack_addrs: invalid IPv4 param length\n");
				}
#endif /* SCTP_DEBUG */
				return;
			}
			sin.sin_addr.s_addr = a4p->addr;
			sa = (struct sockaddr *)&sin;
		} else {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("process_initack_addrs: skipping param type=%xh\n", ptype);
			}
#endif /* SCTP_DEBUG */
			goto next_addr;
		}

		/* see if this address really (still) exists */
		ifa = sctp_find_ifa_by_addr(sa);
		if (ifa == NULL) {
			/* address doesn't exist anymore */
			int status;
			/* are ASCONFs allowed ? */
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) &&
			    stcb->asoc.peer_supports_asconf) {
				/* queue an ASCONF DEL_IP_ADDRESS */
				status = sctp_asconf_queue_add_sa(stcb, sa,
				    SCTP_DEL_IP_ADDRESS);
				/*
				 * if queued ok, and in correct state,
				 * set the ASCONF timer
				 */
				if (status == 0 &&
				    SCTP_GET_STATE(&stcb->asoc) ==
				    SCTP_STATE_OPEN) {
					sctp_timer_start(SCTP_TIMER_TYPE_ASCONF,
					    stcb->sctp_ep, stcb,
					    stcb->asoc.primary_destination);
				}
			}
		} else {
			/* address still exists */
			/*
			 * if subset bound, ep addr's managed by default
			 * if not doing ASCONF, add the address to the assoc
			 */
			if ((stcb->sctp_ep->sctp_flags &
			     SCTP_PCB_FLAGS_BOUNDALL) == 0 &&
			    (stcb->sctp_ep->sctp_flags &
			     SCTP_PCB_FLAGS_DO_ASCONF) == 0) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("process_initack_addrs: adding local addr to asoc\n");
				}
#endif /* SCTP_DEBUG */
				sctp_add_local_addr_assoc(stcb, ifa);
			}
		}

	next_addr:
		/* get next parameter */
		offset += SCTP_SIZE32(plen);
		if ((offset + sizeof(struct sctp_paramhdr)) > length)
			return;
		ph = (struct sctp_paramhdr *)sctp_m_getptr(m, offset,
		    sizeof(struct sctp_paramhdr), (uint8_t *)&tmp_param);
	} /* while */
}

/* FIX ME: need to verify return result for v6 address type if v6 disabled */
/*
 * checks to see if a specific address is in the initack address list
 * returns 1 if found, 0 if not
 */
static uint32_t
sctp_addr_in_initack(struct sctp_tcb *stcb, struct mbuf *m, unsigned int offset,
    unsigned int length, struct sockaddr *sa)
{
	struct sctp_paramhdr tmp_param, *ph;
	uint16_t plen, ptype;
	struct sctp_ipv6addr_param addr_store;
	struct sockaddr_in *sin;
	struct sctp_ipv4addr_param *a4p;
#ifdef INET6
	struct sockaddr_in6 *sin6, sin6_tmp;
	struct sctp_ipv6addr_param *a6p;
#endif /* INET6 */

	if (
#ifdef INET6
		(sa->sa_family != AF_INET6) &&
#endif /* INET6 */
		(sa->sa_family != AF_INET))
		return (0);

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
		printf("find_initack_addr: starting search for ");
		sctp_print_address(sa);
	}
#endif /* SCTP_DEBUG */
	/* convert to upper bound */
	length += offset;

	if ((offset + sizeof(struct sctp_paramhdr)) > length) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("find_initack_addr: invalid offset?\n");
		}
#endif /* SCTP_DEBUG */
		return (0);
	}

	/* go through the addresses in the init-ack */
	ph = (struct sctp_paramhdr *)sctp_m_getptr(m, offset,
	    sizeof(struct sctp_paramhdr), (uint8_t *)&tmp_param);
	while (ph != NULL) {
		ptype = ntohs(ph->param_type);
		plen = ntohs(ph->param_length);
#ifdef INET6
		if (ptype == SCTP_IPV6_ADDRESS && sa->sa_family == AF_INET6) {
			/* get the entire IPv6 address param */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("addr_in_initack: checking IPv6 param\n");
			}
#endif /* SCTP_DEBUG */
			a6p = (struct sctp_ipv6addr_param *)
				sctp_m_getptr(m, offset,
				    sizeof(struct sctp_ipv6addr_param),
				    (uint8_t *)&addr_store);
			if (plen != sizeof(struct sctp_ipv6addr_param) ||
			    ph == NULL) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("addr_in_initack: invalid IPv6 param length\n");
				}
#endif /* SCTP_DEBUG */
				return (0);
			}
			sin6 = (struct sockaddr_in6 *)sa;
			if (IN6_IS_SCOPE_LINKLOCAL(&sin6->sin6_addr)) {
				/* create a copy and clear scope */
				memcpy(&sin6_tmp, sin6,
				    sizeof(struct sockaddr_in6));
				sin6 = &sin6_tmp;
				in6_clearscope(&sin6->sin6_addr);
			}
			if (memcmp(&sin6->sin6_addr, a6p->addr,
			    sizeof(struct in6_addr)) == 0) {
				/* found it */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("addr_in_initack: found IPv6 addr\n");
				}
#endif /* SCTP_DEBUG */
				return (1);
			}
		} else
#endif /* INET6 */

		if (ptype == SCTP_IPV4_ADDRESS &&
		    sa->sa_family == AF_INET) {
			/* get the entire IPv4 address param */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("addr_in_initack: checking IPv4 param\n");
			}
#endif /* SCTP_DEBUG */
			a4p = (struct sctp_ipv4addr_param *)sctp_m_getptr(m,
			    offset, sizeof(struct sctp_ipv4addr_param),
			    (uint8_t *)&addr_store);
			if (plen != sizeof(struct sctp_ipv4addr_param) ||
			    ph == NULL) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("addr_in_initack: invalid IPv4 param length\n");
				}
#endif /* SCTP_DEBUG */
				return (0);
			}
			sin = (struct sockaddr_in *)sa;
			if (sin->sin_addr.s_addr == a4p->addr) {
				/* found it */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("addr_in_initack: found IPv4 addr\n");
				}
#endif /* SCTP_DEBUG */
				return (1);
			}
		} else {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("addr_in_initack: skipping param type=%xh\n", ptype);
			}
#endif /* SCTP_DEBUG */
		}
		/* get next parameter */
		offset += SCTP_SIZE32(plen);
		if (offset + sizeof(struct sctp_paramhdr) > length)
			return (0);
		ph = (struct sctp_paramhdr *)
			sctp_m_getptr(m, offset, sizeof(struct sctp_paramhdr),
			    (uint8_t *)&tmp_param);
	} /* while */
	/* not found! */
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
		printf("addr_in_initack: not found!\n");
	}
#endif /* SCTP_DEBUG */
	return (0);
}

/*
 * makes sure that the current endpoint local addr list is consistent
 * with the new association (eg. subset bound, asconf allowed)
 * adds addresses as necessary
 */
static void
sctp_check_address_list_ep(struct sctp_tcb *stcb, struct mbuf *m, int offset,
    int length, struct sockaddr *init_addr)
{
	struct sctp_laddr *laddr;

	/* go through the endpoint list */
	LIST_FOREACH(laddr, &stcb->sctp_ep->sctp_addr_list, sctp_nxt_addr) {
		/* be paranoid and validate the laddr */
		if (laddr->ifa == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("check_addr_list_ep: laddr->ifa is NULL");
			}
#endif
			continue;
		}
		if (laddr->ifa->ifa_addr == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
				printf("check_addr_list_ep: laddr->ifa->ifa_addr is NULL");
			}
#endif
			continue;
		}
		/* do i have it implicitly? */
		if (sctp_cmpaddr(laddr->ifa->ifa_addr, init_addr)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
				printf("check_address_list_all: skipping ");
				sctp_print_address(laddr->ifa->ifa_addr);
			}
#endif /* SCTP_DEBUG */
			continue;
		}
		/* check to see if in the init-ack */
		if (!sctp_addr_in_initack(stcb, m, offset, length,
					  laddr->ifa->ifa_addr)) {
			/* try to add it */
			sctp_addr_mgmt_assoc(stcb->sctp_ep, stcb, laddr->ifa,
			    SCTP_ADD_IP_ADDRESS);
		}
	}
}

/*
 * makes sure that the current kernel address list is consistent
 * with the new association (with all addrs bound)
 * adds addresses as necessary
 */
static void
sctp_check_address_list_all(struct sctp_tcb *stcb, struct mbuf *m, int offset,
    int length, struct sockaddr *init_addr, uint16_t local_scope,
    uint16_t site_scope, uint16_t ipv4_scope, uint16_t loopback_scope)
{
	struct ifnet *ifn;
	struct ifaddr *ifa;
	int s;

	/* go through all our known interfaces */
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifn) {
		if (loopback_scope == 0 && ifn->if_type == IFT_LOOP) {
			/* skip loopback interface */
			continue;
		}

		/* go through each interface address */
		IFADDR_READER_FOREACH(ifa, ifn) {
			/* do i have it implicitly? */
			if (sctp_cmpaddr(ifa->ifa_addr, init_addr)) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_ASCONF2) {
					printf("check_address_list_all: skipping ");
					sctp_print_address(ifa->ifa_addr);
				}
#endif /* SCTP_DEBUG */
				continue;
			}
			/* check to see if in the init-ack */
			if (!sctp_addr_in_initack(stcb, m, offset, length,
			    ifa->ifa_addr)) {
				/* try to add it */
				sctp_addr_mgmt_assoc(stcb->sctp_ep, stcb,
				    ifa, SCTP_ADD_IP_ADDRESS);
			}
		} /* end foreach ifa */
	} /* end foreach ifn */
	pserialize_read_exit(s);
}

/*
 * validates an init-ack chunk (from a cookie-echo) with current addresses
 * adds addresses from the init-ack into our local address list, if needed
 * queues asconf adds/deletes addresses as needed and makes appropriate
 * list changes for source address selection
 * m, offset: points to the start of the address list in an init-ack chunk
 * length: total length of the address params only
 * init_addr: address where my INIT-ACK was sent from
 */
void
sctp_check_address_list(struct sctp_tcb *stcb, struct mbuf *m, int offset,
    int length, struct sockaddr *init_addr, uint16_t local_scope,
    uint16_t site_scope, uint16_t ipv4_scope, uint16_t loopback_scope)
{

	/* process the local addresses in the initack */
	sctp_process_initack_addresses(stcb, m, offset, length);

	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		/* bound all case */
		sctp_check_address_list_all(stcb, m, offset, length, init_addr,
		    local_scope, site_scope, ipv4_scope, loopback_scope);
	} else {
		/* subset bound case */
	 	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) {
			/* asconf's allowed */
			sctp_check_address_list_ep(stcb, m, offset, length,
			    init_addr);
		}
		/* else, no asconfs allowed, so what we sent is what we get */
	}
}

/*
 * sctp_bindx() support
 */
uint32_t
sctp_addr_mgmt_ep_sa(struct sctp_inpcb *inp, struct sockaddr *sa, uint16_t type)
{
	struct ifaddr *ifa;

	if (sa->sa_len == 0)
		return (EINVAL);

	ifa = sctp_find_ifa_by_addr(sa);
	if (ifa != NULL) {
#ifdef INET6
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct in6_ifaddr *ifa6;
			ifa6 = (struct in6_ifaddr *)ifa;
			if (IFA6_IS_DEPRECATED(ifa6) ||
			    (ifa6->ia6_flags & (IN6_IFF_DETACHED |
			     IN6_IFF_ANYCAST | IN6_IFF_NOTREADY))) {
				/* Can't bind a non-existent addr. */
				return (EINVAL);
			}
		}
#endif /* INET6 */
		/* add this address */
		sctp_addr_mgmt_ep(inp, ifa, type);
	} else {
		/* invalid address! */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_ASCONF1) {
			printf("addr_mgmt_ep_sa: got invalid address!\n");
		}
#endif /* SCTP_DEBUG */
		return (EADDRNOTAVAIL);
	}
	return (0);
}
