#	$NetBSD: Makefile,v 1.6 1997/06/23 13:25:57 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ldd
SRCS=	ldd.c
BINDIR= /usr/bin
.endif

MAN=	ldd.1

.include <bsd.prog.mk>
