#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:34:19 mrg Exp $

.if	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "mips")
SUBDIR+=	common cc1 cc1plus cc1obj libgcc libobjc

all-cc1 all-cc1plus all-cc1obj: all-common

.endif

SUBDIR+=	cc cpp g++

.include <bsd.subdir.mk>
