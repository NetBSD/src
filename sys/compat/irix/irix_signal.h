/*	$NetBSD: irix_signal.h,v 1.16.46.1 2008/01/09 01:50:50 matt Exp $ */

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

/* From IRIX's <sys/ucontext.h> */
#define IRIX_UC_SIGMASK	001
#define IRIX_UC_STACK	002
#define IRIX_UC_CPU	004
#define IRIX_UC_MAU	010
#define IRIX_UC_MCONTEXT (IRIX_UC_CPU|IRIX_UC_MAU)
#define IRIX_UC_ALL	(IRIX_UC_SIGMASK|IRIX_UC_STACK|IRIX_UC_MCONTEXT)

#define IRIX_CTX_MDLO	32
#define IRIX_CTX_MDHI	33
#define IRIX_CTX_CAUSE	34
#define IRIX_CTX_EPC	35

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
#define IRIX_ILL_ILLOPC	1
#define IRIX_ILL_ILLOPN	2
#define IRIX_ILL_ILLADR	3
#define IRIX_ILL_ILLTRP	4
#define IRIX_ILL_PRVOPC	5
#define IRIX_ILL_PRVREG	6
#define IRIX_ILL_COPROC	7
#define IRIX_ILL_BADSTK	8

#define IRIX_FPE_INTDIV	1
#define IRIX_FPE_INTOVF	2
#define IRIX_FPE_FLTDIV	3
#define IRIX_FPE_FLTOVF	4
#define IRIX_FPE_FLTUND	5
#define IRIX_FPE_FLTRES	6
#define IRIX_FPE_FLTINV	7
#define IRIX_FPE_FLTSUB	8

#define IRIX_SEGV_MAPERR	1
#define IRIX_SEGV_ACCERR	2

#define IRIX_BUS_ADRALN	1
#define IRIX_BUS_ADRERR	2
#define IRIX_BUS_OBJERR	3

#define IRIX_TRAP_BRKPT	1
#define IRIX_TRAP_TRACE	2

#define IRIX_CLD_EXITED	1
#define IRIX_CLD_KILLED	2
#define IRIX_CLD_DUMPED	3
#define IRIX_CLD_TRAPPED	4
#define IRIX_CLD_STOPPED	5
#define IRIX_CLD_CONTINUED	6

#define IRIX_POLL_IN	1
#define IRIX_POLL_OUT	2
#define IRIX_POLL_MSG	3
#define IRIX_POLL_ERR	4
#define IRIX_POLL_PRI	5
#define IRIX_POLL_HUP	6

#define IRIX_UME_ECCERR	1

/* From IRIX's <sys/fault.h> */
#define IRIX_FLTILL     1
#define IRIX_FLTPRIV    2
#define IRIX_FLTBPT     3
#define IRIX_FLTTRACE   4
#define IRIX_FLTACCESS  5
#define IRIX_FLTBOUNDS  6
#define IRIX_FLTIOVF    7
#define IRIX_FLTIZDIV   8
#define IRIX_FLTFPE     9
#define IRIX_FLTSTACK   10
#define IRIX_FLTPAGE    11
#define IRIX_FLTPCINVAL 12
#define IRIX_FLTWATCH   13
#define IRIX_FLTKWATCH  14
#define IRIX_FLTSCWATCH 15


#define IRIX_SI_MAXSZ	128
#define IRIX_SI_PAD	((IRIX_SI_MAXSZ / sizeof(__int32_t)) - 3)

/* From IRIX's <sys/ksignal.h> */
typedef union irix_irix5_sigval {
	irix_app32_int_t	sigbval_int;
	irix_app32_ptr_t	sival_ptr;
} irix_irix5_sigval_t;

typedef struct irix_irix5_siginfo {
	irix_app32_int_t 	isi_signo;
	irix_app32_int_t	isi_code;
	irix_app32_int_t	isi_errno;
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

#define isi_pid		__data.__proc.__pid
#define isi_stime	__data.__proc.__pdata.__cld.__stime
#define isi_utime	__data.__proc.__pdata.__cld.__utime
#define isi_status	__data.__proc.__pdata.__cld.__status
#define isi_addr	__data.__fault.__addr
#define isi_trap

/*
 * This is the signal frame, as seen by the signal handler. The
 * kernel only sets up isf_ctx, the signal trampoline does the
 * other fields.
 */
struct irix_sigframe {
	int isf_pad1[7];
	int *isf_uep;	/* Pointer to errno in userspace */
	int isf_errno;
	int isf_signo;
	struct irix_sigcontext *isf_scp;
	struct irix_ucontext *isf_ucp;
	union {
		struct irix_sigcontext isc;
		struct irix_sigcontext_siginfo {
			struct irix_ucontext	iuc;
			struct irix_irix5_siginfo 	iis;
		} iss;
	} isf_ctx;
};


#ifdef _KERNEL
__BEGIN_DECLS
void native_to_irix_sigset(const sigset_t *, irix_sigset_t *);
void irix_to_native_sigset(const irix_sigset_t *, sigset_t *);


void irix_sendsig(const ksiginfo_t *, const sigset_t *);
__END_DECLS
#endif /* _KERNEL */


#endif /* _IRIX_SIGNAL_H_ */
