/*	$NetBSD: apmvar.h,v 1.17 2003/04/02 07:35:59 thorpej Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <dev/apm/apmio.h>

/*
 * virtual & physical address of the trampoline
 * that we use: page 1.
 */
#define APM_BIOSTRAMP	PAGE_SIZE

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

struct apm_attach_args {
	char *aaa_busname;
};

#ifdef _KERNEL
extern struct apm_connect_info apminfo;	/* in locore */
extern int apmpresent;
int apmcall __P((int function, struct bioscallregs *regs));
void bioscall __P((int function, struct bioscallregs *regs));
void apm_cpu_busy __P((void));
void apm_cpu_idle __P((void));
void apminit __P((void));
int apm_set_powstate __P((u_int devid, u_int powstate));
int apm_busprobe __P((void));
#endif /* _KERNEL */
#endif /* __i386_apm_h__ */
