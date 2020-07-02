/*	$NetBSD: netbsd32_machdep.h,v 1.4 2020/07/02 13:04:47 rin Exp $	*/

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

#include <sys/ucontext.h>
#include <compat/sys/ucontext.h>
#include <compat/sys/siginfo.h>

/*
 * arm ptrace constants
 * Please keep in sync with sys/arch/arm/include/ptrace.h.
 */
#define PT32_STEP	(PT_FIRSTMACH + 0) /* Not implemented */
#define PT32_GETREGS	(PT_FIRSTMACH + 1)
#define PT32_SETREGS	(PT_FIRSTMACH + 2)
#define PT32_GETFPREGS	(PT_FIRSTMACH + 5)
#define PT32_SETFPREGS	(PT_FIRSTMACH + 6)
#define PT32_SETSTEP	(PT_FIRSTMACH + 7) /* Not implemented */
#define PT32_CLEARSTEP	(PT_FIRSTMACH + 8) /* Not implemented */

#define NETBSD32_POINTER_TYPE uint32_t
typedef	struct { NETBSD32_POINTER_TYPE i32; } netbsd32_pointer_t;

/* earm has 64bit aligned 64bit integers */
#define NETBSD32_INT64_ALIGN __attribute__((__aligned__(8)))

typedef netbsd32_pointer_t netbsd32_sigcontextp_t;

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

struct netbsd32_sigframe_siginfo {
	siginfo32_t sf_si;
	ucontext32_t sf_uc;
};

struct reg32 {
	uint32_t r[13];
	uint32_t r_sp;
	uint32_t r_lr;
	uint32_t r_pc;
	uint32_t r_cpsr;
};

struct vfpreg32 {
	uint32_t vfp_fpexc;
	uint32_t vfp_fpscr;
	uint32_t vfp_fpinst;
	uint32_t vfp_fpinst2;
	uint64_t vfp_regs[33];	/* In case we need fstmx format. */
};

struct fpreg32 {
	struct vfpreg32 fpr_vfp;
};

/* same as cpustate in arm/arm/core_machdep.c  */
struct netbsd32_cpustate {
	struct reg32 regs;
	struct fpreg32 fpregs;
};

/* compat netbsd/arm sysarch(2) */
#define ARM_SYNC_ICACHE		0
#define ARM_DRAIN_WRITEBUF	1
#define ARM_VFP_FPSCR		2
#define ARM_FPU_USED		3

struct netbsd32_arm_sync_icache_args {
	uint32_t addr;		/* Virtual start address */
	uint32_t len;		/* Region size */
};

/* Support varying ABI names for netbsd32 */
#define PROC_MACHINE_ARCH32(P)	((P)->p_md.md_march32)

/* Translate ptrace() PT_* request from 32-bit userland to kernel. */
int netbsd32_ptrace_translate_request(int);

int netbsd32_process_read_regs(struct lwp *, struct reg32 *);
int netbsd32_process_read_fpregs(struct lwp *, struct fpreg32 *, size_t *);

int netbsd32_process_write_regs(struct lwp *, const struct reg32 *);
int netbsd32_process_write_fpregs(struct lwp *, const struct fpreg32 *, size_t);

#endif /* _MACHINE_NETBSD32_H_ */
