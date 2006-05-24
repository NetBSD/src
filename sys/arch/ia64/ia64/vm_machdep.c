/*	$NetBSD: vm_machdep.c,v 1.1.10.2 2006/05/24 15:47:59 tron Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author: 
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/* Comments on functions from alpha/vm_machdep.c */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

void
cpu_lwp_free(struct lwp *l, int proc)
{
}

/*
 * cpu_exit is called as the last action during exit.
 * We block interrupts and call switch_exit.  switch_exit switches
 * to proc0's PCB and stack, then jumps into the middle of cpu_switch,
 * as if it were switching from proc0.
 */
void
cpu_exit(struct lwp *l)
{
	(void) splhigh();
	/* NOTREACHED */
}

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the pcb and trap frame, making the child ready to run.
 * 
 * Rig the child's kernel stack so that it will start out in
 * proc_trampoline() and call child_return() with p2 as an
 * argument. This causes the newly-created child process to go
 * directly to user level with an apparent return value of 0 from
 * fork(), while the parent process returns normally.
 *
 * p1 is the process being forked; if p1 == &proc0, we are creating
 * a kernel thread, and the return path and argument are specified with
 * `func' and `arg'.
 *
 * If an alternate user-level stack is requested (with non-zero values
 * in both the stack and stacksize args), set up the user stack pointer
 * accordingly.
 */
void
cpu_lwp_fork(struct lwp *l1, struct lwp *l2, void *stack, size_t stacksize,
    void (*func)(void *), void *arg)
{
	return;
}

void
cpu_setfunc(l, func, arg)
	struct lwp *l;
	void (*func) __P((void *));
	void *arg;
{
	return;
}

/*
 * Finish a swapin operation.
 *
 * We need to cache the physical address of the PCB, so we can
 * swap context to it easily.
 */
void
cpu_swapin(struct lwp *l)
{
	return;
}

/*
 * cpu_swapout is called immediately before a process's 'struct user'
 * and kernel stack are unwired (which are in turn done immediately
 * before it's P_INMEM flag is cleared).  If the process is the
 * current owner of the floating point unit, the FP state has to be
 * saved, so that it goes out with the pcb, which is in the user area.
 */
void
cpu_swapout(struct lwp *l)
{
	return;
}

/*
 * Map a user I/O request into kernel virtual address space.
 * Note: the pages are already locked by uvm_vslock(), so we
 * do not need to pass an access_type to pmap_enter().
 */
void
vmapbuf(struct buf *bp, vsize_t len)
{
	return;
}

/*
 * Unmap a previously-mapped user I/O request.
 */
void
vunmapbuf(struct buf *bp, vsize_t len)
{
	return;
}
