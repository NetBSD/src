#	$NetBSD: Makefile,v 1.2 1995/01/20 08:51:48 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat
CFLAGS+=-I/sys -I${.CURDIR}/../vmstat
SRCS=	cmds.c cmdtab.c disks.c fetch.c iostat.c keyboard.c vmstat.c main.c \
	mbufs.c netcmds.c netstat.c pigs.c swap.c
LDADD=	-lcurses -ltermcap -lm -lkvm
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBM} ${LIBKVM}
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
