/*	$NetBSD: SYS.h,v 1.9 2002/05/26 11:48:03 wiz Exp $	*/

#include <machine/asm.h>
#include <sys/syscall.h>

#define	_DOSYSCALL(x)		li	0,(SYS_ ## x)		;\
				sc

#define	_SYSCALL_NOERROR(x,y)	.text				;\
				.align	2			;\
			ENTRY(x)				;\
				_DOSYSCALL(y)

#define _SYSCALL(x,y)		.text				;\
				.align	2			;\
			2:	b	PIC_PLT(_C_LABEL(__cerror));\
				_SYSCALL_NOERROR(x,y)		;\
				bso	2b

#define SYSCALL_NOERROR(x)	_SYSCALL_NOERROR(x,x)

#define SYSCALL(x)		_SYSCALL(x,x)

#define PSEUDO_NOERROR(x,y)	_SYSCALL_NOERROR(x,y)		;\
				blr

#define PSEUDO(x,y)		_SYSCALL_NOERROR(x,y)		;\
				bnslr				;\
				b	PIC_PLT(_C_LABEL(__cerror))

#define RSYSCALL_NOERROR(x)	PSEUDO_NOERROR(x,x)

#define RSYSCALL(x)		PSEUDO(x,x)

#define	WSYSCALL(weak,strong)	WEAK_ALIAS(weak,strong)		;\
				PSEUDO(strong,weak)
