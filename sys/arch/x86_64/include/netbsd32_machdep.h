/*	$NetBSD: netbsd32_machdep.h,v 1.2 2002/06/25 01:24:49 thorpej Exp $	*/

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

typedef u_int32_t netbsd32_sigcontextp_t;

struct netbsd32_sigcontext13 {
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	/* XXX */
	int	sc_eip;
	int	sc_cs;
	int	sc_eflags;
	int	sc_esp;
	int	sc_ss;

	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */

	int	sc_trapno;		/* XXX should be above */
	int	sc_err;
};

struct netbsd32_sigcontext {
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	/* XXX */
	int	sc_eip;
	int	sc_cs;
	int	sc_eflags;
	int	sc_esp;
	int	sc_ss;

	int	sc_onstack;		/* sigstack state to restore */
	int	__sc_mask13;		/* signal mask to restore (old style) */

	int	sc_trapno;		/* XXX should be above */
	int	sc_err;

	sigset_t sc_mask;		/* signal mask to restore (new style) */
};

#define sc_sp sc_esp
#define sc_fp sc_ebp
#define sc_pc sc_eip
#define sc_ps sc_eflags

struct netbsd32_sigframe {
	uint32_t sf_ra;
	int	sf_signum;
	int	sf_code;
	uint32_t sf_scp;
	struct	netbsd32_sigcontext sf_sc;
};

struct reg32 {
	int	r_eax;
	int	r_ecx;
	int	r_edx;
	int	r_ebx;
	int	r_esp;
	int	r_ebp;
	int	r_esi;
	int	r_edi;
	int	r_eip;
	int	r_eflags;
	int	r_cs;
	int	r_ss;
	int	r_ds;
	int	r_es;
	int	r_fs;
	int	r_gs;
};

struct fpreg32 {
	char	__data[108];
};


struct exec_package;
void netbsd32_setregs(struct proc *p, struct exec_package *pack, u_long stack);
int netbsd32_sigreturn(struct proc *p, void *v, register_t *retval);
void netbsd32_sendsig(sig_t catcher, int sig, sigset_t *mask, u_long code);

extern char netbsd32_sigcode[], netbsd32_esigcode[];

#endif /* _MACHINE_NETBSD32_H_ */
