# $NetBSD: Makefile,v 1.1.4.2 2008/01/09 02:00:30 matt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-config
SRCS=		atf-config.cpp
MAN=		atf-config.1

CPPFLAGS+=	-I${SRCDIR}/libs
LDADD+=		-latf

WARNS?=		2

.include <bsd.prog.mk>
