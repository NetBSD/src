#	$NetBSD: Makefile,v 1.15.16.2 2007/07/29 14:34:46 nonaka Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "i386" || \
     ${MACHINE} == "hpcarm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE} == "sparc" || \
     ${MACHINE} == "zaurus")
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${NETBSDSRCDIR}/usr.sbin/apmd
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.sbin/apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
