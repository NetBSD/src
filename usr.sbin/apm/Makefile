#	$NetBSD: Makefile,v 1.6 1997/10/03 05:39:03 mikel Exp $

.if ${MACHINE} == "i386"
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${.CURDIR}/../apmd
CFLAGS+=-I${.CURDIR}/../apmd
WARNS?=	1
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MANSUBDIR=/i386
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
