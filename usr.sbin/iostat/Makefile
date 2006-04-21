#	$NetBSD: Makefile,v 1.23 2006/04/21 13:46:37 yamt Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	iostat
MAN=	iostat.8

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat

# drvstats.c pulled in from ../../usr.bin/vmstat
SRCS=	drvstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
