/*	$NetBSD: cpus.h,v 1.11 2002/03/09 23:49:16 bjh21 Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpus.h
 *
 * cpu device header file
 *
 * Created      : 26/12/95
 */

#ifndef _ARM_CPUS_H_
#define _ARM_CPUS_H_

#define MAX_CPUS	1

#define CPU_MASTER	0

#define FPU_CLASS_NONE		0	/* no Floating point support */
#define FPU_CLASS_FPE		1	/* Floating point emulator installed */
#define FPU_CLASS_FPA		2	/* Floating point accelerator installed */
#define FPU_CLASS_FPU		3	/* Floating point unit installed */

#define FPU_TYPE_SP_FPE		1	/* Single precision FPE */
#define FPU_TYPE_ARMLTD_FPE	2	/* ARM Ltd FPE */
#define FPU_TYPE_FPA11		0x81	/* ID of FPA11 */

#ifndef _LOCORE

/* Define the structure used to describe a cpu */

typedef struct _cpu {
/* These are generic FPU variables */

	u_int	fpu_class;	/* The FPU class */
	u_int	fpu_type;	/* The FPU type */
	u_int	fpu_flags;	/* The FPU flags */
	char	fpu_model[256];	/* Text description of FPU */

} cpu_t;

#ifdef _KERNEL

/* Array of cpu structures, one per possible cpu */

extern cpu_t cpus[MAX_CPUS];

#endif	/* _KERNEL */
#endif	/* _LOCORE */

/* End of cpus.h */
#endif
