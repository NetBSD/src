#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 15:11:12 lukem Exp $

.if ${MACHINE} == "i386"
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${.CURDIR}/../apmd
CPPFLAGS+=-I${.CURDIR}/../apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MANSUBDIR=/i386
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
