#	$NetBSD: Makefile,v 1.1.154.1 2014/05/22 11:39:26 yamt Exp $

.include <bsd.endian.mk>

.if empty(MACHINE_ARCH:Mearm*) && ${TARGET_ENDIANNESS} == "1234"

SUBDIR= boot26

.endif

.include <bsd.subdir.mk>
