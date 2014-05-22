# $NetBSD: Makefile,v 1.5.4.2 2014/05/22 15:40:15 yamt Exp $

USE_ATF_LIBTOOLS=	yes

.include <bsd.init.mk>

.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-report
SRCS=		atf-report.cpp
MAN=		atf-report.1

CPPFLAGS+=	-I${SRCDIR}/tools

WARNS?=		2

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

.include <bsd.prog.mk>
