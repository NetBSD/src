#	$NetBSD: Makefile,v 1.4 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	freebsd_sysent.c freebsd_syscalls.c \
	freebsd_syscall.h freebsd_syscallargs.h

${OBJS}: ${DEP}
	-mv -f freebsd_sysent.c freebsd_sysent.c.bak
	-mv -f freebsd_syscalls.c freebsd_syscalls.c.bak
	-mv -f freebsd_syscall.h freebsd_syscall.h.bak
	-mv -f freebsd_syscallargs.h freebsd_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
