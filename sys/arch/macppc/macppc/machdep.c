/*	$NetBSD: machdep.c,v 1.147.14.6 2007/06/07 20:30:46 garbled Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.147.14.6 2007/06/07 20:30:46 garbled Exp $");

#include "opt_compat_netbsd.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ipkdb.h"
#include "opt_altivec.h"
#include "opt_multiprocessor.h"
#include "adb.h"
#include "zsc.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#endif
 
#ifdef IPKDB
#include <ipkdb/ipkdb.h>
#endif

#include <machine/autoconf.h>
#include <machine/powerpc.h>
#include <machine/trap.h>
#include <machine/bus.h>
#include <machine/fpu.h>
#include <powerpc/oea/bat.h>
#include <powerpc/spr.h>
#ifdef ALTIVEC
#include <powerpc/altivec.h>
#endif
#include <powerpc/ofw_cons.h>

#include <arch/powerpc/pic/picvar.h>
#include <macppc/dev/adbvar.h>
#include <macppc/dev/pmuvar.h>
#include <macppc/dev/cudavar.h>

#include "ksyms.h"
#include "pmu.h"
#include "cuda.h"

void
initppc(u_int startkernel, u_int endkernel, char *args)
{
	ofwoea_initppc(startkernel, endkernel, args);
}

void
consinit(void)
{
	ofwoea_consinit();
}

/*
 * Machine dependent startup code.
 */
void
cpu_startup(void)
{
	oea_startup(NULL);
}

/*
 * Crash dump handling.
 */

void
dumpsys(void)
{
	printf("dumpsys: TBD\n");
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	/*
	 * Enable external interrupts in case someone is rebooting
	 * from a strange context via ddb.
	 */
	mtmsr(mfmsr() | PSL_EE);

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

#ifdef MULTIPROCESSOR
	/* Halt other CPU.  XXX for now... */
	macppc_send_ipi(&cpu_info[1 - cpu_number()], MACPPC_IPI_HALT);
	delay(100000);	/* XXX */
#endif

	splhigh();

	if (!cold && (howto & RB_DUMP))
		dumpsys();

	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		delay(1000000);
#if NCUDA > 0
		cuda_poweroff();
#endif
#if NPMU > 0
		pmu_poweroff();
#endif
#if NADB > 0
		adb_poweroff();
		printf("WARNING: powerdown failed!\n");
#endif
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		/* flush cache for msgbuf */
		__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

		ppc_exit();
	}

	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;

	/* flush cache for msgbuf */
	__syncicache((void *)msgbuf_paddr, round_page(MSGBUFSIZE));

#if NCUDA > 0
	cuda_restart();
#endif
#if NPMU > 0
	pmu_restart();
#endif
#if NADB > 0
	adb_restart();	/* not return */
#endif
	ppc_exit();
}

#if 0
/*
 * OpenFirmware callback routine
 */
void
callback(void *p)
{
	panic("callback");	/* for now			XXX */
}
#endif

#ifdef MULTIPROCESSOR
/*
 * Save a process's FPU state to its PCB.  The state is in another CPU
 * (though by the time our IPI is processed, it may have been flushed already).
 */
void
mp_save_fpu_lwp(struct lwp *l)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct cpu_info *fpcpu;
	int i;

	/*
	 * Send an IPI to the other CPU with the data and wait for that CPU
	 * to flush the data.  Note that the other CPU might have switched
	 * to a different proc's FPU state by the time it receives the IPI,
	 * but that will only result in an unnecessary reload.
	 */

	fpcpu = pcb->pcb_fpcpu;
	if (fpcpu == NULL) {
		return;
	}
	macppc_send_ipi(fpcpu, MACPPC_IPI_FLUSH_FPU);

	/* Wait for flush. */
#if 0
	while (pcb->pcb_fpcpu)
		;
#else
	for (i = 0; i < 0x3fffffff; i++) {
		if (pcb->pcb_fpcpu == NULL)
			return;
	}
	printf("mp_save_fpu_proc{%d} pid = %d.%d, fpcpu->ci_cpuid = %d\n",
	    cpu_number(), l->l_proc->p_pid, l->l_lid, fpcpu->ci_cpuid);
	panic("mp_save_fpu_proc");
#endif
}

#ifdef ALTIVEC
/*
 * Save a process's AltiVEC state to its PCB.  The state may be in any CPU.
 * The process must either be curproc or traced by curproc (and stopped).
 * (The point being that the process must not run on another CPU during
 * this function).
 */
void
mp_save_vec_lwp(struct lwp *l)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct cpu_info *veccpu;
	int i;

	/*
	 * Send an IPI to the other CPU with the data and wait for that CPU
	 * to flush the data.  Note that the other CPU might have switched
	 * to a different proc's AltiVEC state by the time it receives the IPI,
	 * but that will only result in an unnecessary reload.
	 */

	veccpu = pcb->pcb_veccpu;
	if (veccpu == NULL) {
		return;
	}
	macppc_send_ipi(veccpu, MACPPC_IPI_FLUSH_VEC);

	/* Wait for flush. */
#if 0
	while (pcb->pcb_veccpu)
		;
#else
	for (i = 0; i < 0x3fffffff; i++) {
		if (pcb->pcb_veccpu == NULL)
			return;
	}
	printf("mp_save_vec_lwp{%d} pid = %d.%d, veccpu->ci_cpuid = %d\n",
	    cpu_number(), l->l_proc->p_pid, l->l_lid, veccpu->ci_cpuid);
	panic("mp_save_vec_lwp");
#endif
}
#endif /* ALTIVEC */
#endif /* MULTIPROCESSOR */
