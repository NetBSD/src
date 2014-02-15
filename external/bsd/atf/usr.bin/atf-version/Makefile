# $NetBSD: Makefile,v 1.12 2014/02/15 04:19:46 jmmv Exp $

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
atf-version.o:	atf-version.txt

.include <bsd.prog.mk>
