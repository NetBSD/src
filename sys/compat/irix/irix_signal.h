/*	$NetBSD: irix_signal.h,v 1.6 2002/04/06 18:08:31 manu Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _IRIX_SIGNAL_H_
#define _IRIX_SIGNAL_H_

#include <sys/types.h>
#include <sys/signal.h>

#include <machine/svr4_machdep.h>

#include <compat/irix/irix_types.h>

/* From IRIX's <sys/signal.h> */

#define IRIX_SIG_SETMASK32 256

typedef struct irix_sigcontext {
	__uint32_t	isc_regmask;
	__uint32_t	isc_status;
	__uint64_t	isc_pc;
	__uint64_t	isc_regs[32];
	__uint64_t	isc_fpregs[32];
	__uint32_t	isc_ownedfp;
	__uint32_t	isc_fpc_csr;
	__uint32_t	isc_fpc_eir;
	__uint32_t	isc_ssflags;
	__uint64_t	isc_mdhi;
	__uint64_t	isc_mdlo;
	__uint64_t	isc_cause;
	__uint64_t	isc_badvaddr;
	__uint64_t	isc_triggersave;
	irix_sigset_t	isc_sigset;
	__uint64_t	isc_fp_rounded_result;
	__uint64_t	isc_pad[31];
} irix_sigcontext_t;

#define IRIX_SS_ONSTACK	0x00000001
#define IRIX_SS_DISABLE 0x00000002

/* From IRIX sigreturn(2) man page */
struct irix_sigreturna {
	irix_sigcontext_t *scp;
	void *ucp;
	int signo;
};

/* From IRIX's <sys/ucontext.h> */
#define IRIX_UC_SIGMASK	001
#define IRIX_UC_STACK	002
#define IRIX_UC_CPU	004
#define IRIX_UC_MAU	010
#define IRIX_UC_MCONTEXT (IRIX_UC_CPU|IRIX_UC_MAU)
#define IRIX_UC_ALL	(IRIX_UC_SIGMASK|IRIX_UC_STACK|IRIX_UC_MCONTEXT)

#if 1 /* _MIPS_SZLONG == 32 */
typedef struct irix__sigaltstack {
	void    	*ss_sp;
	irix_size_t  	ss_size;
	int     	ss_flags;
} irix_stack_t;
#endif
#if 0 /* _MIPS_SZLONG == 64 */
typedef struct irix__sigaltstack {
	void    	*ss_sp;
	__uint32_t	ss_size;
	int     	ss_flags;
} irix_stack_t;
#endif

typedef struct irix_ucontext {
	unsigned long		iuc_flags;
	struct irix_ucontext	*iuc_link;
	irix_sigset_t		iuc_sigmask;
	irix_stack_t		iuc_stack;
	svr4_mcontext_t		iuc_mcontext;
	long			iuc_filler[47];
	int			iuc_triggersave;
} irix_ucontext_t;

/* From IRIX's <sys/siginfo.h> */
#define IRIX_SI_MAXSZ	128
#define IRIX_SI_PAD	((IRIX_SI_MAXSZ / sizeof(__int32_t)) - 3)

/* From IRIX's <sys/ksignal.h> */
typedef union irix_irix5_sigval {
	irix_app32_int_t	sigbval_int;
	irix_app32_ptr_t	sival_ptr;
} irix_irix5_sigval_t;

typedef struct irix_irix5_siginfo {
	irix_app32_int_t 	si_signo;
	irix_app32_int_t	si_code;
	irix_app32_int_t	si_errno;
	union {
		irix_app32_int_t	si_pad[IRIX_SI_PAD];
		struct {
			irix_irix5_pid_t	__pid;
			union {
				struct {
					irix_irix5_uid_t	__uid;
				} __kill;
				struct {
					irix_irix5_clock_t	__utime;
					irix_app32_int_t	__status;
					irix_irix5_clock_t	__stime;
					irix_app32_int_t	__swap;
				} __cld;
			} __pdata;
		} __proc;
		struct {
			irix_app32_ptr_t	__addr;
		} __fault;
		struct {
			irix_app32_int_t	__fd;
			irix_app32_long_t	__band;
		} __file;
		union irix_irix5_sigval	__value;
	} __data;
} irix_irix5_siginfo_t;
#define si_pid		__data.__proc.__pid
#define si_stime	__data.__proc.__pdata.__cld.__stime
#define si_utime	__data.__proc.__pdata.__cld.__utime
#define si_status	__data.__proc.__pdata.__cld.__status
#define si_addr		__data.__fault.__addr;
#define si_trap


#ifdef _KERNEL
__BEGIN_DECLS
void native_to_irix_sigset __P((const sigset_t *, irix_sigset_t *));
void irix_to_native_sigset __P((const irix_sigset_t *, sigset_t *));


void irix_sendsig __P((sig_t, int, sigset_t *, u_long));
__END_DECLS
#endif /* _KERNEL */


#endif /* _IRIX_SIGNAL_H_ */
