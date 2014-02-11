# $NetBSD: Makefile,v 1.9 2014/02/11 17:28:21 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-DATF_VERSION=\"$(ATF_VERSION)\"
CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I${.CURDIR}

USE_ATF_LIBTOOLS=	yes

WARNS?=		2

.include <bsd.prog.mk>
