/*	$NetBSD: uvax.h,v 1.7.176.1 2017/08/28 17:51:54 skrll Exp $ */
/*
 * Copyright (c) 2002 Hugh Graham.
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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

#ifndef _VAX_UVAX_H_
#define _VAX_UVAX_H_

/*
 * Generic definitions common on all MicroVAXen clock chip.
 */
#define uVAX_CLKVRT	0200
#define uVAX_CLKUIP	0200
#define uVAX_CLKRATE	040
#define uVAX_CLKENABLE	06
#define uVAX_CLKSET	0206

/* cpmbx bits  */
#define uVAX_CLKHLTACT	03

/* halt action values */
#define uVAX_CLKRESTRT	01
#define uVAX_CLKREBOOT	02
#define uVAX_CLKHALT	03

/* in progress flags */
#define uVAX_CLKBOOT	04
#define uVAX_CLKRSTRT	010
#define uVAX_CLKLANG	0360

/*
 * Miscellaneous registers common on most VAXststions.
 */
struct vs_cpu {
	u_long	vc_hltcod;	/* 00 - Halt Code Register */
	u_long	vc_410mser;	/* 04 - VS2K */
	u_long	vc_410cear;	/* 08 - VS2K */
	u_char	vc_intmsk;	/* 0c - Interrupt mask register */
	u_char	vc_vdcorg;	/* 0d - Mono display origin */
	u_char	vc_vdcsel;	/* 0e - Video interrupt select */
	u_char	vc_intreq;	/* 0f - Interrupt request register */
#define vc_intclr vc_intreq
	u_short vc_diagdsp;	/* 10 - Diagnostic display register */
	u_short pad4;		/* 12 */
	u_long	vc_parctl;	/* 14 - Parity Control Register */
#define vc_bwf0 vc_parctl
	u_short pad5;		/* 16 */
	u_short pad6;		/* 18 */
	u_short vc_diagtimu;	/* 1a - usecond timer KA46 */
	u_short vc_diagtme;	/* 1c - Diagnostic time register */
#define vc_diagtimm vc_diagtme	/* msecond time KA46 */
};
#define PARCTL_DMA	0x1000000
#define PARCTL_CPEN	2
#define PARCTL_DPEN	1


/*
 * Console Mailbox layout common to several models.
 */

struct cpmbx {
	unsigned int mbox_halt:2;	/* mailbox halt action */
	unsigned int mbox_bip:1;	/* bootstrap in progress */
	unsigned int mbox_rip:1;	/* restart in progress */
	unsigned int mbox_lang:4;	/* language info */
	unsigned int terminal:8;	/* terminal info */
	unsigned int keyboard:8;	/* keyboard info */
	unsigned int user_four:4;	/* unknown */
	unsigned int user_halt:3;	/* user halt action */
	unsigned int user_one:1;	/* unknown */
};

extern struct cpmbx *cpmbx;

void   generic_halt(void);
void   generic_reboot(int);

#define	MHALT_RESTART_REBOOT	0
#define	MHALT_RESTART		1
#define	MHALT_REBOOT		2
#define	MHALT_HALT		3

#define	UHALT_DEFAULT		0
#define	UHALT_RESTART		1
#define	UHALT_REBOOT		2
#define	UHALT_HALT		3
#define	UHALT_RESTART_REBOOT	4

#endif
