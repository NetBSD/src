#	$NetBSD: Makefile,v 1.9 1997/06/25 07:22:19 mikel Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax")
SUBDIR=	bfd readline libiberty gdb
.else
SUBDIR=	gdb
.endif

.include <bsd.subdir.mk>
