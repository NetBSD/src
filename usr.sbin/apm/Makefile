#	$NetBSD: Makefile,v 1.8 1997/10/23 06:29:14 lukem Exp $

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
