#	$NetBSD: Makefile,v 1.8 1997/06/23 13:25:50 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax")
SUBDIR=	bfd readline libiberty
.endif

SUBDIR=	gdb

.include <bsd.subdir.mk>
