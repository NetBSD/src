#	$NetBSD: Makefile,v 1.15.20.1 2009/05/13 19:20:15 jym Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "i386" || \
     ${MACHINE} == "hpcarm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE_ARCH} == "sparc" || \
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
