# $NetBSD: Makefile,v 1.10 2014/02/11 18:07:30 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-DATF_VERSION=\"$(ATF_VERSION)\"
CPPFLAGS+=	-I${SRCDIR}/tools

USE_ATF_LIBTOOLS=	yes

WARNS?=		2

.include <bsd.prog.mk>
