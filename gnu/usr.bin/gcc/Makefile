#	$NetBSD: Makefile,v 1.2 1997/06/23 13:25:43 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "mips") 
SUBDIR=	common cc1 cc1plus cc1obj libgcc libobjc
.endif

SUBDIR=	cc cpp g++

.include <bsd.subdir.mk>
