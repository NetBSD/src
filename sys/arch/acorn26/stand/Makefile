#	$NetBSD: Makefile,v 1.2 2013/05/01 22:36:52 matt Exp $

.include <bsd.endian.mk>

.if empty(MACHINE_ARCH:Mearm*) && ${TARGET_ENDIANNESS} == "1234"

SUBDIR= boot26

.endif

.include <bsd.subdir.mk>
