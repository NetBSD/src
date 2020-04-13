#	$NetBSD: Makefile,v 1.18.28.1 2020/04/13 08:05:51 martin Exp $

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

COPTS.apm.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
