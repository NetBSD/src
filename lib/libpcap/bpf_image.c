/*	$NetBSD: bpf_image.c,v 1.6 2000/10/10 19:12:48 is Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1992, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char rcsid[] =
    "@(#) Header: bpf_image.c,v 1.22 96/09/26 23:27:56 leres Exp  (LBL)";
#else
__RCSID("$NetBSD: bpf_image.c,v 1.6 2000/10/10 19:12:48 is Exp $");
#endif
#endif

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <string.h>

#include "pcap-int.h"

#include "gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

char *
bpf_image(p, n)
	struct bpf_insn *p;
	int n;
{
	int v;
	char *op;
	static char image[256];
	char operand[64];

	operand[0] = 0;
	
	v = p->k;
	switch (p->code) {

	default:
		op = "unimp";
		snprintf(operand, sizeof operand, "0x%x", p->code);
		break;

	case BPF_RET|BPF_K:
		op = "ret";
		goto immediate;

	case BPF_RET|BPF_A:
		op = "ret";
		break;

	case BPF_LD|BPF_W|BPF_ABS:
		op = "ld";
	abs:
		snprintf(operand, sizeof operand, "[%d]", v);
		break;

	case BPF_LD|BPF_H|BPF_ABS:
		op = "ldh";
		goto abs;

	case BPF_LD|BPF_B|BPF_ABS:
		op = "ldb";
		goto abs;

	case BPF_LD|BPF_W|BPF_LEN:
		op = "ld";
		strlcpy(operand, "#pktlen", sizeof(operand));
		break;

	case BPF_LD|BPF_W|BPF_IND:
		op = "ld";
	offset:
		snprintf(operand, sizeof(operand), "[x + %d]", v);
		break;

	case BPF_LD|BPF_H|BPF_IND:
		op = "ldh";
		goto offset;

	case BPF_LD|BPF_B|BPF_IND:
		op = "ldb";
		goto offset;

	case BPF_LD|BPF_IMM:
		op = "ld";
		goto immediate_hex;		

	case BPF_LDX|BPF_IMM:
		op = "ldx";
		goto immediate_hex;

	case BPF_LDX|BPF_MSH|BPF_B:
		op = "ldxb";
		snprintf(operand, sizeof(operand), "4*([%d]&0xf)", v);
		break;

	case BPF_LD|BPF_MEM:
		op = "ld";
	mem:
		snprintf(operand, sizeof(operand), "M[%d]", v);
		break;

	case BPF_LDX|BPF_MEM:
		op = "ldx";
		goto mem;

	case BPF_ST:
		op = "st";
		goto mem;
		
	case BPF_STX:
		op = "stx";
		goto mem;
		
	case BPF_JMP|BPF_JA:
		op = "ja";
		snprintf(operand, sizeof(operand), "%d", n + 1 + p->k);
		break;

	case BPF_JMP|BPF_JGT|BPF_K:
		op = "jgt";
		goto immediate_hex;

	case BPF_JMP|BPF_JGE|BPF_K:
		op = "jge";
		goto immediate_hex;

	case BPF_JMP|BPF_JEQ|BPF_K:
		op = "jeq";
		goto immediate_hex;		

	case BPF_JMP|BPF_JSET|BPF_K:
		op = "jset";
		goto immediate_hex;

	case BPF_JMP|BPF_JGT|BPF_X:
		op = "jgt";
	x:
		strlcpy(operand, "x", sizeof(operand));
		break;

	case BPF_JMP|BPF_JGE|BPF_X:
		op = "jge";
		goto x;

	case BPF_JMP|BPF_JEQ|BPF_X:
		op = "jeq";
		goto x;
		
	case BPF_JMP|BPF_JSET|BPF_X:
		op = "jset";
		goto x;

	case BPF_ALU|BPF_ADD|BPF_X:
		op = "add";
		goto x;

	case BPF_ALU|BPF_SUB|BPF_X:
		op = "sub";
		goto x;

	case BPF_ALU|BPF_MUL|BPF_X:
		op = "mul";
		goto x;

	case BPF_ALU|BPF_DIV|BPF_X:
		op = "div";
		goto x;

	case BPF_ALU|BPF_AND|BPF_X:
		op = "and";
		goto x;

	case BPF_ALU|BPF_OR|BPF_X:
		op = "or";
		goto x;

	case BPF_ALU|BPF_LSH|BPF_X:
		op = "lsh";
		goto x;

	case BPF_ALU|BPF_RSH|BPF_X:
		op = "rsh";
		goto x;

	case BPF_ALU|BPF_ADD|BPF_K:
		op = "add";
		goto immediate;

	case BPF_ALU|BPF_SUB|BPF_K:
		op = "sub";
		goto immediate;

	case BPF_ALU|BPF_MUL|BPF_K:
		op = "mul";
		goto immediate;

	case BPF_ALU|BPF_DIV|BPF_K:
		op = "div";
	immediate:
		snprintf(operand, sizeof operand, "#%d", v);
		break;

	case BPF_ALU|BPF_AND|BPF_K:
		op = "and";
		goto immediate_hex;

	case BPF_ALU|BPF_OR|BPF_K:
		op = "or";
	immediate_hex:
		snprintf(operand, sizeof operand, "#0x%x", v);		
		break;

	case BPF_ALU|BPF_LSH|BPF_K:
		op = "lsh";
		goto immediate;

	case BPF_ALU|BPF_RSH|BPF_K:
		op = "rsh";
		goto immediate;
		break;

	case BPF_ALU|BPF_NEG:
		op = "neg";
		break;

	case BPF_MISC|BPF_TAX:
		op = "tax";
		break;

	case BPF_MISC|BPF_TXA:
		op = "txa";
		break;
	}

	if (BPF_CLASS(p->code) == BPF_JMP &&
	    BPF_OP(p->code) != BPF_JA)	
		(void)snprintf(image, sizeof image,
		    "(%03d) %-8s %-16s jt %d\tjf %d",
		    n, op, operand, n + 1 + p->jt, n + 1 + p->jf);
	else
		(void)snprintf(image, sizeof image,
		    "(%03d) %-8s %s",
		    n, op, operand);
	return image;
}
