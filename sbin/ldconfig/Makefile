#	$NetBSD: Makefile,v 1.24 2012/08/23 15:45:03 joerg Exp $

.include <bsd.own.mk>   # For MKPIC
.include <bsd.shlib.mk>

.if ${MKPIC} != "no" && \
    (${MACHINE_ARCH} == "arm" ||	\
     ${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_ARCH} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc" ||	\
     ${MACHINE_ARCH} == "vax")

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
