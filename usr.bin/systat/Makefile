#	$NetBSD: Makefile,v 1.4 1995/12/22 08:04:24 jonathan Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat
.if (${MACHINE_ARCH} == "m68k")
CFLAGS+=-D${MACHINE}
.endif
CFLAGS+=-I${.CURDIR}/../../sys/arch  -I${.CURDIR}/../vmstat

SRCS=	cmds.c cmdtab.c disks.c fetch.c iostat.c keyboard.c main.c mbufs.c \
	netcmds.c netstat.c pigs.c swap.c vmstat.c
LDADD=	-lcurses -ltermcap -lm -lkvm
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBM} ${LIBKVM}
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
