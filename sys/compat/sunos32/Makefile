#	$NetBSD: Makefile,v 1.2 2003/10/26 07:25:38 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	sunos32_sysent.c sunos32_syscalls.c sunos32_syscall.h sunos32_syscallargs.h

all: ${OBJS} ${OBJS32}

${OBJS}: ${DEP}
	-mv -f sunos32_sysent.c sunos32_sysent.c.bak
	-mv -f sunos32_syscalls.c sunos32_syscalls.c.bak
	-mv -f sunos32_syscall.h sunos32_syscall.h.bak
	-mv -f sunos32_syscallargs.h sunos32_syscallargs.h.bak
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
