#	$NetBSD: Makefile,v 1.18 2002/01/28 02:16:56 simonb Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	iostat
MAN=	iostat.8

.if ${MACHINE} == "sparc64"
CFLAGS+=	-msoft-quad-float
.endif

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat

# dkstats.c pulled in from ../../usr.bin/vmstat
SRCS=	dkstats.c iostat.c

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
