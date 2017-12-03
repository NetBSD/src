/*	$NetBSD: db_disasm.h,v 1.7.22.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by
 * Bertram Barth.
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



#define SIZE_BYTE	 1		/* Byte */
#define SIZE_WORD	 2		/* Word */
#define SIZE_LONG 	 4		/* Longword */
#define SIZE_QWORD	 8		/* Quadword */
#define SIZE_OWORD	16		/* Octaword */

/*
 * The VAX instruction set has a variable length instruction format which 
 * may be as short as one byte and as long as needed depending on the type 
 * of instruction. [...] Each instruction consists of an opcode followed 
 * by zero to six operand specifiers whose number and type depend on the 
 * opcode. All operand specidiers are, themselves, of the same format -- 
 * i.e. an address mode plus additional information.
 *
 * [VAX Architecture Handbook, p.52:  Instruction Format]
 */

typedef const struct {
	const char *mnemonic;
	const char *argdesc;
} vax_instr_t;

extern vax_instr_t vax_inst[256];
extern vax_instr_t vax_inst2[0x56];

long skip_opcode(long);

/*
 * reasonably simple macro to gather all the reserved two-byte opcodes
 * into only a few table entries...
 */
#define	INDEX_OPCODE(x)	\
	(((x) & 0xff00) == 0xfe00) ? 0 : \
	((x) < 0xfd30) ? 0 : \
	((x) < 0xfd80) ? (x) - 0xfd30 : \
	((x) == 0xfd98) ? 0x50 : \
	((x) == 0xfd99) ? 0x51 : \
	((x) == 0xfdf6) ? 0x52 : \
	((x) == 0xfdf7) ? 0x53 : \
	((x) == 0xfffd) ? 0x54 : \
	((x) == 0xfffe) ? 0x55 : 0
