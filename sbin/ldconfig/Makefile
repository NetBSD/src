#	$NetBSD: Makefile,v 1.15.10.1 2000/10/18 16:42:52 tv Exp $

.include <bsd.own.mk>   # For OBJECT_FMT

.if (${OBJECT_FMT} == "a.out" || (${OBJECT_FMT} == "ELF" && \
    (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "sparc")))

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c

CPPFLAGS += -I$(.CURDIR)/../../libexec/ld.aout_so
.PATH:	$(.CURDIR)/../../libexec/ld.aout_so

.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
