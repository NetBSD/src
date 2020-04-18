/* $NetBSD: syscall.S,v 1.2 2020/04/18 23:55:50 thorpej Exp $ */

#define FUNCNAME	_syscall
#define	SYSTRAP_SYSCALL	SYSTRAP(syscall)

#include "__syscall.S"
WEAK_ALIAS(syscall,_syscall)
