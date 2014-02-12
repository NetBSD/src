# $NetBSD: Makefile,v 1.11 2014/02/12 04:08:32 jmmv Exp $

USE_ATF_LIBTOOLS=	yes

.include <bsd.init.mk>

.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

CPPFLAGS+=	-DATF_VERSION=\"$(ATF_VERSION)\"
atf-version.o:	cookie-version

.include <bsd.prog.mk>
