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
__FBSDID("$FreeBSD: head/lib/libproc/proc_regs.c 285003 2015-07-01 13:59:26Z br $");
#else
__RCSID("$NetBSD: proc_regs.c,v 1.4 2015/09/25 19:09:38 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/ptrace.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ucontext.h>
#include "_libproc.h"

int
proc_regget(struct proc_handle *phdl, proc_reg_t reg, unsigned long *regvalue)
{
	struct reg regs;

	if (phdl->status == PS_DEAD || phdl->status == PS_UNDEAD ||
	    phdl->status == PS_IDLE) {
		errno = ENOENT;
		return (-1);
	}
	memset(&regs, 0, sizeof(regs));
	if (ptrace(PT_GETREGS, proc_getpid(phdl), &regs, 0) < 0)
		return (-1);
	switch (reg) {
	case REG_PC:
#ifdef PTRACE_REG_PC
		*regvalue = PTRACE_REG_PC(&regs);
#else
#	error "Add support for your architecture"
#endif
		break;
	case REG_SP:
#ifdef PTRACE_REG_SP
		*regvalue = PTRACE_REG_SP(&regs);
#else
#	error "Add support for your architecture"
#endif
		break;
	default:
		DPRINTFX("ERROR: no support for reg number %d", reg);
		return (-1);
	}

	return (0);
}

int
proc_regset(struct proc_handle *phdl, proc_reg_t reg, unsigned long regvalue)
{
	struct reg regs;

	if (phdl->status == PS_DEAD || phdl->status == PS_UNDEAD ||
	    phdl->status == PS_IDLE) {
		errno = ENOENT;
		return (-1);
	}
	if (ptrace(PT_GETREGS, proc_getpid(phdl), &regs, 0) < 0)
		return (-1);
	switch (reg) {
	case REG_PC:
#ifdef PTRACE_REG_SET_PC
		PTRACE_REG_SET_PC(&regs, regvalue);
#else
#	error "Add support for your architecture"
#endif
		break;
	case REG_SP:
#ifdef PTRACE_REG_SP
		PTRACE_REG_SP(&regs) = regvalue;
#else
#	error "Add support for your architecture"
#endif
		break;
	default:
		DPRINTFX("ERROR: no support for reg number %d", reg);
		return (-1);
	}
	if (ptrace(PT_SETREGS, proc_getpid(phdl), &regs, 0) < 0)
		return (-1);

	return (0);
}
