#	$NetBSD: Makefile,v 1.2.108.1 2013/06/23 06:19:58 tls Exp $

.include <bsd.endian.mk>

.if empty(MACHINE_ARCH:Mearm*) && ${TARGET_ENDIANNESS} == "1234"

SUBDIR= boot32 nbfs

.endif

.include <bsd.subdir.mk>
