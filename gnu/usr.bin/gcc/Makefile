#	$NetBSD: Makefile,v 1.4 1997/10/18 06:21:28 lukem Exp $

.if	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "mips") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax")
SUBDIR+=	common cc1 cc1plus cc1obj libgcc libobjc

all-cc1 all-cc1plus all-cc1obj: all-common

.endif

SUBDIR+=	cc cpp g++

.include <bsd.subdir.mk>
