#	$NetBSD: Makefile,v 1.14 2005/11/24 15:47:09 macallan Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "i386" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcarm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh" || ${MACHINE} == "sparc")
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${NETBSDSRCDIR}/usr.sbin/apmd
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.sbin/apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
