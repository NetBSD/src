/*	$NetBSD: frame.h,v 1.25.2.1 2014/08/20 00:03:19 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_POWERPC_FRAME_H_
#define	_POWERPC_FRAME_H_

#include <machine/types.h>

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif
#endif

/*
 * We have to save all registers on every trap, because
 *	1. user could attach this process every time
 *	2. we must be able to restore all user registers in case of fork
 * Actually, we do not save the fp registers on trap, since
 * these are not used by the kernel. They are saved only when switching
 * between processes using the FPU.
 *
 * Change ordering to cluster together these register_t's.		XXX
 */
struct reg_sans_pc {
	register_t r_fixreg[32];
	register_t r_lr;
	uint32_t r_cr;
	uint32_t r_xer;
	register_t r_ctr;
};

#ifdef _LP64
struct reg_sans_pc32 {
	register32_t r_fixreg[32];
	register32_t r_lr;
	uint32_t r_cr;
	uint32_t r_xer;
	register32_t r_ctr;
};
#endif

struct utrapframe {
	register_t fixreg[32];
	register_t lr;
	int cr;
	int xer;
	register_t ctr;
	register_t srr0;
	register_t srr1;
	int vrsave;
	int mq;
	int spare;
};

struct clockframe {
	register_t cf_srr0;
	register_t cf_srr1;
	int cf_idepth;
};

#ifdef _LP64
struct clockframe32 {
	register32_t cf_srr0;
	register32_t cf_srr1;
	int cf_idepth;
};
#endif

struct trapframe {
	struct reg_sans_pc tf_ureg;
	struct clockframe tf_cf;
	uint32_t tf_exc;
#if defined(PPC_OEA) || defined(PPC_OEA64) || defined(PPC_OEA64_BRIDGE)
	register_t tf_dar;
	register_t tf_pad0[2];
	uint32_t tf_dsisr;
	uint32_t tf_vrsave;
	uint32_t tf_mq;
	uint32_t tf_pad1[1];
#endif
#if defined(PPC_BOOKE) || defined(PPC_IBM4XX)
	register_t tf_dear;
	register_t tf_mcar;
	register_t tf_sprg1;
	uint32_t tf_esr;
	uint32_t tf_mcsr;
	uint32_t tf_pid;
	uint32_t tf_spefscr;
#endif
};

#ifdef _LP64
struct trapframe32 {
	struct reg_sans_pc32 tf_ureg;
	struct clockframe32 tf_cf;
	uint32_t tf_exc;
#if defined(PPC_OEA) || defined(PPC_OEA64) || defined(PPC_OEA64_BRIDGE)
	register32_t tf_dar;
	register32_t tf_pad0[2];
	uint32_t tf_dsisr;
	uint32_t tf_vrsave;
	uint32_t tf_mq;
	uint32_t tf_pad1[1];
#endif
#if defined(PPC_BOOKE) || defined(PPC_IBM4XX)
	register32_t tf_dear;
	register32_t tf_mcar;
	register32_t tf_sprg1;
	uint32_t tf_esr;
	uint32_t tf_mcsr;
	uint32_t tf_pid;
	uint32_t tf_spefscr;
#endif
};
#endif /* _LP64 */
#define tf_fixreg	tf_ureg.r_fixreg
#define tf_lr		tf_ureg.r_lr
#define tf_cr		tf_ureg.r_cr
#define tf_xer		tf_ureg.r_xer
#define tf_ctr		tf_ureg.r_ctr
#define tf_srr0		tf_cf.cf_srr0
#define tf_srr1		tf_cf.cf_srr1
#define tf_idepth	tf_cf.cf_idepth

struct ktrapframe {
	register_t ktf_sp;
	register_t ktf_lr;
	struct trapframe ktf_tf;
	register_t ktf_cframe_lr;	/* for DDB */
};

#if defined(_KERNEL) || defined(_LKM)
#ifdef _LP64
struct utrapframe32 {
	register32_t fixreg[32];
	register32_t lr;
	int cr;
	int xer;
	register32_t ctr;
	register32_t srr0;
	register32_t srr1;
	int vrsave;
	int mq;
	int spare;
};
#endif
#endif /* _KERNEL || _LKM */

/*
 * This is to ensure alignment of the stackpointer
 */
#define	FRAMELEN	roundup(sizeof(struct ktrapframe), CALLFRAMELEN)
#define	ktrapframe(l)	((struct ktrapframe *)(uvm_lwp_getuarea(l) + USPACE - CALLFRAMELEN - FRAMELEN))
#define	trapframe(l)	(&(ktrapframe(l)->ktf_tf))

#define	SFRAMELEN	roundup(sizeof(struct switchframe), CALLFRAMELEN)
struct switchframe {
	register_t sf_sp;
	register_t sf_lr;
	register_t sf_user_sr;		/* VSID on IBM4XX */
	register_t sf_cr;		/* why?  CR is volatile. */
	register_t sf_fixreg2;
	register_t sf_fixreg[19];	/* R13-R31 */
};

/*
 * Call frame for PowerPC used during fork.
 */
#define	CALLFRAMELEN	sizeof(struct callframe)
struct callframe {
	register_t cf_sp;
	register_t cf_lr;
	register_t cf_r30;
	register_t cf_r31;
};

#endif	/* _POWERPC_FRAME_H_ */
