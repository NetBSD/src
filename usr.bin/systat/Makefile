#	$NetBSD: Makefile,v 1.27 2002/09/18 14:00:41 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	systat

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat -DSUPPORT_UTMP -DSUPPORT_UTMPX \
	-I${NETBSDSRCDIR}/usr.bin/who
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
