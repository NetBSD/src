/*	$NetBSD: pcb.h,v 1.1 2003/08/19 10:53:06 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _PDP10_PCB_H_
#define _PDP10_PCB_H_

/*
 * The "user process table" for extended TOPS-20.
 */
struct pcb {
	int pcb_pad1[0420];	/* Page mapping in TOPS-10 */
	int pcb_luuo;		/* Address of LUUO block */
	int pcb_ovtrap;		/* Arithmetic overflow trap insn */
	int pcb_ovstack;	/* Stack overflow trap insn */
	int pcb_trap3;		/* User trap 3 instruction */
	int pcb_muuo_flop;	/* MUUO flags/opcode */
	int pcb_muuo_oldpc;	/* MUUO old PC */
	int pcb_muuo_e;		/* MUUO E address */
	int pcb_muuo_pcw;	/* MUUO Process Context Word */
	int pcb_knotrap;	/* Kernel no trap MUUO new PC */
	int pcb_ktrap;		/* Kernel trap MUUO new PC */
	int pcb_snotrap;	/* Supervisor no trap MUUO new PC */
	int pcb_strap;		/* Supervisor trap MUUO new PC */
	int pcb_cnotrap;	/* Concealed no trap MUUO new PC */
	int pcb_ctrap;		/* Concealed trap MUUO new PC */
	int pcb_pnotrap;	/* Public no trap MUUO new PC */
	int pcb_ptrap;		/* Public trap MUUO new PC */
	int pcb_pad2[040];	/* Reserved */
	int pcb_pfw;		/* Page fail word */
	int pcb_pff;		/* Page fail flags */
	int pcb_pfopc;		/* Page fail old pc */
	int pcb_pfnpc;		/* Page fail new pc */
	int pcb_upet[2];	/* User Process Execution Time */
	int pcb_umrc[2];	/* User Memory Reference Count */
	int pcb_pad3[030];	/* Reserved */
	int pcb_section[040];	/* Section pointers */
	int pcb_pad4[0200];	/* Reserved */
};

/*
 * The "executive process table" for extended TOPS-20.
 */
struct ept {
	int ept_channel[8][4];	/* Channel logout areas */
	int ept_pad1[2];	/* Reserved */
	int ept_spii[016];	/* Standard Priority Interrupt Instructions */
	int ept_fcbfw[4];	/* Four channel block fill word */
	int ept_pad2[054];	/* Reserved */
	int ept_dte20[040];	/* Four DTE20 control blocks */
	int ept_pad3[0221];	/* Reserved */
	int ept_earov;		/* Executive Arithmetic Overflow Trap Insn */
	int ept_esov;		/* Executive Stack Overflow Trap Insn */
	int ept_etrap3;		/* Executive Trap 3 Trap Insn */
	int ept_pad4[064];	/* Reserved */
	int ept_tb[2];		/* Time Base */
	int ept_pac[2];		/* Performance Analysis Count */
	int ept_icii;		/* Interval Counter Interrupt Instruction */
	int ept_pad5[023];	/* Reserved */
	int ept_section[040];	/* Section pointers */
	int ept_pad6[0200];	/* Reserved */
};

#define	PG_IMM		0100000000000	/* Immediate access */
#define	PG_SH		0200000000000	/* Shared access */
#define	PG_IND		0300000000000	/* Indirect access */
#define	PG_PUBLIC	0040000000000	/* Public access */
#define	PG_WRITE	0020000000000	/* Write access */
#define	PG_CACHE	0004000000000	/* Cachable access */

struct	md_coredump {
	int dummy;
};

#ifdef _KERNEL
extern struct ept *ept;
#endif
#endif /* _PDP10_PCB_H_ */

