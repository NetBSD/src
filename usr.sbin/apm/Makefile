#	$NetBSD: Makefile,v 1.13 2004/07/03 12:55:47 uch Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "i386" || ${MACHINE} == "macppc" || ${MACHINE} == "hpcarm" || ${MACHINE} == "hpcmips" || ${MACHINE} == "hpcsh")
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${NETBSDSRCDIR}/usr.sbin/apmd
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.sbin/apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
