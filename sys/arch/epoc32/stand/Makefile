#	$NetBSD: Makefile,v 1.2.12.2 2014/05/22 11:39:39 yamt Exp $

.include <bsd.endian.mk>

.if ${MACHINE} == "epoc32" && ${TARGET_ENDIANNESS} == "1234"

SUBDIR+=	e32boot

.endif

.include <bsd.subdir.mk>
