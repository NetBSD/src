#	$NetBSD: Makefile,v 1.22 2006/04/14 13:14:57 blymn Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	iostat
MAN=	iostat.8

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	drvstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
