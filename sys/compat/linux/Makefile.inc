#	$NetBSD: Makefile.inc,v 1.3 2003/10/26 07:25:37 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../../../kern/makesyscalls.sh
OBJS=	linux_sysent.c linux_syscalls.c linux_syscall.h linux_syscallargs.h

${OBJS}: ${DEP}
	-mv -f linux_sysent.c linux_sysent.c.bak
	-mv -f linux_syscalls.c linux_syscalls.c.bak
	-mv -f linux_syscall.h linux_syscall.h.bak
	-mv -f linux_syscallargs.h linux_syscallargs.h.bak
	${HOST_SH} ../../../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}
