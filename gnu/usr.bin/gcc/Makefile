#	$NetBSD: Makefile,v 1.3 1997/06/24 12:25:57 chopps Exp $

.if	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "mips") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax")
SUBDIR+=	common cc1 cc1plus cc1obj libgcc libobjc
.endif

SUBDIR+=	cc cpp g++

.include <bsd.subdir.mk>
