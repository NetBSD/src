/*	$NetBSD: genassym.c,v 1.13.2.2 2000/08/07 01:02:16 mrg Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)genassym.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <uvm/uvm.h>

#include <machine/db_machdep.h>
#include <machine/pmap.h>
#include <machine/cpu.h>

#include <machine/bsd_openprom.h>
#include <machine/reg.h>

#ifdef notyet
#include <machine/bus.h>
#include <sparc64/dev/zsreg.h>
#include <sparc64/dev/zsvar.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>
#endif

#include <sparc64/dev/fdreg.h>
#include <sparc64/dev/fdvar.h>

#include <stdio.h>
#include <stddef.h>

#define	off(what, str, mem)	def(what, (int)offsetof(str, mem))
#define siz(what, str)		def(what, (int)sizeof(str))

void def __P((char *, int));
void flush __P((void));
int main __P((void));

void
def(what, where)
	char *what;
	int where;
{

	if (printf("#define\t%s\t%d\n", what, where) < 0) {
		perror("printf");
		exit(1);
	}
}

void
flush()
{

	if (fflush(stdout)) {
		perror("fflush");
		exit(1);
	}
}

int
main()
{

	/* general constants */
	def("BSD", BSD);
	def("USRSTACK", USRSTACK);
	def("PADDRT", sizeof(paddr_t));

	/* proc fields and values */
	off("P_ADDR", struct proc, p_addr);
	off("P_STAT", struct proc, p_stat);
	off("P_WCHAN", struct proc, p_wchan);
	off("P_VMSPACE", struct proc, p_vmspace);
	off("P_PID", struct proc, p_pid);
	off("P_FPSTATE", struct proc, p_md.md_fpstate);
	def("SRUN", SRUN);
	def("SONPROC", SONPROC);

	/* user struct stuff */
	siz("USIZ", struct user); /* Needed for redzone calculations */

	/* VM structure fields */
	off("VM_PMAP", struct vmspace, vm_map.pmap);

	/* UVM structure fields */
	off("UVM_PAGE_IDLE_ZERO", struct uvm, page_idle_zero);

	/* pmap structure fields */
	off("PM_CTX", struct pmap, pm_ctx);
	off("PM_SEGS", struct pmap, pm_segs);
	off("PM_PHYS", struct pmap, pm_physaddr);

	/* interrupt/fault metering */
	off("V_SWTCH", struct uvmexp, swtch);
	off("V_INTR", struct uvmexp, intrs);
	off("V_FAULTS", struct uvmexp, faults);

	/* CPU info structure */
	off("CI_CURPROC", struct cpu_info, ci_curproc);
	off("CI_CPCB", struct cpu_info, ci_cpcb);
	off("CI_NEXT", struct cpu_info, ci_next);
	off("CI_FPPROC", struct cpu_info, ci_fpproc);
	off("CI_NUMBER", struct cpu_info, ci_number);
	off("CI_UPAID", struct cpu_info, ci_upaid);
	off("CI_SPINUP", struct cpu_info, ci_spinup);
	off("CI_INITSTACK", struct cpu_info, ci_initstack);
	off("CI_PADDR", struct cpu_info, ci_paddr);

	/* FPU state */
	off("FS_REGS", struct fpstate64, fs_regs);
	off("FS_FSR", struct fpstate64, fs_fsr);
	off("FS_GSR", struct fpstate64, fs_gsr);
	off("FS_QSIZE", struct fpstate64, fs_qsize);
	off("FS_QUEUE", struct fpstate64, fs_queue);
	siz("FS_SIZE", struct fpstate64);
	def("FSR_QNE", FSR_QNE);
	def("FPRS_FEF", FPRS_FEF);
	def("FPRS_DU", FPRS_DU);
	def("FPRS_DL", FPRS_DL);

	/* system calls */
	def("SYS___sigreturn14", SYS___sigreturn14);
	def("SYS_execve", SYS_execve);
	def("SYS_exit", SYS_exit);

	/* errno */
	def("EFAULT", EFAULT);
	def("ENAMETOOLONG", ENAMETOOLONG);

	/* PCB fields */
	off("PCB_NSAVED", struct pcb, pcb_nsaved);
	off("PCB_ONFAULT", struct pcb, pcb_onfault);
	off("PCB_PSTATE", struct pcb, pcb_pstate); 
	off("PCB_CWP", struct pcb, pcb_cwp);
	off("PCB_PIL", struct pcb, pcb_pil);
	off("PCB_RW", struct pcb, pcb_rw);
	off("PCB_SP", struct pcb, pcb_sp);
	off("PCB_PC", struct pcb, pcb_pc);
	off("PCB_LASTCALL", struct pcb, lastcall);
	siz("PCB_SIZE", struct pcb);

	/* trapframe64 fields */
	off("TF_TSTATE", struct trapframe64, tf_tstate);
	off("TF_PC", struct trapframe64, tf_pc);
	off("TF_NPC", struct trapframe64, tf_npc);
	off("TF_FAULT", struct trapframe64, tf_fault);
	off("TF_KSTACK", struct trapframe64, tf_kstack);
	off("TF_Y", struct trapframe64, tf_y);
	off("TF_PIL", struct trapframe64, tf_pil);
	off("TF_OLDPIL", struct trapframe64, tf_oldpil);
	off("TF_TT", struct trapframe64, tf_tt);
	off("TF_GLOBAL", struct trapframe64, tf_global);
	off("TF_OUT", struct trapframe64, tf_out);
	off("TF_LOCAL", struct trapframe64, tf_local);
	off("TF_IN", struct trapframe64, tf_in);
	/* shortened versions */
	off("TF_G", struct trapframe64, tf_global);
	off("TF_O", struct trapframe64, tf_out);
	off("TF_L", struct trapframe64, tf_local);
	off("TF_I", struct trapframe64, tf_in);
	siz("TF_SIZE", struct trapframe64);

#if 0
	/* clockframe fields */
	off("CF_TSTATE", struct clockframe, tstate);
	off("CF_PC", struct clockframe, pc);
	off("CF_NPC", struct clockframe, npc);
	off("CF_PIL", struct clockframe, pil);
	off("CF_OLDPIL", struct clockframe, oldpil);
	off("CF_FP", struct clockframe, fp);
#endif

	/* interrupt handler fields */
	off("IH_FUN", struct intrhand, ih_fun);
	off("IH_ARG", struct intrhand, ih_arg);
	off("IH_NUMBER", struct intrhand, ih_number);
	off("IH_PIL", struct intrhand, ih_pil);
	off("IH_PEND", struct intrhand, ih_pending);
	off("IH_NEXT", struct intrhand, ih_next);
	off("IH_MAP", struct intrhand, ih_map);
	off("IH_CLR", struct intrhand, ih_clr);
	siz("IH_SIZE", struct intrhand);

	off("NO_NEXTNODE", struct nodeops, no_nextnode);
	off("NO_GETPROP", struct nodeops, no_getprop);

	/* floppy trap handler fields */
	off("FDC_REG_MSR", struct fdcio, fdcio_reg_msr);
	off("FDC_REG_FIFO", struct fdcio, fdcio_reg_fifo);
	off("FDC_ISTATE", struct fdcio, fdcio_istate);
	off("FDC_STATUS", struct fdcio, fdcio_status);
	off("FDC_NSTAT", struct fdcio, fdcio_nstat);
	off("FDC_DATA", struct fdcio, fdcio_data);
	off("FDC_TC", struct fdcio, fdcio_tc);
	off("FDC_EVCNT", struct fdcio, fdcio_intrcnt.ev_count);

#if 0
	/* db_regs structure so we can save all registers */
	off("DBR_TRAPS", struct db_regs, dbr_traps);
	off("DBR_Y", struct db_regs, dbr_y);
	off("DBR_TL", struct db_regs, dbr_tl);
	off("DBR_CANRESTORE", struct db_regs, dbr_canrestore);
	off("DBR_CANSAVE", struct db_regs, dbr_cansave);
	off("DBR_CLEANWIN", struct db_regs, dbr_cleanwin);
	off("DBR_CWP", struct db_regs, dbr_cwp);
	off("DBR_WSTATE", struct db_regs, dbr_wstate);
	off("DBR_G", struct db_regs, dbr_g);
	off("DBR_AG", struct db_regs, dbr_ag);
	off("DBR_IG", struct db_regs, dbr_ig);
	off("DBR_MG", struct db_regs, dbr_mg);
	off("DBR_OUT", struct db_regs, dbr_out);
	off("DBR_LOCAL", struct db_regs, dbr_local);
	off("DBR_IN", struct db_regs, dbr_in);
#endif

	flush();

	return(0);
}
