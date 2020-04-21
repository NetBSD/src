/* $NetBSD: syscall.S,v 1.1.26.1 2020/04/21 18:41:57 martin Exp $ */

#define FUNCNAME	_syscall
#define	SYSTRAP_SYSCALL	SYSTRAP(syscall)

#include "__syscall.S"
WEAK_ALIAS(syscall,_syscall)
