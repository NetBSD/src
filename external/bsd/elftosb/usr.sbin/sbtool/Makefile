# $NetBSD: Makefile,v 1.4 2013/05/02 03:56:39 matt Exp $

.include <bsd.init.mk>
.include <bsd.endian.mk>
.include <bsd.own.mk>

.PATH:		${DIST}/sbtool

.if ${MACHINE_CPU} == "arm" && ${TARGET_ENDIANNESS} == "1234"

PROG_CXX=	sbtool
SRCS=		EncoreBootImageReader.cpp sbtool.cpp

.endif

MAN=		sbtool.8
MANSUBDIR=	/evbarm

.include <bsd.prog.mk>

