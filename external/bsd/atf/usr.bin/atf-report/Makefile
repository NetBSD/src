# $NetBSD: Makefile,v 1.8 2014/02/11 17:28:21 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-report
SRCS=		atf-report.cpp
MAN=		atf-report.1

USE_ATF_LIBTOOLS=	yes

CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

.include <bsd.prog.mk>
