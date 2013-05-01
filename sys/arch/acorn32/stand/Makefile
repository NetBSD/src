#	$NetBSD: Makefile,v 1.3 2013/05/01 22:30:39 matt Exp $

.include <bsd.endian.mk>

.if empty(MACHINE_ARCH:Mearm*) && ${TARGET_ENDIANNESS} == "1234"

SUBDIR= boot32 nbfs

.endif

.include <bsd.subdir.mk>
