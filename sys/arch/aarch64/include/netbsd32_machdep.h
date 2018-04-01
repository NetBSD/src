/*	$NetBSD: netbsd32_machdep.h,v 1.1 2018/04/01 04:35:03 ryo Exp $	*/

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

#include <sys/ucontext.h>
#include <compat/sys/ucontext.h>
#include <compat/sys/siginfo.h>

#define NETBSD32_POINTER_TYPE uint32_t
typedef	struct { NETBSD32_POINTER_TYPE i32; } netbsd32_pointer_t;

/* earm has 64bit aligned 64bit integers */
#define NETBSD32_INT64_ALIGN __attribute__((__aligned__(8)))

typedef netbsd32_pointer_t netbsd32_sigcontextp_t;

#define netbsd32_syscall_intern syscall_intern

struct netbsd32_sigcontext13 {
	int32_t		sc_onstack;		/* sigstack state to restore */
	int32_t		__sc_mask13;		/* signal mask to restore (old style) */

	uint32_t	sc_spsr;
	uint32_t	sc_r0;
	uint32_t	sc_r1;
	uint32_t	sc_r2;
	uint32_t	sc_r3;
	uint32_t	sc_r4;
	uint32_t	sc_r5;
	uint32_t	sc_r6;
	uint32_t	sc_r7;
	uint32_t	sc_r8;
	uint32_t	sc_r9;
	uint32_t	sc_r10;
	uint32_t	sc_r11;
	uint32_t	sc_r12;
	uint32_t	sc_usr_sp;
	uint32_t	sc_usr_lr;
	uint32_t	sc_svc_lr;
	uint32_t	sc_pc;
};

struct netbsd32_sigcontext {
	int32_t		sc_onstack;		/* sigstack state to restore */
	int32_t		__sc_mask13;		/* signal mask to restore (old style) */

	uint32_t	sc_spsr;
	uint32_t	sc_r0;
	uint32_t	sc_r1;
	uint32_t	sc_r2;
	uint32_t	sc_r3;
	uint32_t	sc_r4;
	uint32_t	sc_r5;
	uint32_t	sc_r6;
	uint32_t	sc_r7;
	uint32_t	sc_r8;
	uint32_t	sc_r9;
	uint32_t	sc_r10;
	uint32_t	sc_r11;
	uint32_t	sc_r12;
	uint32_t	sc_usr_sp;
	uint32_t	sc_usr_lr;
	uint32_t	sc_svc_lr;
	uint32_t	sc_pc;

	sigset_t	sc_mask;		/* signal mask to restore (new style) */
};

#define NETBSD32_MID_MACHINE MID_ARM

#endif /* _MACHINE_NETBSD32_H_ */
