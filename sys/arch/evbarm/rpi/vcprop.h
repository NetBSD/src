/*	$NetBSD: vcprop.h,v 1.1.2.2 2012/10/30 17:19:26 yamt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * Mailbox property interface
 */

#ifndef	_EVBARM_RPI_VCPROP_H_
#define	_EVBARM_RPI_VCPROP_H_

struct vcprop_tag {
	uint32_t vpt_tag;
#define	VCPROPTAG_NULL			0x00000000
#define	VCPROPTAG_GET_FIRMWAREREV	0x00000001
#define	VCPROPTAG_GET_BOARDMODEL	0x00010001
#define	VCPROPTAG_GET_BOARDREVISION	0x00010002
#define	VCPROPTAG_GET_MACADDRESS	0x00010003
#define	VCPROPTAG_GET_BOARDSERIAL	0x00010004
#define	VCPROPTAG_GET_ARMMEMORY		0x00010005
#define	VCPROPTAG_GET_VCMEMORY		0x00010006
#define	VCPROPTAG_GET_CLOCKS		0x00010007
	
#define	VCPROPTAG_GET_CMDLINE		0x00050001
#define	VCPROPTAG_GET_DMACHAN		0x00060001
	uint32_t vpt_len;
	uint32_t vpt_rcode;
#define	VCPROPTAG_REQUEST	(0U << 31)
#define	VCPROPTAG_RESPONSE	(1U << 31)

};

#define VCPROPTAG_LEN(x) (sizeof((x)) - sizeof(struct vcprop_tag))

struct vcprop_memory {
	uint32_t base;
	uint32_t size;
};

#define	VCPROP_MAXMEMBLOCKS 4
struct vcprop_tag_memory {
	struct vcprop_tag tag;
	struct vcprop_memory mem[VCPROP_MAXMEMBLOCKS];
};

struct vcprop_tag_fwrev {
	struct vcprop_tag tag;
	uint32_t rev;
};

struct vcprop_tag_boardmodel {
	struct vcprop_tag tag;
	uint32_t model;
} ;

struct vcprop_tag_boardrev {
	struct vcprop_tag tag;
	uint32_t rev;
} ;

struct vcprop_tag_macaddr {
	struct vcprop_tag tag;
	uint64_t addr;
};

struct vcprop_tag_boardserial {
	struct vcprop_tag tag;
	uint64_t sn;
};

struct vcprop_clock {
	uint32_t pclk;
	uint32_t cclk;
};

#define	VCPROP_MAXCLOCKS 16
struct vcprop_tag_clock {
	struct vcprop_tag tag;
	struct vcprop_clock clk[VCPROP_MAXCLOCKS];
};

#define	VCPROP_MAXCMDLINE 256
struct vcprop_tag_cmdline {
	struct vcprop_tag tag;
	uint8_t cmdline[VCPROP_MAXCMDLINE];
};

struct vcprop_tag_dmachan {
	struct vcprop_tag tag;
	uint32_t mask;
};

struct vcprop_buffer_hdr {
	uint32_t vpb_len;
	uint32_t vpb_rcode;
#define	VCPROP_PROCESS_REQUEST 0
#define VCPROP_REQ_SUCCESS	(1U << 31)
#define VCPROP_REQ_EPARSE	(1U << 0)
};

static inline bool
vcprop_buffer_success_p(struct vcprop_buffer_hdr *vpbh)
{

	return (vpbh->vpb_rcode & VCPROP_REQ_SUCCESS);
}

static inline bool
vcprop_tag_success_p(struct vcprop_tag *vpbt)
{

	return (vpbt->vpt_rcode & VCPROPTAG_RESPONSE);
}

static inline size_t
vcprop_tag_resplen(struct vcprop_tag *vpbt)
{

	return (vpbt->vpt_rcode & ~VCPROPTAG_RESPONSE);
}

#endif	/* _EVBARM_RPI_VCPROP_H_ */
