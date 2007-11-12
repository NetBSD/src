# $NetBSD: Makefile,v 1.1 2007/11/12 15:00:00 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/libexec
PROG_CXX=	atf-cleanup
SRCS=		atf-cleanup.cpp
MAN=		atf-cleanup.1

CPPFLAGS+=	-I${SRCDIR}/libs
LDADD+=		-latf

WARNS?=		2

.include <bsd.prog.mk>
