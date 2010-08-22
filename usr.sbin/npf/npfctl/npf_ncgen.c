/*	$NetBSD: npf_ncgen.c,v 1.1 2010/08/22 18:56:23 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * N-code generation.
 *
 * WARNING: Update npfctl_calc_ncsize() and npfctl_failure_offset()
 * calculations, when changing generation routines.
 */

#include <sys/types.h>

#include "npfctl.h"

/*
 * npfctl_calc_ncsize: calculate size required for the n-code.
 */
size_t
npfctl_calc_ncsize(int nblocks[])
{
	/*
	 * Blocks:
	 * - 13 words by npfctl_gennc_ether(), single initial block.
	 * - 5 words each by npfctl_gennc_ports/tbl(), stored in nblocks[0].
	 * - 6 words each by npfctl_gennc_v4cidr(), stored in nblocks[1].
	 * - 4 words by npfctl_gennc_complete(), single last fragment.
	 */
	return nblocks[0] * 5 * sizeof(uint32_t) +
	    nblocks[1] * 6 * sizeof(uint32_t) +
	    13 * sizeof(uint32_t) +
	    4 * sizeof(uint32_t);
}

/*
 * npfctl_failure_offset: calculate offset value to the failure block.
 */
size_t
npfctl_failure_offset(int nblocks[])
{
	size_t tblport_blocks, v4cidr_blocks;
	/*
	 * Take into account all blocks (plus 2 words for comparison each),
	 * and additional 4 words to skip the last comparison and success path.
	 */
	tblport_blocks = (3 + 2) * nblocks[0];
	v4cidr_blocks = (4 + 2) * nblocks[1];
	return tblport_blocks + v4cidr_blocks + 4;
}

/*
 * npfctl_gennc_ether: initial n-code fragment to check Ethernet frame.
 */
void
npfctl_gennc_ether(void **ncptr, int foff, uint16_t ethertype)
{
	uint32_t *nc = *ncptr;

	/* NPF handler will set REG_0 to either NPF_LAYER_2 or NPF_LAYER_3. */
	*nc++ = NPF_OPCODE_CMP;
	*nc++ = NPF_LAYER_3;
	*nc++ = 0;

	/* Skip all further code, if layer 3. */
	*nc++ = NPF_OPCODE_BEQ;
	*nc++ = 0x0a;

	/* Otherwise, assume layer 2 and perform NPF_OPCODE_ETHER. */
	*nc++ = NPF_OPCODE_ETHER;
	*nc++ = 0x00;		/* reserved */
	*nc++ = 0x00;		/* reserved */
	*nc++ = ethertype;

	/* Fail (+ 2 words of ADVR) or advance to layer 3 (IPv4) header. */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = foff + 2;
	/* Offset to the header is returned by NPF_OPCODE_ETHER in REG_3. */
	*nc++ = NPF_OPCODE_ADVR;
	*nc++ = 3;

	/* + 13 words. */
	*ncptr = (void *)nc;
}

/*
 * npfctl_gennc_v4cidr: fragment to match IPv4 CIDR.
 */
void
npfctl_gennc_v4cidr(void **ncptr, int foff,
    in_addr_t netaddr, in_addr_t subnet, bool sd)
{
	uint32_t *nc = *ncptr;

	/* OP, direction, netaddr/subnet (4 words) */
	*nc++ = NPF_OPCODE_IP4MASK;
	*nc++ = (sd ? 0x01 : 0x00);
	*nc++ = netaddr;
	*nc++ = subnet;

	/* If not equal, jump to failure block, continue otherwise (2 words). */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = foff;

	/* + 6 words. */
	*ncptr = (void *)nc;
}

/*
 * npfctl_gennc_ports: fragment to match TCP or UDP ports.
 */
void
npfctl_gennc_ports(void **ncptr, int foff,
    in_port_t pfrom, in_port_t pto, bool tcpudp, bool sd)
{
	uint32_t *nc = *ncptr;

	/* OP, direction, port range (3 words). */
	*nc++ = (tcpudp ? NPF_OPCODE_TCP_PORTS : NPF_OPCODE_UDP_PORTS);
	*nc++ = (sd ? 0x01 : 0x00);
	*nc++ = ((uint32_t)pfrom << 16) | pto;

	/* If not equal, jump to failure block, continue otherwise (2 words). */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = foff;

	/* + 5 words. */
	*ncptr = (void *)nc;
}

/*
 * npfctl_gennc_icmp: fragment to match ICMP code and type.
 */
void
npfctl_gennc_icmp(void **ncptr, int foff, int code, int type)
{
	uint32_t *nc = *ncptr;

	/* OP, code, type (3 words) */
	*nc++ = NPF_OPCODE_ICMP4;
	*nc++ = code;
	*nc++ = type;

	/* If not equal, jump to failure block, continue otherwise (2 words). */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = foff;

	/* + 5 words. */
	*ncptr = (void *)nc;
}

/*
 * npfctl_gennc_tbl: fragment to match IPv4 source/destination address of
 * the packet against table specified by ID.
 */
void
npfctl_gennc_tbl(void **ncptr, int foff, u_int tid, bool sd)
{
	uint32_t *nc = *ncptr;

	/* OP, direction, table ID (3 words). */
	*nc++ = NPF_OPCODE_IP4TABLE;
	*nc++ = (sd ? 0x01 : 0x00);
	*nc++ = tid;

	/* If not equal, jump to failure block, continue otherwise (2 words). */
	*nc++ = NPF_OPCODE_BNE;
	*nc++ = foff;

	/* + 5 words. */
	*ncptr = (void *)nc;
}

/*
 * npfctl_gennc_complete: append success and failure fragments.
 */
void
npfctl_gennc_complete(void **ncptr)
{
	uint32_t *nc = *ncptr;

	/* Success path (return 0x0). */
	*nc++ = NPF_OPCODE_RET;
	*nc++ = 0x0;

	/* Failure path (return 0xff). */
	*nc++ = NPF_OPCODE_RET;
	*nc++ = 0xff;

	/* + 4 words. */
	*ncptr = (void *)nc;
}
