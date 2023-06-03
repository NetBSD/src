#	$NetBSD: Makefile,v 1.45 2023/06/03 09:09:18 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

USE_FORT?= yes	# setgid
PROG=	systat

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat ${NETBSDSRCDIR}/usr.bin/who

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat -DSUPPORT_UTMP -DSUPPORT_UTMPX \
	-I${NETBSDSRCDIR}/usr.bin/who -D_KMEMUSER
CWARNFLAGS+=    -Wno-format-y2k
SRCS=	bufcache.c cmds.c cmdtab.c convtbl.c disks.c df.c drvstats.c \
	fetch.c globalcmds.c icmp.c ifcmds.c ifstat.c iostat.c ip.c \
	keyboard.c main.c mbufs.c netcmds.c netstat.c pigs.c ps.c swap.c \
	tcp.c vmstat.c utmpentry.c syscall.c
DPADD=	${LIBUTIL} ${LIBCURSES} ${LIBTERMINFO} ${LIBM} ${LIBKVM}
LDADD=	-lutil -lcurses -lterminfo -lm -lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/systat ${BINDIR}/sysstat
MLINKS+=systat.1 sysstat.1

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
SRCS+=	ip6.c
.endif

.include "../../compat/exec.mk"

COPTS.df.c+=		${CC_WNO_FORMAT_TRUNCATION}
COPTS.ifstat.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.ps.c+=		${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
