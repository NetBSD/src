/* $NetBSD: machdep.c,v 1.46 2011/12/27 14:55:31 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Reinoud Zandijk <reinoud@netbsd.org>
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * Note that this machdep.c uses the `dummy' mcontext_t defined for usermode.
 * This is basicly a blob of PAGE_SIZE big. We might want to switch over to
 * non-generic mcontext_t's one day, but will this break non-NetBSD hosts?
 */


#include "opt_memsize.h"
#include "opt_sdl.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.46 2011/12/27 14:55:31 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/boot_flag.h>
#include <sys/ucontext.h>
#include <sys/utsname.h>
#include <machine/pcb.h>
#include <machine/psl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <dev/mm.h>
#include <machine/machdep.h>
#include <machine/thunk.h>

char machine[_SYS_NMLN] = "";
char machine_arch[_SYS_NMLN] = "";
char module_machine_usermode[_SYS_NMLN] = "";

struct vm_map *phys_map = NULL;

static char **saved_argv;
char *usermode_root_image_path = NULL;
static char usermode_tap_devicebuf[PATH_MAX] = "";
char *usermode_tap_device = NULL;
char *usermode_tap_eaddr = NULL;
static char usermode_audio_devicebuf[PATH_MAX] = "";
char *usermode_audio_device = NULL;

void	main(int argc, char *argv[]);
void	usermode_reboot(void);

static void
usage(const char *pn)
{
	printf("usage: %s [-acdqsvxz]"
	    " [tap=<dev>,<eaddr>]"
	    " [audio=<dev>]"
	    " [<fsimg>]\n",
	    pn);
	printf("       (ex. \"%s"
	    " tap=tap0,00:00:be:ef:ca:fe"
	    " audio=audio0"
	    " root.fs\")\n", pn);
}

void
main(int argc, char *argv[])
{
#if defined(SDL)
	extern int genfb_thunkbus_cnattach(void);
#endif
	extern void ttycons_consinit(void);
	extern void pmap_bootstrap(void);
	extern void kernmain(void);
	int i, j, r, tmpopt = 0;

	saved_argv = argv;

	/* Get machine and machine_arch from host */
	thunk_getmachine(machine, sizeof(machine),
	    machine_arch, sizeof(machine_arch));
	/* Override module_machine to be ${machine}usermode */
	snprintf(module_machine_usermode, sizeof(module_machine_usermode),
	    "%susermode", machine);

#if defined(SDL)
	if (genfb_thunkbus_cnattach() == 0)
#endif
		ttycons_consinit();

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (strncmp(argv[i], "tap=", strlen("tap=")) == 0) {
				char *tap = argv[i] + strlen("tap=");
				char *mac = strchr(tap, ',');
				if (mac == NULL) {
					printf("bad tap= format\n");
					return;
				}
				*mac++ = '\0';
				if (*tap != '/')
					snprintf(usermode_tap_devicebuf,
					    sizeof(usermode_tap_devicebuf),
					    "/dev/%s", tap);
				else
					snprintf(usermode_tap_devicebuf,
					    sizeof(usermode_tap_devicebuf),
					    "%s", tap);
				usermode_tap_device = usermode_tap_devicebuf;
				usermode_tap_eaddr = mac;
			} else if (strncmp(argv[i], "audio=",
			    strlen("audio=")) == 0) {
				char *audio = argv[i] + strlen("audio=");
				if (*audio != '/')
					snprintf(usermode_audio_devicebuf,
					    sizeof(usermode_audio_devicebuf),
					    "/dev/%s", audio);
				else
					snprintf(usermode_audio_devicebuf,
					    sizeof(usermode_audio_devicebuf),
					    "%s", audio);
				usermode_audio_device =
				    usermode_audio_devicebuf;
			} else {
				usermode_root_image_path = argv[i];
			}
			continue;
		}
		for (j = 1; argv[i][j] != '\0'; j++) {
			r = 0;
			BOOT_FLAG(argv[i][j], r);
			if (r == 0) {
				printf("-%c: unknown flag\n", argv[i][j]);
				usage(argv[0]);
				return;
			}
			tmpopt |= r;
		}
	}
	boothowto = tmpopt;

	uvm_setpagesize();
	uvmexp.ncolors = 2;

	pmap_bootstrap();

	splinit();
	splraise(IPL_HIGH);

	kernmain();
}

void
usermode_reboot(void)
{
	struct thunk_itimerval itimer;

	/* make sure the timer is turned off */
	memset(&itimer, 0, sizeof(itimer));
	thunk_setitimer(ITIMER_REAL, &itimer, NULL);

	if (thunk_execv(saved_argv[0], saved_argv) == -1)
		thunk_abort();
	/* NOTREACHED */
}

void
setstatclockrate(int arg)
{
}

void
consinit(void)
{
	printf("NetBSD/usermode startup\n");
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prog)
{
	return 0;
}


#ifdef __i386__

#if 0
static void dump_regs(register_t *reg);;

static void
dump_regs(register_t *reg)
{
	int i;

	/* register dump before call */
	const char *name[] = {"GS", "FS", "ES", "DS", "EDI", "ESI", "EBP", "ESP",
		"EBX", "EDX", "ECX", "EAX", "TRAPNO", "ERR", "EIP", "CS", "EFL",
		"UESP", "SS"};

	for (i =0; i < 19; i++)
		printf("reg[%02d] (%6s) = %"PRIx32"\n", i, name[i], (uint32_t) reg[i]);
}
#endif


/* from sys/arch/i386/include/frame.h : KEEP IN SYNC */

/*
 * New-style signal frame
 */
struct sigframe_siginfo {
	int		sf_ra;		/* return address for handler */
	int		sf_signum;	/* "signum" argument for handler */
	siginfo_t	*sf_sip;	/* "sip" argument for handler */
	ucontext_t	*sf_ucp;	/* "ucp" argument for handler */
	siginfo_t	sf_si;		/* actual saved siginfo */
	ucontext_t	sf_uc;		/* actual saved ucontext */
};


/*
 * mcontext extensions to handle signal delivery.
 */
#define _UC_SETSTACK	0x00010000
#define _UC_CLRSTACK	0x00020000
#define _UC_VM		0x00040000
#define	_UC_TLSBASE	0x00080000


void
sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pcb *pcb = lwp_getpcb(l);
	struct sigacts *ps = p->p_sigacts;
	struct sigframe_siginfo *fp, frame;
	int sig = ksi->ksi_signo;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	register_t *reg = (register_t *) &ucp->uc_mcontext;
	int onstack, error;

	KASSERT(mutex_owned(p->p_lock));

#if 0
	printf("%s: ", __func__);
	printf("flags %d, ", (int) ksi->ksi_flags);
	printf("to lwp %d, signo %d, code %d, errno %d\n",
		(int) ksi->ksi_lid,
		ksi->ksi_signo,
		ksi->ksi_code,
		ksi->ksi_errno);
#endif

	/* do we need to jump onto the signal stack? */
	onstack = (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
	    && (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	fp = (void *) reg[17];	/* ESP */
	if (onstack)
		fp = (void *)
			((char *) l->l_sigstk.ss_sp + l->l_sigstk.ss_size);

	fp--;

	/* set up stack frame */
	frame.sf_ra = (int)ps->sa_sigdesc[sig].sd_tramp;
	frame.sf_signum = sig;
	frame.sf_sip = &fp->sf_si;
	frame.sf_ucp = &fp->sf_uc;
	frame.sf_si._info = ksi->ksi_info;

	/* copy our userret context into sf_uc */
	memcpy(&frame.sf_uc, ucp, sizeof(ucontext_t));
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = l->l_ctxlink; /* XXX ??? */
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));

	sendsig_reset(l, sig);

	/* copyout our frame to the stackframe */
	mutex_exit(p->p_lock);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	/* set catcher and the new stack pointer */
	reg[17] = (register_t) fp;	/* ESP */
	reg[14] = (register_t) catcher;	/* EIP */

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	uint *reg, i;

#ifdef DEBUG_EXEC
	printf("setregs called: lwp %p, exec package %p, stack %p\n",
		l, pack, (void *) stack);
	printf("current stat of pcb %p\n", pcb);
	printf("\tpcb->pcb_ucp.uc_stack.ss_sp   = %p\n",
		pcb->pcb_ucp.uc_stack.ss_sp);
	printf("\tpcb->pcb_ucp.uc_stack.ss_size = %d\n",
		(int) pcb->pcb_ucp.uc_stack.ss_size);
	printf("\tpcb->pcb_userret_ucp.uc_stack.ss_sp   = %p\n",
		pcb->pcb_userret_ucp.uc_stack.ss_sp);
	printf("\tpcb->pcb_userret_ucp.uc_stack.ss_size = %d\n",
		(int) pcb->pcb_userret_ucp.uc_stack.ss_size);
#endif

	reg = (int *) &ucp->uc_mcontext;
	for (i = 4; i < 11; i++)
		reg[i] = 0;

	ucp->uc_stack.ss_sp = (void *) (stack-4);	/* to prevent clearing */
	ucp->uc_stack.ss_size = 0; //pack->ep_ssize;
	thunk_makecontext(ucp, (void *) pack->ep_entry, 0, NULL, NULL, NULL);

	/* patch up */
	reg[ 8] = l->l_proc->p_psstrp;	/* _REG_EBX */
	reg[17] = (stack);		/* _REG_UESP */

	//dump_regs(reg);

#ifdef DEBUG_EXEC
	printf("updated pcb %p\n", pcb);
	printf("\tpcb->pcb_ucp.uc_stack.ss_sp   = %p\n",
		pcb->pcb_ucp.uc_stack.ss_sp);
	printf("\tpcb->pcb_ucp.uc_stack.ss_size = %d\n",
		(int) pcb->pcb_ucp.uc_stack.ss_size);
	printf("\tpcb->pcb_userret_ucp.uc_stack.ss_sp   = %p\n",
		pcb->pcb_userret_ucp.uc_stack.ss_sp);
	printf("\tpcb->pcb_userret_ucp.uc_stack.ss_size = %d\n",
		(int) pcb->pcb_userret_ucp.uc_stack.ss_size);
	printf("\tpack->ep_entry                = %p\n",
		(void *) pack->ep_entry);
#endif
}

void
md_syscall_get_syscallnumber(ucontext_t *ucp, uint32_t *code)
{
	uint *reg = (int *) &ucp->uc_mcontext;
	*code = reg[11];			/* EAX */
}

int
md_syscall_getargs(lwp_t *l, ucontext_t *ucp, int nargs, int argsize,
	register_t *args)
{
	uint *reg = (int *) &ucp->uc_mcontext;
	register_t *sp = (register_t *) reg[17];/* ESP */
	int ret;

	//dump_regs(reg);
	ret = copyin(sp + 1, args, argsize);

	return ret;
}

void
md_syscall_set_returnargs(lwp_t *l, ucontext_t *ucp,
	int error, register_t *rval)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext;

	reg[16] &= ~PSL_C;		/* EFL */
	if (error > 0) {
		rval[0] = error;
		reg[16] |= PSL_C;	/* EFL */
	}

	/* set return parameters */
	reg[11]	= rval[0];		/* EAX */
	if (error == 0)
		reg[ 9] = rval[1];	/* EDX */

	//dump_regs(reg);
}

int
md_syscall_check_opcode(ucontext_t *ucp)
{
	uint32_t opcode;

	md_syscall_get_opcode(ucp, &opcode);

	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
		return 1;
	default:
		return 0;
	}
}

void
md_syscall_get_opcode(ucontext_t *ucp, uint32_t *opcode)
{
	register_t *reg = (register_t *) &ucp->uc_mcontext;
//	uint8_t  *p8  = (uint8_t *) (reg[14]);
	uint16_t *p16 = (uint16_t*) (reg[14]);	/* EIP */

	switch (*p16) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
		*opcode = *p16;
		break;
	default:
		*opcode = 0;
	}
}

void
md_syscall_inc_pc(ucontext_t *ucp, uint32_t opcode)
{
	uint *reg = (int *) &ucp->uc_mcontext;

	/* advance program counter */
	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
		reg[14] += 2;	/* EIP */
		break;
	default:
		panic("%s, unknown illegal instruction: opcode = %x\n",
			__func__, (uint32_t) opcode);
	}
}

void
md_syscall_dec_pc(ucontext_t *ucp, uint32_t opcode)
{
	uint *reg = (int *) &ucp->uc_mcontext;

	switch (opcode) {
	case 0xff0f:	/* UD1      */
	case 0xff0b:	/* UD2      */
	case 0x80cd:	/* int $80  */
	case 0x340f:	/* sysenter */
		reg[14] -= 2;	/* EIP */
		break;
	default:
		panic("%s, unknown illegal instruction: opcode = %x\n",
			__func__, (uint32_t) opcode);
	}
}


#else
#	error machdep functions not yet ported to this architecture
#endif

