#	$NetBSD: Makefile,v 1.11 2002/09/19 12:08:37 lukem Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "i386" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcarm")
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${NETBSDSRCDIR}/usr.sbin/apmd
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.sbin/apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MANSUBDIR=/i386
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
