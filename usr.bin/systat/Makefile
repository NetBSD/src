#	$NetBSD: Makefile,v 1.9.2.1 1997/11/08 10:38:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
SRCS=	cmds.c cmdtab.c disks.c dkstats.c fetch.c iostat.c keyboard.c \
	main.c mbufs.c netcmds.c netstat.c pigs.c swap.c vmstat.c
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBM} ${LIBKVM}
LDADD=	-lcurses -ltermcap -lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
