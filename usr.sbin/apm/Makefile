#	$NetBSD: Makefile,v 1.10 2002/09/16 19:59:00 manu Exp $

.if (${MACHINE} == "i386" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcarm")
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
