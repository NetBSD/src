#	$NetBSD: Makefile,v 1.24.12.1 2014/08/20 00:05:08 tls Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	iostat
MAN=	iostat.8

.PATH:	${NETBSDSRCDIR}/usr.bin/vmstat

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/vmstat

# drvstats.c pulled in from ../../usr.bin/vmstat
SRCS=	drvstats.c iostat.c

DPADD=	${LIBM}
LDADD=	-lm

.include <bsd.prog.mk>
