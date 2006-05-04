#	$NetBSD: Makefile,v 1.6 2006/05/04 22:27:51 tsutsui Exp $

.include <bsd.sys.mk>		# for HOST_SH

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	aoutm68k_sysent.c aoutm68k_syscalls.c \
	aoutm68k_syscall.h aoutm68k_syscallargs.h

${OBJS}: ${DEP}
	${HOST_SH} ../../kern/makesyscalls.sh syscalls.conf syscalls.master

all: ${OBJS}

.include <bsd.kinc.mk>
