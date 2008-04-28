/*	$NetBSD: apmvar.h,v 1.25 2008/04/28 20:23:24 martin Exp $	*/
/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
#ifndef __I386_APM_H__
#define __I386_APM_H__

#include <dev/apm/apmbios.h>

#ifndef _LOCORE
#include <dev/apm/apmio.h>
#endif /* _LCORE */

/*
 * virtual & physical address of the trampoline
 * that we use: page 1.
 */
#define APM_BIOSTRAMP	PAGE_SIZE

#ifndef _LOCORE
/* filled in by apmcall */ 

struct apm_connect_info {
	u_int apm_code32_seg_base;	/* real-mode style segment selector */
	u_int apm_code16_seg_base;
	u_int apm_data_seg_base;
	u_int apm_entrypt;
	u_short	apm_segsel;		/* segment selector for APM */
	u_short _pad1;
	u_int apm_code32_seg_len;
	u_int apm_code16_seg_len;
	u_int apm_data_seg_len;
	u_int apm_detail;
};

#ifdef _KERNEL
extern struct apm_connect_info apminfo;	/* in locore */
extern int apmpresent;
int apmcall(int function, struct bioscallregs *regs);
void bioscall(int function, struct bioscallregs *regs);
int apm_set_powstate(void *, u_int, u_int);
void apminit(void);
int apm_busprobe(void);
#endif /* _KERNEL */
#endif /* _LOCORE */
#endif /* __i386_apm_h__ */
