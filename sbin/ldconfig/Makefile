#	$NetBSD: Makefile,v 1.18 2001/03/21 14:54:39 tsutsui Exp $

.include <bsd.own.mk>   # For OBJECT_FMT

.if (${OBJECT_FMT} == "a.out" ||	\
    (${OBJECT_FMT} == "ELF" &&		\
    (${MACHINE_ARCH} == "i386" ||	\
     ${MACHINE_ARCH} == "m68k" ||	\
     ${MACHINE_ARCH} == "sparc")))

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
