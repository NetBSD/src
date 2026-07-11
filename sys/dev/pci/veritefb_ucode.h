/*	$NetBSD: veritefb_ucode.h,v 1.1 2026/07/11 15:18:21 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Rendition Verite RISC and 2D microcode interface definitions.
 *
 * Command numbers are taken from the dispatch table extracted from the
 * V2x00 2D microcode blob itself (v20002d.uc, dispatch table at 0x5140).
 */

#ifndef VERITEFB_UCODE_H
#define VERITEFB_UCODE_H

/*
 * VRAM layout: the first VFB_MC_SIZE bytes are reserved for microcode,
 * the framebuffer starts right after...  
 *
 * The context-switch monitor (csucode) lives at VFB_CSUCODE_BASE.
 */
#define VFB_MC_SIZE		0x10000
#define VFB_CSUCODE_BASE	0x800
#define VFB_CSUCODE_SEM0	0x7f8
#define VFB_CSUCODE_SEM1	0x7fc

/* csucode monitor commands (first FIFO word after starting the monitor) */
#define VFB_CSUCODE_INIT	0	/* a1=ctx store area, a2, entry */
#define VFB_CSUCODE_SYNC	2	/* wait for pixel engine idle */

/*
 * Host->RISC commands: 32-bit words into the input FIFO,
 * command index in bits 15:0, first parameter in bits 31:16.
 */
#define VFB_CMDW(param, cmd)	(((uint32_t)(uint16_t)(param) << 16) | \
				 (uint16_t)(cmd))
#define VFB_P2(x, y)		VFB_CMDW(x, y)

#define VCMD_FILLRECTSOLID	1	/* prefer the ROP variant */
#define VCMD_PIXENGSYNC		8	/* -> 0xffffffff in output FIFO */
#define VCMD_GETPIXEL		9	/* P2(x,y) -> pixel in output FIFO */
#define VCMD_SETSCREENINFO	10
#define VCMD_SCREENBLT		12
#define VCMD_MONOSOURCEBLT	22
#define VCMD_SETUP		32	/* 6 words total */
#define VCMD_SETPALETTE		33
#define VCMD_SETPIXEL		34
#define VCMD_DRAWGLYPHS		35
#define VCMD_SETCLIPPING	36
#define VCMD_FILLRECTSOLIDROP	41

#define VFB_SYNC_TOKEN		0xffffffffU

/* 2D blob layout facts, for RISC program counter classification. */
#define VFB_UC_BASE		0x1000	/* link base; dispatch loop here */
#define VFB_UC_DISPATCH_END	0x101c	/* end of the dispatch idle loop */
#define VFB_UC_TRAP		0x1050	/* invalid-command self-branch */
#define VFB_UC_TRAP_END		0x1058	/* including the delay slot */
#define VFB_UC_END		0x5000	/* end of the command handlers */
#define VFB_RISC_ROM_BASE	0xfffe0000 /* boot ROM, RISC view */

/* DstMode for 2D: ALUMode in bits 3:0, other bits zero. */
#define VFB_ROP_COPY		0xc

/*
 * RISC register file indices and instruction encodings, for driving the
 * processor through the STATEINDEX/STATEDATA debug port.
 */
#define VRISC_FLAG	37	/* flags register */
#define VRISC_SP	252	/* stack pointer / scratch */
#define VRISC_RA	254	/* link / scratch */
#define VRISC_FP	255	/* frame pointer / scratch */

#define VRISC_NOP	0x00000000	/* addi zero, zero, 0 */

#define VRISC_ADDI_OP	0x00
#define VRISC_ADD_OP	0x10
#define VRISC_ANDN_OP	0x12
#define VRISC_OR_OP	0x15
#define VRISC_ADDIFI_OP	0x40
#define VRISC_ADDSL8_OP	0x4b
#define VRISC_SPRI_OP	0x4f
#define VRISC_JMP_OP	0x6c
#define VRISC_LB_OP	0x70
#define VRISC_LH_OP	0x71
#define VRISC_LW_OP	0x72
#define VRISC_LI_OP	0x76
#define VRISC_LUI_OP	0x77
#define VRISC_SB_OP	0x78
#define VRISC_SH_OP	0x79
#define VRISC_SW_OP	0x7a

#define VRISC_INT(op, d, s2, s1)					\
	(((uint32_t)(op) << 24) | ((uint32_t)(d) << 16) |		\
	 ((uint32_t)(s2) << 8) | ((uint32_t)(s1) & 0xff))
#define VRISC_LD(op, d, off8, s1)					\
	(((uint32_t)(op) << 24) | ((uint32_t)(d) << 16) |		\
	 (((uint32_t)(off8) & 0xff) << 8) | ((uint32_t)(s1)))
#define VRISC_ST(op, off8, s2, s1)					\
	(((uint32_t)(op) << 24) | (((uint32_t)(off8) & 0xff) << 16) |	\
	 ((uint32_t)(s2) << 8) | ((uint32_t)(s1)))
#define VRISC_LI(op, d, imm16)						\
	(((uint32_t)(op) << 24) | ((uint32_t)(d) << 16) |		\
	 ((uint32_t)(imm16) & 0xffff))
#define VRISC_JMP(addr24)						\
	(((uint32_t)VRISC_JMP_OP << 24) | ((uint32_t)(addr24)))

/* Instruction cache */
#define VRISC_ICACHESIZE	2048
#define VRISC_ICACHELINESIZE	32
#define VRISC_ICACHE_ONOFF_MASK	(((uint32_t)1 << 17) | (1 << 3))

/*
 * Context-switch monitor microcode ("csucode")
 */
static const uint32_t veritefb_csucode[] = {
	0x10802100, 0x5d808000, 0x4c808002, 0x6b820000,
	0x00818002, 0x45818103, 0x10828281, 0x6f000082,
	0x00000000, 0x62000500, 0x00000000, 0x62000300,
	0x00000000, 0x62000800, 0x00000000, 0x10812100,
	0x10822100, 0x10c02100, 0x6ffe00c0, 0x00000000,
	0x62ffeb00, 0x00000000, 0x04812502, 0x61fffe81,
	0x00000000, 0x10218000, 0x00000000, 0x00000000,
	0x62ffe300, 0x00000000,
};

#endif /* VERITEFB_UCODE_H */
