#	$NetBSD: Makefile,v 1.1.26.2 2004/09/18 14:43:04 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aoutm68k_sysent.c aoutm68k_syscalls.c \
	aoutm68k_syscall.h aoutm68k_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master
	rm -f aoutm68k_syscalls.c	# garbage-collect, not needed

all: ${OBJS}

.include <bsd.kinc.mk>
