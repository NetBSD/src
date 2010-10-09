/*	$NetBSD: npf_ncode.h,v 1.2.2.2 2010/10/09 03:32:37 yamt Exp $	*/

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
 * NPF n-code interface.
 *
 * WARNING: Backwards compatibilty is not _yet_ maintained and instructions
 * or their codes may (or may not) change.  Expect ABI breakage.
 */

#ifndef _NPF_NCODE_H_
#define _NPF_NCODE_H_

#include "npf.h"

/* N-code processing, validation & building. */
int	npf_ncode_process(npf_cache_t *, const void *, nbuf_t *, const int);
int	npf_ncode_validate(const void *, size_t, int *);

void *	npf_ncode_alloc(size_t);
void	npf_ncode_free(void *, size_t);

/* Error codes. */
#define	NPF_ERR_OPCODE		-1	/* Invalid instruction. */
#define	NPF_ERR_JUMP		-2	/* Invalid jump (e.g. out of range). */
#define	NPF_ERR_REG		-3	/* Invalid register. */
#define	NPF_ERR_INVAL		-4	/* Invalid argument value. */
#define	NPF_ERR_RANGE		-5	/* Processing out of range. */

/* Number of registers: [0..N] */
#define	NPF_NREGS		4

/* Maximum loop count. */
#define	NPF_LOOP_LIMIT		100

/* Shift to check if CISC-like instruction. */
#define	NPF_CISC_SHIFT		7
#define	NPF_CISC_OPCODE(insn)	(insn >> NPF_CISC_SHIFT)

/*
 * RISC-like n-code instructions.
 */

/* Return, advance, jump, tag and invalidate instructions. */
#define	NPF_OPCODE_RET			0x00
#define	NPF_OPCODE_ADVR			0x01
#define	NPF_OPCODE_J			0x02
#define	NPF_OPCODE_INVL			0x03
#define	NPF_OPCODE_TAG			0x04

/* Set and load instructions. */
#define	NPF_OPCODE_MOV			0x10
#define	NPF_OPCODE_LOAD			0x11

/* Compare and jump instructions. */
#define	NPF_OPCODE_CMP			0x21
#define	NPF_OPCODE_CMPR			0x22
#define	NPF_OPCODE_BEQ			0x23
#define	NPF_OPCODE_BNE			0x24
#define	NPF_OPCODE_BGT			0x25
#define	NPF_OPCODE_BLT			0x26

/* Bitwise instructions. */
#define	NPF_OPCODE_AND			0x30

/*
 * CISC-like n-code instructions.
 */

#define	NPF_OPCODE_ETHER		0x80

#define	NPF_OPCODE_IP4MASK		0x90
#define	NPF_OPCODE_IP4TABLE		0x91
#define	NPF_OPCODE_ICMP4		0x92

#define	NPF_OPCODE_TCP_PORTS		0xa0
#define	NPF_OPCODE_UDP_PORTS		0xa1
#define	NPF_OPCODE_TCP_FLAGS		0xa2

#endif
