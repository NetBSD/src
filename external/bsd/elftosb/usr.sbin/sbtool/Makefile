# $NetBSD: Makefile,v 1.2.2.3 2014/05/22 15:44:51 yamt Exp $

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

