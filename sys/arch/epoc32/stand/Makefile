#	$NetBSD: Makefile,v 1.2.4.2 2013/06/23 06:20:03 tls Exp $

.include <bsd.endian.mk>

.if ${MACHINE} == "epoc32" && ${TARGET_ENDIANNESS} == "1234"

SUBDIR+=	e32boot

.endif

.include <bsd.subdir.mk>
