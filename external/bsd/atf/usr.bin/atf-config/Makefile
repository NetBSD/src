# $NetBSD: Makefile,v 1.5.4.1 2014/05/22 15:40:15 yamt Exp $

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
