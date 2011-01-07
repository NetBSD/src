#	$NetBSD: Makefile,v 1.24 2011/01/07 03:12:27 jakllsch Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	iostat
MAN=	iostat.8

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat

# drvstats.c pulled in from ../../usr.bin/vmstat
SRCS=	drvstats.c iostat.c

DPADD=	${LIBKVM} ${LIBM}
LDADD=	-lkvm -lm

.include <bsd.prog.mk>
