# $NetBSD: Makefile,v 1.1.2.3 2013/02/25 00:26:03 tls Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>

.PATH:		${DIST}/sbtool

.if ${MACHINE} == "evbarm" \
    && ${MACHINE_CPU} == "arm" \
    && ${TARGET_ENDIANNESS} == "1234"

PROG_CXX=	sbtool
SRCS=		EncoreBootImageReader.cpp sbtool.cpp

.endif

MAN=		sbtool.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

