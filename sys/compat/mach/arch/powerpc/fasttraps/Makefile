#	$NetBSD: Makefile,v 1.1.8.2 2004/08/06 10:14:01 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../../kern/makesyscalls.sh
OBJS=	mach_fasttraps_sysent.c mach_fasttraps_syscalls.c \
	mach_fasttraps_syscall.h mach_fasttraps_syscallargs.h

${OBJS}: ${DEP}
	-mv -f mach_fasttraps_sysent.c mach_fasttraps_sysent.c.bak
	-mv -f mach_fasttraps_syscalls.c mach_fasttraps_syscalls.c.bak
	-mv -f mach_fasttraps_syscall.h mach_fasttraps_syscall.h.bak
	-mv -f mach_fasttraps_syscallargs.h mach_fasttraps_syscallargs.h.bak
	${HOST_SH} ../../../../../kern/makesyscalls.sh syscalls.conf syscalls.master
