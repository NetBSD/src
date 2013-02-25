# $NetBSD: Makefile,v 1.1.2.3 2013/02/25 00:26:03 tls Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>

.PATH:		${DIST}/keygen

.if ${MACHINE} == "evbarm" \
    && ${MACHINE_CPU} == "arm" \
    && ${TARGET_ENDIANNESS} == "1234"

PROG_CXX=	sbkeygen
SRCS=		keygen.cpp

.endif

MAN=		sbkeygen.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

