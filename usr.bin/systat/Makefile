#	$NetBSD: Makefile,v 1.26 2002/08/08 17:06:31 abs Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat

.PATH:	${.CURDIR}/../../usr.bin/vmstat ${.CURDIR}/../../usr.bin/who

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat -DSUPPORT_UTMP -DSUPPORT_UTMPX \
	-I${.CURDIR}/../../usr.bin/who
CWARNFLAGS+=    -Wno-format-y2k
SRCS=	bufcache.c cmds.c cmdtab.c disks.c dkstats.c fetch.c globalcmds.c \
	icmp.c iostat.c ip.c keyboard.c main.c mbufs.c netcmds.c netstat.c \
	pigs.c ps.c swap.c tcp.c vmstat.c utmpentry.c
DPADD=	${LIBCURSES} ${LIBM} ${LIBKVM}
LDADD=	-lutil -lcurses -lm -lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/systat ${BINDIR}/sysstat
MLINKS+=systat.1 sysstat.1

CPPFLAGS+=-DINET6 -DIPSEC
SRCS+=	ip6.c ipsec.c

.include <bsd.prog.mk>
