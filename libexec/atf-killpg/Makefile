# $NetBSD: Makefile,v 1.1.4.2 2008/03/23 00:41:23 matt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/libexec
PROG_CXX=	atf-killpg
SRCS=		atf-killpg.cpp
MAN=		atf-killpg.1

CPPFLAGS+=	-I${SRCDIR}/libs
LDADD+=		-latf

WARNS?=		2

.include <bsd.prog.mk>
