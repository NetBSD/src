#	$NetBSD: Makefile,v 1.13 1998/03/02 19:46:44 cgd Exp $

.include <bsd.own.mk>

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDSTATIC?=-static
BINDIR=	/sbin
.endif

MAN=	ldconfig.8

.include <bsd.prog.mk>
