#	$NetBSD: Makefile,v 1.20 2002/08/19 10:16:54 lukem Exp $

.include <bsd.own.mk>   # For OBJECT_FMT

.if (${OBJECT_FMT} == "a.out" ||	\
    (${OBJECT_FMT} == "ELF" &&		\
    (${MACHINE_ARCH} == "arm" ||	\
     ${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_ARCH} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc" ||	\
     ${MACHINE_ARCH} == "vax")))

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I${NETBSDSRCDIR}/libexec/ld.aout_so
.PATH:	${NETBSDSRCDIR}/libexec/ld.aout_so

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
