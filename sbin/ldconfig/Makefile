#	$NetBSD: Makefile,v 1.19 2002/02/24 03:18:44 matt Exp $

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

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
