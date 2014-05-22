#	$NetBSD: Makefile,v 1.2.98.1 2014/05/22 11:39:26 yamt Exp $

.include <bsd.endian.mk>

.if empty(MACHINE_ARCH:Mearm*) && ${TARGET_ENDIANNESS} == "1234"

SUBDIR= boot32 nbfs

.endif

.include <bsd.subdir.mk>
