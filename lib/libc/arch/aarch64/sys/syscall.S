/* $NetBSD: syscall.S,v 1.1.4.2 2014/08/20 00:02:09 tls Exp $ */

#define FUNCNAME	syscall
#define	SYSTRAP_SYSCALL	SYSTRAP(syscall)

#include "__syscall.S"
