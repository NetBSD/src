# $NetBSD: Makefile,v 1.1.6.1 2008/05/18 12:30:43 yamt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/libexec
PROG_CXX=	atf-killpg
SRCS=		atf-killpg.cpp
MAN=		atf-killpg.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
