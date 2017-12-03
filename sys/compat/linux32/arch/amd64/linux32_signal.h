/*	$NetBSD: linux32_signal.h,v 1.3.10.1 2017/12/03 11:36:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _AMD64_LINUX32_SIGNAL_H_
#define _AMD64_LINUX32_SIGNAL_H_

#define native_to_linux32_signo native_to_linux_signo
#define linux32_to_native_signo linux_to_native_signo

typedef	netbsd32_pointer_t	linux32_handlerp_t;
typedef	netbsd32_pointer_t	linux32_restorer_t;
typedef netbsd32_pointer_t 	linux32_siginfop_t;
typedef netbsd32_pointer_t 	linux32_ucontextp_t;
typedef netbsd32_pointer_t 	linux32_sigcontextp_t;
typedef netbsd32_pointer_t 	linux32_fpstatep_t;
typedef u_int32_t		linux32_old_sigset_t;

#define LINUX32__NSIG             64
#define LINUX32__NSIG_BPW         32
#define LINUX32__NSIG_WORDS       (LINUX32__NSIG / LINUX32__NSIG_BPW)

#define LINUX32_SIG_BLOCK         0
#define LINUX32_SIG_UNBLOCK       1
#define LINUX32_SIG_SETMASK       2

#define LINUX32_SA_NOCLDSTOP      0x00000001
#define LINUX32_SA_NOCLDWAIT      0x00000002
#define LINUX32_SA_SIGINFO        0x00000004
#define LINUX32_SA_RESTORER       0x04000000
#define LINUX32_SA_ONSTACK        0x08000000
#define LINUX32_SA_RESTART        0x10000000
#define LINUX32_SA_INTERRUPT      0x20000000 
#define LINUX32_SA_NOMASK         0x40000000 
#define LINUX32_SA_ONESHOT        0x80000000
#define LINUX32_SA_ALLBITS        0xfc000007

#define LINUX32_SS_ONSTACK        1
#define LINUX32_SS_DISABLE        2

/* 
 * We only define the ones used in linux32_machdep.c, since they are
 * the same on i386 and amd64...
 */
#define LINUX32_SIGILL	LINUX_SIGILL
#define LINUX32_SIGFPE	LINUX_SIGFPE
#define LINUX32_SIGSEGV	LINUX_SIGSEGV
#define LINUX32_SIGBUS	LINUX_SIGBUS
#define LINUX32_SIGTRAP	LINUX_SIGTRAP
#define LINUX32_SIGCHLD	LINUX_SIGCHLD
#define LINUX32_SIGIO	LINUX_SIGIO
#define LINUX32_SIGALRM	LINUX_SIGALRM
#define LINUX32_SIGRTMIN	LINUX_SIGRTMIN

typedef struct {
	u_int32_t sig[LINUX32__NSIG_WORDS];
} linux32_sigset_t;

struct linux32_sigaction {
	linux32_handlerp_t	linux_sa_handler;
	u_int32_t		linux_sa_flags;
	linux32_restorer_t	linux_sa_restorer;
	linux32_sigset_t	linux_sa_mask;
};


struct linux32_sigaltstack { 
        netbsd32_voidp ss_sp;
        int ss_flags; 
        netbsd32_size_t ss_size;
};   

struct linux32_sigcontext {
        int     sc_gs;
        int     sc_fs;
        int     sc_es;
        int     sc_ds;
        int     sc_edi;
        int     sc_esi;
        int     sc_ebp;
        int     sc_esp;
        int     sc_ebx;
        int     sc_edx;
        int     sc_ecx;
        int     sc_eax;
        int     sc_trapno;
        int     sc_err;
        int     sc_eip;
        int     sc_cs;
        int     sc_eflags;
        int     sc_esp_at_signal;
        int     sc_ss;
        linux32_fpstatep_t sc_387;
        /* XXX check this */
        linux32_old_sigset_t sc_mask;
	int     sc_cr2;
};

struct linux32_libc_fpreg {
        unsigned short  significand[4];
        unsigned short  exponent;
};

struct linux32_libc_fpstate {
        u_int32_t cw;
        u_int32_t sw;
        u_int32_t tag;
        u_int32_t ipoff;
        u_int32_t cssel;
        u_int32_t dataoff;
        u_int32_t datasel;
        struct linux32_libc_fpreg _st[8];
        u_int32_t status;
};

struct linux32_ucontext {
        u_int32_t   uc_flags;
        linux32_ucontextp_t uc_link;
        struct linux32_sigaltstack uc_stack;
        struct linux32_sigcontext uc_mcontext;
        linux32_sigset_t uc_sigmask;
        struct linux32_libc_fpstate uc_fpregs_mem;
}; 

struct linux32_rt_sigframe {
        int     sf_sig;
        linux32_siginfop_t  sf_sip;
        linux32_ucontextp_t sf_ucp;
        struct  linux32_siginfo  sf_si;
        struct  linux32_ucontext sf_uc; 
        linux32_handlerp_t   sf_handler;
}; 

struct linux32_sigframe {
        int     sf_sig;
        struct  linux32_sigcontext sf_sc; 
        linux32_handlerp_t   sf_handler;
};

#endif /* _AMD64_LINUX32_SIGNAL_H_ */
