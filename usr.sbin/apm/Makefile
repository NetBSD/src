#	$NetBSD: Makefile,v 1.17.6.1 2014/05/22 11:43:01 yamt Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} == "i386" || \
     ${MACHINE_CPU} == "arm" || \
     ${MACHINE} == "hpcmips" || \
     ${MACHINE} == "hpcsh" || \
     ${MACHINE} == "macppc" || \
     ${MACHINE} == "sparc")
PROG=	apm
SRCS=	apm.c apmsubr.c
.PATH:	${NETBSDSRCDIR}/usr.sbin/apmd
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.sbin/apmd
LINKS=	${BINDIR}/apm ${BINDIR}/zzz
.endif

MAN=	apm.8
MLINKS=	apm.8 zzz.8

.include <bsd.prog.mk>
