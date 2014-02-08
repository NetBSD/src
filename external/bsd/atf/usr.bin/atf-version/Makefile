# $NetBSD: Makefile,v 1.8 2014/02/08 19:15:34 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I${SRCDIR:H}/lib/libatf-c  # For bconfig.h.
CPPFLAGS+=	-I${.CURDIR}

USE_ATF_LIBTOOLS=	yes

WARNS?=		2

.include <bsd.prog.mk>
