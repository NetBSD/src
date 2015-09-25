/*
 * Copyright (c) 2010 The FreeBSD Foundation 
 * All rights reserved. 
 * 
 * This software was developed by Rui Paulo under sponsorship from the
 * FreeBSD Foundation. 
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */ 

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/lib/libproc/proc_bkpt.c 287106 2015-08-24 12:17:15Z andrew $");
#else
__RCSID("$NetBSD: proc_bkpt.c,v 1.4 2015/09/25 19:09:38 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <assert.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include "_libproc.h"

#ifndef PTRACE_BREAKPOINT
#error "Add support for your architecture"
#endif

static int
proc_stop(struct proc_handle *phdl)
{
	int status;

	if (kill(proc_getpid(phdl), SIGSTOP) == -1) {
		DPRINTF("kill %d", proc_getpid(phdl));
		return (-1);
	} else if (waitpid(proc_getpid(phdl), &status, WSTOPPED) == -1) {
		DPRINTF("waitpid %d", proc_getpid(phdl));
		return (-1);
	} else if (!WIFSTOPPED(status)) {
		DPRINTFX("waitpid: unexpected status 0x%x", status);
		return (-1);
	}

	return (0);
}

int
proc_bkptset(struct proc_handle *phdl, uintptr_t address,
    proc_breakpoint_t *saved)
{
	struct ptrace_io_desc piod;
	unsigned long paddr, caddr;
	int ret = 0, stopped;
	proc_breakpoint_t copy;

	if (phdl->status == PS_DEAD || phdl->status == PS_UNDEAD ||
	    phdl->status == PS_IDLE) {
		errno = ENOENT;
		return (-1);
	}

	DPRINTFX("adding breakpoint at 0x%lx", address);

	stopped = 0;
	if (phdl->status != PS_STOP) {
		if (proc_stop(phdl) != 0)
			return (-1);
		stopped = 1;
	}

	/*
	 * Read the original instruction.
	 */
	caddr = address;
	paddr = 0;
	piod.piod_op = PIOD_READ_I;
	piod.piod_offs = (void *)caddr;
	piod.piod_addr = (void *)saved->data;
	piod.piod_len  = sizeof(saved->data);
	if (ptrace(PT_IO, proc_getpid(phdl), &piod, 0) < 0) {
		DPRINTF("ERROR: couldn't read instruction at address 0x%"
		    PRIuPTR, address);
		ret = -1;
		goto done;
	}
	/*
	 * Write a breakpoint instruction to that address.
	 */
	caddr = address;
	piod.piod_op = PIOD_WRITE_I;
	piod.piod_offs = (void *)caddr;
	piod.piod_addr = (void *)PTRACE_BREAKPOINT;
	piod.piod_len  = sizeof(PTRACE_BREAKPOINT);
	if (ptrace(PT_IO, proc_getpid(phdl), &piod, 0) < 0) {
		DPRINTF("ERROR: couldn't write instruction at address 0x%"
		    PRIuPTR, address);
		ret = -1;
		goto done;
	}

done:
	if (stopped)
		/* Restart the process if we had to stop it. */
		proc_continue(phdl);

	return (ret);
}

int
proc_bkptdel(struct proc_handle *phdl, uintptr_t address,
    proc_breakpoint_t *saved)
{
	struct ptrace_io_desc piod;
	unsigned long paddr, caddr;
	int ret = 0, stopped;

	if (phdl->status == PS_DEAD || phdl->status == PS_UNDEAD ||
	    phdl->status == PS_IDLE) {
		errno = ENOENT;
		return (-1);
	}

	DPRINTFX("removing breakpoint at 0x%lx", address);

	stopped = 0;
	if (phdl->status != PS_STOP) {
		if (proc_stop(phdl) != 0)
			return (-1);
		stopped = 1;
	}

	/*
	 * Overwrite the breakpoint instruction that we setup previously.
	 */
	caddr = address;
	piod.piod_op = PIOD_WRITE_I;
	piod.piod_offs = (void *)caddr;
	piod.piod_addr = (void *)saved->data;
	piod.piod_len  = sizeof(saved->data);
	if (ptrace(PT_IO, proc_getpid(phdl), &piod, 0) < 0) {
		DPRINTF("ERROR: couldn't write instruction at address 0x%"
		    PRIuPTR, address);
		ret = -1;
	}

	if (stopped)
		/* Restart the process if we had to stop it. */
		proc_continue(phdl);
 
	return (ret);
}

/*
 * Decrement pc so that we delete the breakpoint at the correct
 * address, i.e. at the breakpoint instruction address.
 *
 * This is only needed on some architectures where the pc value
 * when reading registers points at the instruction after the
 * breakpoint, e.g. x86.
 */
void
proc_bkptregadj(unsigned long *pc)
{

	(void)pc;
#ifdef PTRACE_BREAKPOINT_ADJ
	*pc = *pc - PTRACE_BREAKPOINT_ADJ;
#endif
}

/*
 * Step over the breakpoint.
 */
int
proc_bkptexec(struct proc_handle *phdl, proc_breakpoint_t *saved)
{
	unsigned long pc;
	proc_breakpoint_t samesaved;
	int status;

	if (proc_regget(phdl, REG_PC, &pc) < 0) {
		DPRINTFX("ERROR: couldn't get PC register");
		return (-1);
	}
	proc_bkptregadj(&pc);
	if (proc_bkptdel(phdl, pc, saved) < 0) {
		DPRINTFX("ERROR: couldn't delete breakpoint");
		return (-1);
	}
	/*
	 * Go back in time and step over the new instruction just
	 * set up by proc_bkptdel().
	 */
	proc_regset(phdl, REG_PC, pc);
	if (ptrace(PT_STEP, proc_getpid(phdl), (void *)(intptr_t)1, 0) < 0) {
		DPRINTFX("ERROR: ptrace step failed");
		return (-1);
	}
	proc_wstatus(phdl);
	status = proc_getwstat(phdl);
	if (!WIFSTOPPED(status)) {
		DPRINTFX("ERROR: don't know why process stopped");
		return (-1);
	}
	/*
	 * Restore the breakpoint. The saved instruction should be
	 * the same as the one that we were passed in.
	 */
	if (proc_bkptset(phdl, pc, &samesaved) < 0) {
		DPRINTFX("ERROR: couldn't restore breakpoint");
		return (-1);
	}

	assert(memcmp(saved, &samesaved, sizeof(samesaved)) == 0);

	return (0);
}
