/*	$NetBSD: SYS.h,v 1.7 1999/01/14 22:48:21 kleink Exp $	*/

#include <machine/asm.h>
#include <sys/syscall.h>

#ifdef __STDC__
#define	_DOSYSCALL(x)		li	0,(SYS_ ## x)		;\
				sc
#else
#define	_DOSYSCALL(x)		li	0,(SYS_/**/x)		;\
				sc
#endif /* __STDC__ */

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
