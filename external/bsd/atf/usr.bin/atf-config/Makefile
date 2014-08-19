# $NetBSD: Makefile,v 1.5.10.1 2014/08/19 23:45:56 tls Exp $

USE_ATF_LIBTOOLS=	yes

.include <bsd.init.mk>

.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-config
SRCS=		atf-config.cpp
MAN=		atf-config.1

CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

.include <bsd.prog.mk>
