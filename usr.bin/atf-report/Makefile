# $NetBSD: Makefile,v 1.2 2008/05/01 15:35:14 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-report
SRCS=		atf-report.cpp
MAN=		atf-report.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
