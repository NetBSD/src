/*	$NetBSD: __syscall.S,v 1.4 2021/02/19 08:33:02 simonb Exp $	*/

#include "SYS.h"

RSYSCALL(__syscall)

#ifndef __mips_o32
STRONG_ALIAS(_syscall,__syscall)
WEAK_ALIAS(syscall,__syscall)
#endif /* ! __mips_o32 */
