#	$NetBSD: Makefile,v 1.9 2002/06/18 05:24:44 itojun Exp $

.if (${MACHINE} == "i386" || ${MACHINE} == "macppc")
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
