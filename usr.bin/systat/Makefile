#	$NetBSD: Makefile,v 1.19 1999/11/15 06:16:56 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
CWARNFLAGS+=    -Wno-format-y2k
SRCS=	bufcache.c cmds.c cmdtab.c disks.c dkstats.c fetch.c icmp.c iostat.c \
	ip.c keyboard.c main.c mbufs.c netcmds.c netstat.c pigs.c ps.c swap.c \
	tcp.c vmstat.c
DPADD=	${LIBCURSES} ${LIBM} ${LIBKVM}
LDADD=	-lcurses -lm -lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/systat ${BINDIR}/sysstat
MLINKS+=systat.1 sysstat.1

.include <bsd.prog.mk>
