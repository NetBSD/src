#	$NetBSD: Makefile,v 1.7 1997/10/18 06:42:33 lukem Exp $

.if ${MACHINE} == "i386"
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${.CURDIR}/../apmd
CFLAGS+=-I${.CURDIR}/../apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MANSUBDIR=/i386
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
