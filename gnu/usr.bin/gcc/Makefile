#	$NetBSD: Makefile,v 1.6 1998/07/28 18:53:40 thorpej Exp $

.if	(${MACHINE_ARCH} != "alpha") && \
	(${MACHINE_ARCH} != "powerpc")

SUBDIR+=	common cc1 cc1plus cc1obj libgcc libobjc

all-cc1 all-cc1plus all-cc1obj: all-common

.endif

SUBDIR+=	cc cpp g++

.include <bsd.subdir.mk>
