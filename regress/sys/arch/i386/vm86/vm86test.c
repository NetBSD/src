/* $NetBSD: vm86test.c,v 1.1 2003/08/16 15:02:36 drochner Exp $ */

/*
 * Copyright (c) 2003
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
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

#include <sys/types.h>
#include <signal.h>
#include <machine/segments.h>
#include <machine/sysarch.h>
#include <machine/vm86.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

/*
 * Some code to execute in vm86 mode. Uses INT 0x20 to print some status info
 * and INT 0x21 to exit.
 * We don't use an IDT, so the interrupts must be vectored to the user task.
 */
const char vmcode[] = {
	0xcd, /* INTxx */
	0x20,
	0xfb, /* STI */
	0xcd, /* INTxx */
	0x20,
	0xfa, /* CLI */
	0xcd, /* INTxx */
	0x21,
};

/*
 * XXX before our text segment, must be below 1M to be addressable.
 * Also chosen to start >=64k, to catch the longstanding kernel bug which
 * caused the stack segment to be ignored in the !SA_ONSTACK case.
 */
#define VMCODEBASE (void *)0x80000
#define VMSIZE 0x4000

void
urghdl(int sig, int code, struct sigcontext *sc)
{
	stack_t oss;
	int res, ip = sc->sc_eip;

	res = sigaltstack(0, &oss);
	if (res < 0)
		err(6, "sigaltstack in handler");
	fprintf(stderr, "urghdl called @ip=%04x, code=%04x, ", ip, code);
	fprintf(stderr, "eflags=%08x, stackflags=%01x\n",
		sc->sc_eflags, oss.ss_flags);
	if (code == VM86_MAKEVAL(VM86_INTx, 0x21))
		exit (0);
}

struct vm86_struct vm; /* zero inited */

int
main(int argc, char **argv)
{
	void *mapaddr;
	unsigned int codeaddr;
	struct sigaction sa;
	int usealtstack, res;
	stack_t ss;

	usealtstack = 0;
	if (argc > 1)
		usealtstack = 1;

	mapaddr = mmap(VMCODEBASE, VMSIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
		       MAP_ANON|MAP_FIXED, -1, 0);
	if (mapaddr == (void *)-1)
		err(1, "mmap");

	memcpy(mapaddr, vmcode, sizeof(vmcode));

	codeaddr = (unsigned int)mapaddr;
	vm.substr.regs.vmsc.sc_cs = codeaddr >> 4;
	vm.substr.regs.vmsc.sc_eip = codeaddr & 15; /* unnecessary here */
	vm.substr.ss_cpu_type = VCPU_586;
	vm.int_byuser[4] = 0x03; /* vector INT 0x20 and 0x21 */

	if (usealtstack) {
		ss.ss_sp = malloc(SIGSTKSZ);
		ss.ss_size = SIGSTKSZ;
		ss.ss_flags = 0;
		res = sigaltstack(&ss, 0);
		if (res < 0)
			err (2, "sigaltstack");
	} else {
		vm.substr.regs.vmsc.sc_ss = codeaddr >> 4;
		vm.substr.regs.vmsc.sc_esp = VMSIZE - (codeaddr & 15); /* hmm */
	}

	sa.sa_handler = (void(*)(int))urghdl;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = usealtstack ? SA_ONSTACK : 0;
	res = sigaction(SIGURG, &sa, 0);
	if (res < 0)
		err(3, "sigaction");

	res = i386_vm86(&vm);
	if (res < 0)
		err(4, "vm86");

	/* NOTREACHED */
	exit (5);
}
