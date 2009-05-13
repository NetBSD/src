#	$NetBSD: Makefile,v 1.34.20.1 2009/05/13 19:20:07 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid
PROG=	systat

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat -DSUPPORT_UTMP -DSUPPORT_UTMPX \
	-I${NETBSDSRCDIR}/usr.bin/who
CWARNFLAGS+=    -Wno-format-y2k
SRCS=	bufcache.c cmds.c cmdtab.c disks.c df.c drvstats.c fetch.c \
	globalcmds.c icmp.c iostat.c ip.c keyboard.c main.c mbufs.c \
	netcmds.c netstat.c pigs.c ps.c swap.c tcp.c vmstat.c utmpentry.c \
	syscall.c
DPADD=	${LIBCURSES} ${LIBM} ${LIBKVM}
LDADD=	-lutil -lcurses -lm -lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/systat ${BINDIR}/sysstat
MLINKS+=systat.1 sysstat.1

CPPFLAGS+=-DIPSEC
SRCS+=	ipsec.c

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
SRCS+=	ip6.c
.endif

.include <bsd.prog.mk>
