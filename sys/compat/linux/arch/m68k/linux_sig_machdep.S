/*	$NetBSD: linux_sig_machdep.S,v 1.2 1999/04/19 20:58:38 kleink Exp $	*/

/*
 * Written by ITOH Yasufumi.
 * Public domain.
 */

#include <m68k/asm.h>
#include <compat/linux/linux_syscall.h>

/*
 * Signal "trampoline" code for Linux emulation.
 * The sigtramp for Linux/m68k is on the stack frame, and this is a dummy.
 */
	.align	2
GLOBAL(linux_sigcode)
	moveq	#LINUX_SYS_exit,d0	| _exit(?)
	trap	#0
	.align	2
GLOBAL(linux_esigcode)
