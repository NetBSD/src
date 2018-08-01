/*	$NetBSD: kgdb_machdep.c,v 1.1 2018/08/01 10:22:20 reinoud Exp $	*/

/*
 * Copyright (c) 1996 Matthias Pfaller.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kgdb_machdep.c,v 1.1 2018/08/01 10:22:20 reinoud Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kgdb.h>
#include <sys/socket.h>

#include <uvm/uvm_extern.h>

//#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/db_machdep.h>
#include <machine/thunk.h>
#include <netinet/in.h>


/*
 * Determine if the memory at va..(va+len) is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	vaddr_t last_va;

	last_va = va + len;
	va  &= ~PGOFSET;
	last_va &= ~PGOFSET;

thunk_printf("%s: [%p .. %p]\n", __func__, (void *) va, (void *) last_va);
	do {
		if (db_validate_address(va))
			return (0);
		va  += PAGE_SIZE;
	} while (va < last_va);

	return (1);
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 */
int 
kgdb_signal(int type)
{
	return type;

//	panic("%s", __func__);
#if 0
	switch (type) {
	case T_BREAKPOINT:
		return(SIGTRAP);
	case -1:
		return(SIGSEGV);
	default:
		return(SIGINT);
	}
#endif
}

/*
 * Definitions exported from gdb.
 */

/*
 * Translate the values stored in the kernel regs struct to the format
 * understood by gdb.
 */
void
kgdb_getregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
#ifdef __x86_64__
	kgdb_reg_t *gregs = regs->uc_mcontext.__gregs;

	gdb_regs[ 0] = gregs[_REG_RAX];
	gdb_regs[ 1] = gregs[_REG_RBX];
	gdb_regs[ 2] = gregs[_REG_RCX];
	gdb_regs[ 3] = gregs[_REG_RDX];
	gdb_regs[ 4] = gregs[_REG_RSI];
	gdb_regs[ 5] = gregs[_REG_RDI];
	gdb_regs[ 6] = gregs[_REG_RBP];
	gdb_regs[ 7] = gregs[_REG_RSP];
	gdb_regs[ 8] = gregs[_REG_R8];
	gdb_regs[ 9] = gregs[_REG_R9];
	gdb_regs[10] = gregs[_REG_R10];
	gdb_regs[11] = gregs[_REG_R11];
	gdb_regs[12] = gregs[_REG_R12];
	gdb_regs[13] = gregs[_REG_R13];
	gdb_regs[14] = gregs[_REG_R14];
	gdb_regs[15] = gregs[_REG_R15];
	gdb_regs[16] = gregs[_REG_RIP];
	gdb_regs[17] = gregs[_REG_RFLAGS];
	gdb_regs[18] = gregs[_REG_CS];
	gdb_regs[19] = gregs[_REG_SS];
		
#elif defined(__i386)
	gdb_regs[ 0] = regs->tf_eax;
	gdb_regs[ 1] = regs->tf_ecx;
	gdb_regs[ 2] = regs->tf_edx;
	gdb_regs[ 3] = regs->tf_ebx;
	gdb_regs[ 4] = regs->tf_esp;
	gdb_regs[ 5] = regs->tf_ebp;
	gdb_regs[ 6] = regs->tf_esi;
	gdb_regs[ 7] = regs->tf_edi;
	gdb_regs[ 8] = regs->tf_eip;
	gdb_regs[ 9] = regs->tf_eflags;
	gdb_regs[10] = regs->tf_cs;
	gdb_regs[11] = regs->tf_ss;
	gdb_regs[12] = regs->tf_ds;
	gdb_regs[13] = regs->tf_es;
	gdb_regs[14] = regs->tf_fs;
	gdb_regs[15] = regs->tf_gs;

/*XXX OOPS XXX? */
#if 0
	if (KERNELMODE(regs->tf_cs)) {
		/*
		 * Kernel mode - esp and ss not saved.
		 */
		gdb_regs[ 4] = (kgdb_reg_t)&regs->tf_esp; /* kernel stack
							     pointer */
		gdb_regs[11] = x86_getss();
	}
#endif
#else
#error port kgdb_machdep.c kgdb_getregs
#endif
}

 /*
 * And the reverse.
 */

void
kgdb_setregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{
#ifdef __x86_64__
	kgdb_reg_t *gregs = regs->uc_mcontext.__gregs;

	gregs[_REG_RAX] = gdb_regs[ 0];
	gregs[_REG_RBX] = gdb_regs[ 1];
	gregs[_REG_RCX] = gdb_regs[ 2];
	gregs[_REG_RDX] = gdb_regs[ 3];
	gregs[_REG_RSI] = gdb_regs[ 4];
	gregs[_REG_RDI] = gdb_regs[ 5];
	gregs[_REG_RBP] = gdb_regs[ 6];
	gregs[_REG_RSP] = gdb_regs[ 7];
	gregs[_REG_R8 ] = gdb_regs[ 8];
	gregs[_REG_R9 ] = gdb_regs[ 9];
	gregs[_REG_R10] = gdb_regs[10];
	gregs[_REG_R11] = gdb_regs[11];
	gregs[_REG_R12] = gdb_regs[12];
	gregs[_REG_R13] = gdb_regs[13];
	gregs[_REG_R14] = gdb_regs[14];
	gregs[_REG_R15] = gdb_regs[15];
	gregs[_REG_RIP] = gdb_regs[16];
	gregs[_REG_RFLAGS] = gdb_regs[17];
	gregs[_REG_CS ] = gdb_regs[18];
	gregs[_REG_SS ] = gdb_regs[19];
#elif defined(__i386)
panic("%s", __func__);
	regs->tf_eax    = gdb_regs[ 0];
	regs->tf_ecx    = gdb_regs[ 1];
	regs->tf_edx    = gdb_regs[ 2];
	regs->tf_ebx    = gdb_regs[ 3];
	regs->tf_ebp    = gdb_regs[ 5];
	regs->tf_esi    = gdb_regs[ 6];
	regs->tf_edi    = gdb_regs[ 7];
	regs->tf_eip    = gdb_regs[ 8];
	regs->tf_eflags = gdb_regs[ 9];
	regs->tf_cs     = gdb_regs[10];
	regs->tf_ds     = gdb_regs[12];
	regs->tf_es     = gdb_regs[13];

	if (KERNELMODE(regs->tf_cs) == 0) {
		/*
		 * Trapped in user mode - restore esp and ss.
		 */
		regs->tf_esp = gdb_regs[ 4];
		regs->tf_ss  = gdb_regs[11];
	}
#else
panic("%s", __func__);
#endif
}

/*
 * Trap into kgdb to wait for debugger to connect,
 * noting on the console why nothing else is going on.
 */
void
kgdb_connect(int verbose)
{
	if (kgdb_dev == NODEV)
		return;

	if (verbose)
		printf("kgdb waiting...");

	breakpoint();

	if (verbose)
		printf("connected.\n");

	kgdb_debug_panic = 1;
}

/*
 * Decide what to do on panic.
 * (This is called by panic, like Debugger())
 */
void
kgdb_panic(void)
{
	if (kgdb_dev != NODEV && kgdb_debug_panic) {
		printf("entering kgdb\n");
		kgdb_connect(kgdb_active == 0);
	}
}

static int kgdb_socket, kgdb_fd;
static int kgdb_connected;


static void
kgdb_get_connection(void)
{
	while (!kgdb_connected) {
		thunk_printf("...[kgdb connecting]...");
		kgdb_fd = thunk_gdb_accept(kgdb_socket);
		if (kgdb_fd)
			kgdb_connected = 1;
	}
	kgdb_active = 1;
}

static int
kgdb_getc(void *arg)
{
	char ch;

	while (thunk_kgdb_getc(kgdb_fd, &ch) < 0) {
		kgdb_connected = 0;
		kgdb_get_connection();
	}
//thunk_printf("[<%c]", ch);
	return (int) ch;
}


static void
kgdb_putc(void *arg, int ch_in)
{
	char ch = (char) ch_in;
	while (thunk_kgdb_putc(kgdb_fd, ch) < 0) {
		kgdb_connected = 0;
		kgdb_get_connection();
	}
//thunk_printf("[>%c]", ch);
}

void
kgdb_port_init(void)
{
	kgdb_connected = 0;

	/* open our socket */
	kgdb_socket = thunk_gdb_open();
	if (kgdb_socket == 0) {
		kgdb_dev = 0;
		printf("aborting kgdb\n");
		return;
	}

	/* signal we have a connection `dev' */
	kgdb_dev = 0x123;
	kgdb_attach(kgdb_getc, kgdb_putc, 0);
}

/*
 * handle an trap instruction encountered from KGDB
 */
void
kgdb_kernel_trap(int signo, vaddr_t pc, vaddr_t va, ucontext_t *ucp)
{
	kgdb_get_connection();

thunk_printf("entering trap\n");
thunk_printf("  signo %d, pc %p, va %p\n", signo, (void *) pc, (void *) va);
	kgdb_trap(signo, ucp);
}

