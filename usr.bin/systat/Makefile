#	$NetBSD: Makefile,v 1.8 1997/07/21 07:03:52 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
SRCS=	cmds.c cmdtab.c disks.c dkstats.c fetch.c iostat.c keyboard.c \
	main.c mbufs.c netcmds.c netstat.c pigs.c swap.c vmstat.c
DPADD=	${LIBCURSES} ${LIBTERMCAP} ${LIBM} ${LIBKVM}
LDADD=	-lcurses -ltermcap -lm -lkvm
BINGRP=	kmem
BINMODE=2555
WARNS?=	1

.include <bsd.prog.mk>
