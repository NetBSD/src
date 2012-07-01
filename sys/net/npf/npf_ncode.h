/*	$NetBSD: npf_ncode.h,v 1.9 2012/07/01 23:21:06 rmind Exp $	*/

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

#if defined(_KERNEL)
/*
 * N-code processing, validation & building.
 */
void *	npf_ncode_alloc(size_t);
void	npf_ncode_free(void *, size_t);

int	npf_ncode_process(npf_cache_t *, const void *, nbuf_t *, const int);
int	npf_ncode_validate(const void *, size_t, int *);

#endif

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
#define	NPF_OPCODE_MOVE			0x10
#define	NPF_OPCODE_LW			0x11

/* Compare and jump instructions. */
#define	NPF_OPCODE_CMP			0x21
#define	NPF_OPCODE_CMPR			0x22
#define	NPF_OPCODE_BEQ			0x23
#define	NPF_OPCODE_BNE			0x24
#define	NPF_OPCODE_BGT			0x25
#define	NPF_OPCODE_BLT			0x26

/* Arithmetic instructions. */
#define	NPF_OPCODE_ADD			0x30
#define	NPF_OPCODE_SUB			0x31
#define	NPF_OPCODE_MULT			0x32
#define	NPF_OPCODE_DIV			0x33

/* Bitwise instructions. */
#define	NPF_OPCODE_NOT			0x40
#define	NPF_OPCODE_AND			0x41
#define	NPF_OPCODE_OR			0x42
#define	NPF_OPCODE_XOR			0x43
#define	NPF_OPCODE_SLL			0x44
#define	NPF_OPCODE_SRL			0x45

/*
 * CISC-like n-code instructions.
 */

#define	NPF_OPCODE_ETHER		0x80
#define	NPF_OPCODE_PROTO		0x81

#define	NPF_OPCODE_IP4MASK		0x90
#define	NPF_OPCODE_TABLE		0x91
#define	NPF_OPCODE_ICMP4		0x92
#define	NPF_OPCODE_IP6MASK		0x93

#define	NPF_OPCODE_TCP_PORTS		0xa0
#define	NPF_OPCODE_UDP_PORTS		0xa1
#define	NPF_OPCODE_TCP_FLAGS		0xa2

#ifdef NPF_OPCODES_STRINGS

# define	NPF_OPERAND_NONE		0
# define	NPF_OPERAND_REGISTER		1
# define	NPF_OPERAND_KEY			2
# define	NPF_OPERAND_VALUE		3
# define	NPF_OPERAND_SD			4
# define		NPF_OPERAND_SD_SRC		1
# define		NPF_OPERAND_SD_DST		0
# define	NPF_OPERAND_REL_ADDRESS		5
# define	NPF_OPERAND_NET_ADDRESS4	6
# define	NPF_OPERAND_NET_ADDRESS6	7
# define	NPF_OPERAND_ETHER_TYPE		8
# define	NPF_OPERAND_SUBNET		9
# define	NPF_OPERAND_LENGTH		10
# define	NPF_OPERAND_TABLE_ID		11
# define	NPF_OPERAND_ICMP4_TYPE_CODE	12
# define	NPF_OPERAND_TCP_FLAGS_MASK	13
# define	NPF_OPERAND_PORT_RANGE		14
# define	NPF_OPERAND_PROTO		15

static const struct npf_instruction {
	const char *	name;
	uint8_t		op[4];
} npf_instructions[] = {
	[NPF_OPCODE_RET] = {
		.name = "ret",
		.op = {
			[0] = NPF_OPERAND_VALUE,
		},
	},
	[NPF_OPCODE_ADVR] = {
		.name = "advr",
		.op = {
			[0] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_J] = {
		.name = "j",
		.op = {
			[0] = NPF_OPERAND_REL_ADDRESS,
		},
	},
	[NPF_OPCODE_INVL] = {
		.name = "invl",
	},
	[NPF_OPCODE_TAG] = {
		.name = "tag",
		.op = {
			[0] = NPF_OPERAND_KEY,
			[1] = NPF_OPERAND_VALUE,
		},
	},
	[NPF_OPCODE_MOVE] = {
		.name = "move",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_LW] = {
		.name = "lw",
		.op = {
			[0] = NPF_OPERAND_LENGTH,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_CMP] = {
		.name = "cmp",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_CMPR] = {
		.name = "cmpr",
		.op = {
			[0] = NPF_OPERAND_REGISTER,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_BEQ] = {
		.name = "beq",
		.op = {
			[0] = NPF_OPERAND_REL_ADDRESS,
		},
	},
	[NPF_OPCODE_BNE] = {
		.name = "bne",
		.op = {
			[0] = NPF_OPERAND_REL_ADDRESS,
		},
	},
	[NPF_OPCODE_BGT] = {
		.name = "bge",
		.op = {
			[0] = NPF_OPERAND_REL_ADDRESS,
		},
	},
	[NPF_OPCODE_BLT] = {
		.name = "blt",
		.op = {
			[0] = NPF_OPERAND_REL_ADDRESS,
		},
	},
	[NPF_OPCODE_ADD] = {
		.name = "add",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_SUB] = {
		.name = "sub",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_MULT] = {
		.name = "mult",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_DIV] = {
		.name = "div",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_NOT] = {
		.name = "not",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_AND] = {
		.name = "and",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_OR] = {
		.name = "or",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_XOR] = {
		.name = "xor",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_SLL] = {
		.name = "sll",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_SRL] = {
		.name = "srl",
		.op = {
			[0] = NPF_OPERAND_VALUE,
			[1] = NPF_OPERAND_REGISTER,
		},
	},
	[NPF_OPCODE_ETHER] = {
		.name = "ether",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_NET_ADDRESS4,
			[2] = NPF_OPERAND_ETHER_TYPE,
		},
	},
	[NPF_OPCODE_PROTO] = {
		.name = "proto",
		.op = {
			[0] = NPF_OPERAND_PROTO,
		},
	},
	[NPF_OPCODE_IP4MASK] = {
		.name = "ip4mask",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_NET_ADDRESS4,
			[2] = NPF_OPERAND_SUBNET,
		},
	},
	[NPF_OPCODE_TABLE] = {
		.name = "table",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_TABLE_ID,
		},
	},
	[NPF_OPCODE_ICMP4] = {
		.name = "icmp4",
		.op = {
			[0] = NPF_OPERAND_ICMP4_TYPE_CODE,
		},
	},
	[NPF_OPCODE_IP6MASK] = {
		.name = "ip6mask",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_NET_ADDRESS6,
			[2] = NPF_OPERAND_SUBNET,
		},
	},
	[NPF_OPCODE_TCP_PORTS] = {
		.name = "tcp_ports",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_PORT_RANGE,
		},
	},
	[NPF_OPCODE_UDP_PORTS] = {
		.name = "udp_ports",
		.op = {
			[0] = NPF_OPERAND_SD,
			[1] = NPF_OPERAND_PORT_RANGE,
		},
	},
	[NPF_OPCODE_TCP_FLAGS] = {
		.name = "tcp_flags",
		.op = {
			[0] = NPF_OPERAND_TCP_FLAGS_MASK,
		},
	},
};
#endif /* NPF_OPCODES_STRINGS */

#endif /* _NET_NPF_NCODE_H_ */
