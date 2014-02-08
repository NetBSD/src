# $NetBSD: Makefile,v 1.6 2014/02/08 19:15:34 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-config
SRCS=		atf-config.cpp
MAN=		atf-config.1

USE_ATF_LIBTOOLS=	yes

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

.include <bsd.prog.mk>
