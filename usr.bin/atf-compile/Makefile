# $NetBSD: Makefile,v 1.1.8.1 2008/05/18 12:36:04 yamt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-compile
SRCS=		atf-compile.cpp
MAN=		atf-compile.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
