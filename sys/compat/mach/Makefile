#	$NetBSD: Makefile,v 1.1.24.4 2005/03/04 16:40:12 skrll Exp $

SYSCALL_DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
SYSCALL_OBJS=	mach_sysent.c mach_syscalls.c mach_syscall.h mach_syscallargs.h
MACH_DEP=	mach_services.master makemachservices.sh
MACH_OBJ=	mach_services.c mach_services.h mach_services_names.c

all:	${SYSCALL_OBJS} ${MACH_OBJ}

.include <bsd.sys.mk>		# for HOST_SH

${SYSCALL_OBJS}: ${SYSCALL_DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

${MACH_OBJ}: ${MACH_DEP}
	${HOSH_SH} ./makemachservices.sh
