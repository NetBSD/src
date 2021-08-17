#	$NetBSD: Makefile,v 1.1 2021/08/17 17:29:20 christos Exp $

.include "../Makefile.inc"

KMOD=	blake2s

BLAKE2SDIR=	$S/crypto/blake2
CPPFLAGS+=	-I${BLAKE2SDIR}

.PATH: ${BLAKE2SDIR}
SRCS+=	blake2s.c

.include <bsd.kmodule.mk>
