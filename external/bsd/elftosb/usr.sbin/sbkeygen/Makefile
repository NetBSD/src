# $NetBSD: Makefile,v 1.4 2013/05/02 03:56:39 matt Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>
.include <bsd.own.mk>

.PATH:		${DIST}/keygen

.if ${MACHINE_CPU} == "arm" && ${TARGET_ENDIANNESS} == "1234"

PROG_CXX=	sbkeygen
SRCS=		keygen.cpp

.endif

MAN=		sbkeygen.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

