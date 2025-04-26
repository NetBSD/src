/*	$NetBSD: intr.h,v 1.20 2025/04/26 04:39:09 tsutsui Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#include <mips/intr.h>

#ifdef _KERNEL
#ifdef __INTR_PRIVATE
#include <sys/evcnt.h>
#include <mips/cpuregs.h>

struct mipsco_intrhand {
	LIST_ENTRY(mipsco_intrhand) ih_q;
	int	(*ih_fun)(void *);
	void	 *ih_arg;
	struct	mipsco_intr *ih_intrhead;
	int	ih_pending;
};

struct mipsco_intr {
	LIST_HEAD(,mipsco_intrhand) intr_q;
	struct	evcnt ih_evcnt;
	unsigned long intr_siq;
};

extern struct mipsco_intrhand intrtab[];
#define	CALL_INTR(lev)	((*intrtab[lev].ih_fun)(intrtab[lev].ih_arg))

#define MAX_INTR_COOKIES 16

#endif /* __INTR_PRIVATE */

#define SYS_INTR_LEVEL0	0
#define SYS_INTR_LEVEL1	1
#define SYS_INTR_LEVEL2	2
#define SYS_INTR_LEVEL3	3
#define SYS_INTR_LEVEL4	4
#define SYS_INTR_LEVEL5	5
#define SYS_INTR_SCSI	6
#define SYS_INTR_TIMER	7
#define SYS_INTR_ETHER	8
#define SYS_INTR_SCC0	9
#define SYS_INTR_FDC	10
#define SYS_INTR_ATBUS	11

#endif /* _KERNEL */
#endif /* _MACHINE_INTR_H_ */
