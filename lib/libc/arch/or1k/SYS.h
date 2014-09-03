/*	$NetBSD: SYS.h,v 1.1 2014/09/03 19:34:25 matt Exp $	*/

#include <machine/asm.h>
#include <sys/syscall.h>

#define	_DOSYSCALL(x)		l.addi	r13,r0,(SYS_ ## x)	;\
				l.sys	0			;\
				l.nop

#define	_SYSCALL_NOERROR(x,y)	.text				;\
				.p2align 2			;\
			ENTRY(x)				;\
				_DOSYSCALL(y)

#define _SYSCALL(x,y)		.text				;\
				.align	2			;\
			2:	l.j	_C_LABEL(__cerror)	;\
				l.nop				;\
				_SYSCALL_NOERROR(x,y)		;\
				l.bf	2b			;\
				l.nop

#define SYSCALL_NOERROR(x)	_SYSCALL_NOERROR(x,x)

#define SYSCALL(x)		_SYSCALL(x,x)

#define PSEUDO_NOERROR(x,y)	_SYSCALL_NOERROR(x,y)		;\
				l.jr	lr			;\
				l.nop				;\
				END(x)

#define PSEUDO(x,y)		_SYSCALL_NOERROR(x,y)		;\
				l.bf	_C_LABEL(__cerror)	;\
				l.nop				;\
				l.jr	lr			;\
				l.nop				;\
				END(x)

#define RSYSCALL_NOERROR(x)	PSEUDO_NOERROR(x,x)

#define RSYSCALL(x)		PSEUDO(x,x)

#define	WSYSCALL(weak,strong)	WEAK_ALIAS(weak,strong)		;\
				PSEUDO(strong,weak)
