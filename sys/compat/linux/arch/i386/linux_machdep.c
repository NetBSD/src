/*	$NetBSD: linux_machdep.c,v 1.49.2.1 2000/06/22 17:05:47 minoura Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#include "opt_vm86.h"
#include "opt_user_ldt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/syscallargs.h>
#include <sys/filedesc.h>
#include <sys/exec_elf.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/segments.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/vm86.h>
#include <machine/vmparam.h>

/*
 * To see whether wscons is configured (for virtual console ioctl calls).
 */
#include "wsdisplay.h"
#if (NWSDISPLAY > 0)
#include <sys/ioctl.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#include "opt_xserver.h"
#endif

#ifdef USER_LDT
#include <machine/cpu.h>
int linux_read_ldt __P((struct proc *, struct linux_sys_modify_ldt_args *,
    register_t *));
int linux_write_ldt __P((struct proc *, struct linux_sys_modify_ldt_args *,
    register_t *));
#endif

/*
 * Deal with some i386-specific things in the Linux emulation code.
 */

void
linux_setregs(p, epp, stack)
	struct proc *p;
	struct exec_package *epp;
	u_long stack;
{
	struct pcb *pcb = &p->p_addr->u_pcb;

	setregs(p, epp, stack);
	pcb->pcb_savefpu.sv_env.en_cw = __Linux_NPXCW__;
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */

void
linux_sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig;
	sigset_t *mask;
	u_long code;
{
	struct proc *p = curproc;
	struct trapframe *tf;
	struct linux_sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;

	tf = p->p_md.md_regs;

	/* Allocate space for the signal handler context. */
	/* XXX Linux doesn't support the signal stack. */
	fp = (struct linux_sigframe *)tf->tf_esp;
	fp--;

	/* Build stack frame for signal trampoline. */
	frame.sf_handler = catcher;
	frame.sf_sig = native_to_linux_sig[sig];

	/* Save register context. */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		frame.sf_sc.sc_eflags = get_vflags(p);
	} else
#endif
	{
		__asm("movl %%gs,%w0" : "=r" (frame.sf_sc.sc_gs));
		__asm("movl %%fs,%w0" : "=r" (frame.sf_sc.sc_fs));
		frame.sf_sc.sc_es = tf->tf_es;
		frame.sf_sc.sc_ds = tf->tf_ds;
		frame.sf_sc.sc_eflags = tf->tf_eflags;
	}
	frame.sf_sc.sc_edi = tf->tf_edi;
	frame.sf_sc.sc_esi = tf->tf_esi;
	frame.sf_sc.sc_ebp = tf->tf_ebp;
	frame.sf_sc.sc_ebx = tf->tf_ebx;
	frame.sf_sc.sc_edx = tf->tf_edx;
	frame.sf_sc.sc_ecx = tf->tf_ecx;
	frame.sf_sc.sc_eax = tf->tf_eax;
	frame.sf_sc.sc_eip = tf->tf_eip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp_at_signal = tf->tf_esp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_err = tf->tf_err;
	frame.sf_sc.sc_trapno = tf->tf_trapno;

	/* Save signal stack. */
	/* XXX Linux doesn't support the signal stack. */

	/* Save signal mask. */
	native_to_linux_old_sigset(mask, &frame.sf_sc.sc_mask);

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)psp->ps_sigcode;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	/* XXX Linux doesn't support the signal stack. */
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper privileges or to cause
 * a machine fault.
 */
int
linux_sys_rt_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* XXX XAX write me */
	return(ENOSYS);
}

int
linux_sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sigreturn_args /* {
		syscallarg(struct linux_sigcontext *) scp;
	} */ *uap = v;
	struct linux_sigcontext *scp, context;
	struct trapframe *tf;
	sigset_t mask;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, scp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	/* Restore register context. */
	tf = p->p_md.md_regs;
#ifdef VM86
	if (context.sc_eflags & PSL_VM) {
		tf->tf_vm86_gs = context.sc_gs;
		tf->tf_vm86_fs = context.sc_fs;
		tf->tf_vm86_es = context.sc_es;
		tf->tf_vm86_ds = context.sc_ds;
		set_vflags(p, context.sc_eflags);
	} else
#endif
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((context.sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0 ||
		    !USERMODE(context.sc_cs, context.sc_eflags))
			return (EINVAL);

		/* %fs and %gs were restored by the trampoline. */
		tf->tf_es = context.sc_es;
		tf->tf_ds = context.sc_ds;
		tf->tf_eflags = context.sc_eflags;
	}
	tf->tf_edi = context.sc_edi;
	tf->tf_esi = context.sc_esi;
	tf->tf_ebp = context.sc_ebp;
	tf->tf_ebx = context.sc_ebx;
	tf->tf_edx = context.sc_edx;
	tf->tf_ecx = context.sc_ecx;
	tf->tf_eax = context.sc_eax;
	tf->tf_eip = context.sc_eip;
	tf->tf_cs = context.sc_cs;
	tf->tf_esp = context.sc_esp_at_signal;
	tf->tf_ss = context.sc_ss;

	/* Restore signal stack. */
	p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;

	/* Restore signal mask. */
	linux_old_to_native_sigset(&context.sc_mask, &mask);
	(void) sigprocmask1(p, SIG_SETMASK, &mask, 0);

	return (EJUSTRETURN);
}

#ifdef USER_LDT

int
linux_read_ldt(p, uap, retval)
	struct proc *p;
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap;
	register_t *retval;
{
	struct i386_get_ldt_args gl;
	int error;
	caddr_t sg;
	char *parms;

	sg = stackgap_init(p->p_emul);

	gl.start = 0;
	gl.desc = SCARG(uap, ptr);
	gl.num = SCARG(uap, bytecount) / sizeof(union descriptor);

	parms = stackgap_alloc(&sg, sizeof(gl));

	if ((error = copyout(&gl, parms, sizeof(gl))) != 0)
		return (error);

	if ((error = i386_get_ldt(p, parms, retval)) != 0)
		return (error);

	*retval *= sizeof(union descriptor);
	return (0);
}

struct linux_ldt_info {
	u_int entry_number;
	u_long base_addr;
	u_int limit;
	u_int seg_32bit:1;
	u_int contents:2;
	u_int read_exec_only:1;
	u_int limit_in_pages:1;
	u_int seg_not_present:1;
};

int
linux_write_ldt(p, uap, retval)
	struct proc *p;
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap;
	register_t *retval;
{
	struct linux_ldt_info ldt_info;
	struct segment_descriptor sd;
	struct i386_set_ldt_args sl;
	int error;
	caddr_t sg;
	char *parms;

	if (SCARG(uap, bytecount) != sizeof(ldt_info))
		return (EINVAL);
	if ((error = copyin(SCARG(uap, ptr), &ldt_info, sizeof(ldt_info))) != 0)
		return error;
	if (ldt_info.contents == 3)
		return (EINVAL);

	sg = stackgap_init(p->p_emul);

	sd.sd_lobase = ldt_info.base_addr & 0xffffff;
	sd.sd_hibase = (ldt_info.base_addr >> 24) & 0xff;
	sd.sd_lolimit = ldt_info.limit & 0xffff;
	sd.sd_hilimit = (ldt_info.limit >> 16) & 0xf;
	sd.sd_type =
	    16 | (ldt_info.contents << 2) | (!ldt_info.read_exec_only << 1);
	sd.sd_dpl = SEL_UPL;
	sd.sd_p = !ldt_info.seg_not_present;
	sd.sd_def32 = ldt_info.seg_32bit;
	sd.sd_gran = ldt_info.limit_in_pages;

	sl.start = ldt_info.entry_number;
	sl.desc = stackgap_alloc(&sg, sizeof(sd));
	sl.num = 1;

#if 0
	printf("linux_write_ldt: idx=%d, base=%x, limit=%x\n",
	    ldt_info.entry_number, ldt_info.base_addr, ldt_info.limit);
#endif

	parms = stackgap_alloc(&sg, sizeof(sl));

	if ((error = copyout(&sd, sl.desc, sizeof(sd))) != 0)
		return (error);
	if ((error = copyout(&sl, parms, sizeof(sl))) != 0)
		return (error);

	if ((error = i386_set_ldt(p, parms, retval)) != 0)
		return (error);

	*retval = 0;
	return (0);
}

#endif /* USER_LDT */

int
linux_sys_modify_ldt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_modify_ldt_args /* {
		syscallarg(int) func;
		syscallarg(void *) ptr;
		syscallarg(size_t) bytecount;
	} */ *uap = v;

	switch (SCARG(uap, func)) {
#ifdef USER_LDT
	case 0:
		return (linux_read_ldt(p, uap, retval));

	case 1:
		return (linux_write_ldt(p, uap, retval));
#endif /* USER_LDT */

	default:
		return (ENOSYS);
	}
}

/*
 * XXX Pathetic hack to make svgalib work. This will fake the major
 * device number of an opened VT so that svgalib likes it. grmbl.
 * Should probably do it 'wrong the right way' and use a mapping
 * array for all major device numbers, and map linux_mknod too.
 */
dev_t
linux_fakedev(dev)
	dev_t dev;
{
#if (NWSDISPLAY > 0)
	if (major(dev) == NETBSD_WSCONS_MAJOR)
		return makedev(LINUX_CONS_MAJOR, (minor(dev) + 1));
#endif
	return dev;
}

#if (NWSDISPLAY > 0)
/*
 * That's not complete, but enough to get an X server running.
 */
#define NR_KEYS 128
static u_short plain_map[NR_KEYS] = {
	0x0200,	0x001b,	0x0031,	0x0032,	0x0033,	0x0034,	0x0035,	0x0036,
	0x0037,	0x0038,	0x0039,	0x0030,	0x002d,	0x003d,	0x007f,	0x0009,
	0x0b71,	0x0b77,	0x0b65,	0x0b72,	0x0b74,	0x0b79,	0x0b75,	0x0b69,
	0x0b6f,	0x0b70,	0x005b,	0x005d,	0x0201,	0x0702,	0x0b61,	0x0b73,
	0x0b64,	0x0b66,	0x0b67,	0x0b68,	0x0b6a,	0x0b6b,	0x0b6c,	0x003b,
	0x0027,	0x0060,	0x0700,	0x005c,	0x0b7a,	0x0b78,	0x0b63,	0x0b76,
	0x0b62,	0x0b6e,	0x0b6d,	0x002c,	0x002e,	0x002f,	0x0700,	0x030c,
	0x0703,	0x0020,	0x0207,	0x0100,	0x0101,	0x0102,	0x0103,	0x0104,
	0x0105,	0x0106,	0x0107,	0x0108,	0x0109,	0x0208,	0x0209,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x003c,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x001c,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, shift_map[NR_KEYS] = {
	0x0200,	0x001b,	0x0021,	0x0040,	0x0023,	0x0024,	0x0025,	0x005e,
	0x0026,	0x002a,	0x0028,	0x0029,	0x005f,	0x002b,	0x007f,	0x0009,
	0x0b51,	0x0b57,	0x0b45,	0x0b52,	0x0b54,	0x0b59,	0x0b55,	0x0b49,
	0x0b4f,	0x0b50,	0x007b,	0x007d,	0x0201,	0x0702,	0x0b41,	0x0b53,
	0x0b44,	0x0b46,	0x0b47,	0x0b48,	0x0b4a,	0x0b4b,	0x0b4c,	0x003a,
	0x0022,	0x007e,	0x0700,	0x007c,	0x0b5a,	0x0b58,	0x0b43,	0x0b56,
	0x0b42,	0x0b4e,	0x0b4d,	0x003c,	0x003e,	0x003f,	0x0700,	0x030c,
	0x0703,	0x0020,	0x0207,	0x010a,	0x010b,	0x010c,	0x010d,	0x010e,
	0x010f,	0x0110,	0x0111,	0x0112,	0x0113,	0x0213,	0x0203,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x003e,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x0200,	0x0701,	0x0205,	0x0114,	0x0603,
	0x020b,	0x0601,	0x0602,	0x0117,	0x0600,	0x020a,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, altgr_map[NR_KEYS] = {
	0x0200,	0x0200,	0x0200,	0x0040,	0x0200,	0x0024,	0x0200,	0x0200,
	0x007b,	0x005b,	0x005d,	0x007d,	0x005c,	0x0200,	0x0200,	0x0200,
	0x0b71,	0x0b77,	0x0918,	0x0b72,	0x0b74,	0x0b79,	0x0b75,	0x0b69,
	0x0b6f,	0x0b70,	0x0200,	0x007e,	0x0201,	0x0702,	0x0914,	0x0b73,
	0x0917,	0x0919,	0x0b67,	0x0b68,	0x0b6a,	0x0b6b,	0x0b6c,	0x0200,
	0x0200,	0x0200,	0x0700,	0x0200,	0x0b7a,	0x0b78,	0x0916,	0x0b76,
	0x0915,	0x0b6e,	0x0b6d,	0x0200,	0x0200,	0x0200,	0x0700,	0x030c,
	0x0703,	0x0200,	0x0207,	0x050c,	0x050d,	0x050e,	0x050f,	0x0510,
	0x0511,	0x0512,	0x0513,	0x0514,	0x0515,	0x0208,	0x0202,	0x0911,
	0x0912,	0x0913,	0x030b,	0x090e,	0x090f,	0x0910,	0x030a,	0x090b,
	0x090c,	0x090d,	0x090a,	0x0310,	0x0206,	0x0200,	0x007c,	0x0516,
	0x0517,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x0200,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
}, ctrl_map[NR_KEYS] = {
	0x0200,	0x0200,	0x0200,	0x0000,	0x001b,	0x001c,	0x001d,	0x001e,
	0x001f,	0x007f,	0x0200,	0x0200,	0x001f,	0x0200,	0x0008,	0x0200,
	0x0011,	0x0017,	0x0005,	0x0012,	0x0014,	0x0019,	0x0015,	0x0009,
	0x000f,	0x0010,	0x001b,	0x001d,	0x0201,	0x0702,	0x0001,	0x0013,
	0x0004,	0x0006,	0x0007,	0x0008,	0x000a,	0x000b,	0x000c,	0x0200,
	0x0007,	0x0000,	0x0700,	0x001c,	0x001a,	0x0018,	0x0003,	0x0016,
	0x0002,	0x000e,	0x000d,	0x0200,	0x020e,	0x007f,	0x0700,	0x030c,
	0x0703,	0x0000,	0x0207,	0x0100,	0x0101,	0x0102,	0x0103,	0x0104,
	0x0105,	0x0106,	0x0107,	0x0108,	0x0109,	0x0208,	0x0204,	0x0307,
	0x0308,	0x0309,	0x030b,	0x0304,	0x0305,	0x0306,	0x030a,	0x0301,
	0x0302,	0x0303,	0x0300,	0x0310,	0x0206,	0x0200,	0x0200,	0x010a,
	0x010b,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
	0x030e,	0x0702,	0x030d,	0x001c,	0x0701,	0x0205,	0x0114,	0x0603,
	0x0118,	0x0601,	0x0602,	0x0117,	0x0600,	0x0119,	0x0115,	0x0116,
	0x011a,	0x010c,	0x010d,	0x011b,	0x011c,	0x0110,	0x0311,	0x011d,
	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,	0x0200,
};

u_short *linux_keytabs[] = {
	plain_map, shift_map, altgr_map, altgr_map, ctrl_map
};
#endif

/*
 * We come here in a last attempt to satisfy a Linux ioctl() call
 */
int
linux_machdepioctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	struct sys_ioctl_args bia;
	u_long com;
#if (NWSDISPLAY > 0)
	int error;
	struct vt_mode lvt;
	caddr_t bvtp, sg;
	struct kbentry kbe;
#endif

	SCARG(&bia, fd) = SCARG(uap, fd);
	SCARG(&bia, data) = SCARG(uap, data);
	com = SCARG(uap, com);

	switch (com) {
#if (NWSDISPLAY > 0)
	case LINUX_KDGKBMODE:
		com = KDGKBMODE;
		break;
	case LINUX_KDSKBMODE:
		com = KDSKBMODE;
		if ((unsigned)SCARG(uap, data) == LINUX_K_MEDIUMRAW)
			SCARG(&bia, data) = (caddr_t)K_RAW;
		break;
	case LINUX_KDMKTONE:
		com = KDMKTONE;
		break;
	case LINUX_KDSETMODE:
		com = KDSETMODE;
		break;
	case LINUX_KDENABIO:
		com = KDENABIO;
		break;
	case LINUX_KDDISABIO:
		com = KDDISABIO;
		break;
	case LINUX_KDGETLED:
		com = KDGETLED;
		break;
	case LINUX_KDSETLED:
		com = KDSETLED;
		break;
	case LINUX_VT_OPENQRY:
		com = VT_OPENQRY;
		break;
	case LINUX_VT_GETMODE:
		SCARG(&bia, com) = VT_GETMODE;
		if ((error = sys_ioctl(p, &bia, retval)))
			return error;
		if ((error = copyin(SCARG(uap, data), (caddr_t)&lvt,
		    sizeof (struct vt_mode))))
			return error;
		lvt.relsig = native_to_linux_sig[lvt.relsig];
		lvt.acqsig = native_to_linux_sig[lvt.acqsig];
		lvt.frsig = native_to_linux_sig[lvt.frsig];
		return copyout((caddr_t)&lvt, SCARG(uap, data),
		    sizeof (struct vt_mode));
	case LINUX_VT_SETMODE:
		com = VT_SETMODE;
		if ((error = copyin(SCARG(uap, data), (caddr_t)&lvt,
		    sizeof (struct vt_mode))))
			return error;
		lvt.relsig = linux_to_native_sig[lvt.relsig];
		lvt.acqsig = linux_to_native_sig[lvt.acqsig];
		lvt.frsig = linux_to_native_sig[lvt.frsig];
		sg = stackgap_init(p->p_emul);
		bvtp = stackgap_alloc(&sg, sizeof (struct vt_mode));
		if ((error = copyout(&lvt, bvtp, sizeof (struct vt_mode))))
			return error;
		SCARG(&bia, data) = bvtp;
		break;
	case LINUX_VT_RELDISP:
		com = VT_RELDISP;
		break;
	case LINUX_VT_ACTIVATE:
		com = VT_ACTIVATE;
		break;
	case LINUX_VT_WAITACTIVE:
		com = VT_WAITACTIVE;
		break;
	case LINUX_VT_GETSTATE:
		com = VT_GETSTATE;
		break;
	case LINUX_KDGKBTYPE:
		/* This is what Linux does. */
		return (subyte(SCARG(uap, data), KB_101));
	case LINUX_KDGKBENT:
		/*
		 * The Linux KDGKBENT ioctl is different from the
		 * SYSV original. So we handle it in machdep code.
		 * XXX We should use keyboard mapping information
		 * from wsdisplay, but this would be expensive.
		 */
		if ((error = copyin(SCARG(uap, data), &kbe,
				    sizeof(struct kbentry))))
			return (error);
		if (kbe.kb_table >= sizeof(linux_keytabs) / sizeof(u_short *)
		    || kbe.kb_index >= NR_KEYS)
			return (EINVAL);
		kbe.kb_value = linux_keytabs[kbe.kb_table][kbe.kb_index];
		return (copyout(&kbe, SCARG(uap, data),
				sizeof(struct kbentry)));
#endif
	default:
		printf("linux_machdepioctl: invalid ioctl %08lx\n", com);
		return EINVAL;
	}
	SCARG(&bia, com) = com;
	return sys_ioctl(p, &bia, retval);
}

/*
 * Set I/O permissions for a process. Just set the maximum level
 * right away (ignoring the argument), otherwise we would have
 * to rely on I/O permission maps, which are not implemented.
 */
int
linux_sys_iopl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct linux_sys_iopl_args /* {
		syscallarg(int) level;
	} */ *uap = v;
#endif
	struct trapframe *fp = p->p_md.md_regs;

	if (suser(p->p_ucred, &p->p_acflag) != 0)
		return EPERM;
	fp->tf_eflags |= PSL_IOPL;
	*retval = 0;
	return 0;
}

/*
 * See above. If a root process tries to set access to an I/O port,
 * just let it have the whole range.
 */
int
linux_sys_ioperm(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_ioperm_args /* {
		syscallarg(unsigned int) lo;
		syscallarg(unsigned int) hi;
		syscallarg(int) val;
	} */ *uap = v;
	struct trapframe *fp = p->p_md.md_regs;

	if (suser(p->p_ucred, &p->p_acflag) != 0)
		return EPERM;
	if (SCARG(uap, val))
		fp->tf_eflags |= PSL_IOPL;
	*retval = 0;
	return 0;
}
