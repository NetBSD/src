#	$NetBSD: Makefile,v 1.21 2005/08/07 12:34:21 blymn Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	iostat
MAN=	iostat.8

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	dkstats.c tpstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
