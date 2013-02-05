# $NetBSD: Makefile,v 1.3 2013/02/05 07:27:03 matt Exp $

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

