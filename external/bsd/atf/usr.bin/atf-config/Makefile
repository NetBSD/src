# $NetBSD: Makefile,v 1.7 2014/02/11 17:28:21 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-config
SRCS=		atf-config.cpp
MAN=		atf-config.1

USE_ATF_LIBTOOLS=	yes

CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

.include <bsd.prog.mk>
