#	$NetBSD: Makefile,v 1.2 2013/05/01 22:57:37 matt Exp $

.include <bsd.endian.mk>

.if ${MACHINE} == "epoc32" && ${TARGET_ENDIANNESS} == "1234"

SUBDIR+=	e32boot

.endif

.include <bsd.subdir.mk>
