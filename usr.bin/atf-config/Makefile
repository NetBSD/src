# $NetBSD: Makefile,v 1.2 2008/05/01 15:35:14 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-config
SRCS=		atf-config.cpp
MAN=		atf-config.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
