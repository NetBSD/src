/*	$NetBSD: linux_sig_machdep.S,v 1.2.8.1 1999/12/27 18:34:25 wrstuden Exp $	*/

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
	moveq	#LINUX_SYS_exit,%d0	| _exit(?)
	trap	#0
	.align	2
GLOBAL(linux_esigcode)
