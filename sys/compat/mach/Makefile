#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	mach_sysent.c mach_syscalls.c mach_syscall.h mach_syscallargs.h

${OBJS}: ${DEP}
	-mv -f mach_sysent.c mach_sysent.c.bak
	-mv -f mach_syscalls.c mach_syscalls.c.bak
	-mv -f mach_syscall.h mach_syscall.h.bak
	-mv -f mach_syscallargs.h mach_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
