#	$NetBSD: Makefile,v 1.3 2003/11/13 13:40:39 manu Exp $

SYSCALL_DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh 
SYSCALL_OBJS=	mach_sysent.c mach_syscalls.c mach_syscall.h mach_syscallargs.h 
MACH_DEP=	mach_services.master makemachservices.sh
MACH_OBJ=	mach_services.c mach_services.h

all:	${SYSCALL_OBJS} ${MACH_OBJ}

.include <bsd.sys.mk>		# for HOST_SH

${SYSCALL_OBJS}: ${SYSCALL_DEP}
	-mv -f mach_sysent.c mach_sysent.c.bak
	-mv -f mach_syscalls.c mach_syscalls.c.bak
	-mv -f mach_syscall.h mach_syscall.h.bak
	-mv -f mach_syscallargs.h mach_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

${MACH_OBJ}: ${MACH_DEP}
	-mv -f mach_services.c mach_services.c.bak
	-mv -f mach_services.h mach_services.h.bak
	${HOSH_SH} ./makemachservices.sh
