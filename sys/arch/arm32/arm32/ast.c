/* $NetBSD: ast.c,v 1.1 1996/01/31 23:15:05 mark Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * ast.c
 *
 * Code to handle ast's and returns to user mode
 *
 * Created      : 11/10/94
 * Last updated : 28/08/95
 *
 *    $Id: ast.c,v 1.1 1996/01/31 23:15:05 mark Exp $
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/katelib.h>

int want_resched = 0;
#ifdef DIAGNOSTIC
extern u_int spl_mask;
#endif

void
userret(p, pc, oticks)
	register struct proc *p;
	int pc;
	u_quad_t oticks;
{
	int sig, s;

	if (p == NULL)
		panic("userret: p=0 curproc=%08x", curproc);
    
#ifdef DIAGNOSTIC
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE) {
		traceback();
		panic("userret called in non SVC mode !");
	}

	if (spl_mask != 0xffffffff)
		printf("WARNING: spl_mask=%08x\n", spl_mask);
#endif

/* take pending signals */

	while ((sig = (CURSIG(p))) != 0) {
		postsig(sig);
	}

	p->p_priority = p->p_usrpri;

/*
 * Check for reschedule request, at the moment there is only
 * 1 ast so this code should always be run
 */

	if (want_resched) {
        /*
         * Since we are curproc, a clock interrupt could
         * change our priority without changing run queues
         * (the running process is not kept on a run queue).
         * If this happened after we setrunqueue ourselves but
         * before we switch()'ed, we might not be on the queue
         * indicated by our priority
         */

/*
		s = splhigh();
        	printf("userret: want_resched proc=%08x pid=%d pc=%08x\n", p, p->p_pid, pc);
        	splx(s);
*/
	        s = splstatclock();
		setrunqueue(p);
		p->p_stats->p_ru.ru_nivcsw++;

		mi_switch();

		splx(s);
		while ((sig = (CURSIG(p))) != 0) {
			postsig(sig);
		}
	}

/*
 * The profiling bit is commented out at the moment. This can be reinstated
 * later on. Currently addupc_task is not written.
 */

	if (p->p_flag & P_PROFIL) {
/*#ifdef notyet*/
		extern int psratio;
		addupc_task(p, pc, (int)(p->p_sticks - oticks) * psratio );
/*#endif*/
	}

	curpriority = p->p_priority;

#ifdef DIAGNOSTIC
	if (spl_mask != 0xffffffff)
		printf("WARNING:(1) spl_mask=%08x\n", spl_mask);
#endif
}


/*
 * void ast(trapframe_t *frame)
 *
 * Handle asynchronous system traps.
 * This is called from the irq handler to deliver signals
 * and switch processes if required.
 * userret() does all the signal delivery and process switching work
 */

void
ast(frame)
	trapframe_t *frame;
{
	register struct proc *p;

	cnt.v_trap++;

	if ((p = curproc) == 0)
		p = &proc0;
	if (&p->p_addr->u_pcb == 0)
		panic("ast: nopcb!");

	cnt.v_soft++;
	if (p->p_flag & P_OWEUPC) {
		p->p_flag &= ~P_OWEUPC;
		ADDUPROF(p);
	}
#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 3);
#endif

	userret(p, frame->tf_pc, p->p_sticks);

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 3);
#endif
}

/* End of ast.c */
